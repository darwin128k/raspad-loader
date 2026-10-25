#include "identity.h"
#include "revemu2013.h"

#include <authum/crypto/ed25519.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

void Vellum_Log(const char *fmt, ...);

#ifdef _WIN32
#define vellum_snprintf _snprintf
#else
#define vellum_snprintf snprintf
#endif

static_assert(sizeof(VellumTicket) == 120, "Vellum v2 ticket must stay 120 bytes");

static VellumIdentity g_id;
static int g_ready;

static void JoinPath(char *out, size_t outSize, const char *dir, const char *file)
{
	size_t n = strlen(dir);
	if (n > 0 && (dir[n - 1] == '\\' || dir[n - 1] == '/')) {
		vellum_snprintf(out, outSize, "%s%s", dir, file);
	} else {
#ifdef _WIN32
		vellum_snprintf(out, outSize, "%s\\%s", dir, file);
#else
		vellum_snprintf(out, outSize, "%s/%s", dir, file);
#endif
	}
	out[outSize - 1] = '\0';
}

#ifdef _WIN32
static void DirFromThisDll(char *dir, size_t dirSize)
{
	HMODULE mod = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                   (LPCSTR)&Vellum_InitIdentity, &mod);
	GetModuleFileNameA(mod, dir, (DWORD)dirSize);
	dir[dirSize - 1] = '\0';
	char *slash = strrchr(dir, '\\');
	if (slash != NULL) {
		slash[1] = '\0';
	}
}
#else
static void DirFromThisDll(char *dir, size_t dirSize)
{
	ssize_t n = readlink("/proc/self/exe", dir, dirSize - 1);
	char *slash;

	if (n > 0) {
		dir[n] = '\0';
		slash = strrchr(dir, '/');
		if (slash != NULL) {
			slash[1] = '\0';
			return;
		}
	}
	strncpy(dir, "./", dirSize - 1);
	dir[dirSize - 1] = '\0';
}
#endif

static uint32 ReadAppId(const char *dir)
{
	const char *env = getenv("SteamAppId");
	if (env != NULL && env[0] != '\0') {
		return (uint32)strtoul(env, NULL, 10);
	}
	char path[MAX_PATH];
	JoinPath(path, sizeof(path), dir, "steam_appid.txt");
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		return 10;
	}
	char buf[32] = {};
	size_t n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';
	uint32 id = (uint32)strtoul(buf, NULL, 10);
	return id ? id : 10;
}

#ifdef _WIN32
static void FillPersona(char *persona, size_t personaSize)
{
	DWORD n = (DWORD)personaSize;
	if (!GetUserNameA(persona, &n) || persona[0] == '\0') {
		strncpy(persona, "Vellum", personaSize - 1);
		persona[personaSize - 1] = '\0';
	}
}

/* %APPDATA%\Raspad — per Windows user, never inside the game folder: game
 * folders get zipped and shared, and a shared key is a shared SteamID. */
static int KeyDir(char *dir, size_t dirSize)
{
	const char *appdata = getenv("APPDATA");
	if (appdata == NULL || appdata[0] == '\0') {
		return 0;
	}
	JoinPath(dir, dirSize, appdata, "Raspad");
	CreateDirectoryA(dir, NULL);
	return 1;
}

static int RandomBytes(uint8_t *out, size_t n)
{
	return BCryptGenRandom(NULL, out, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}

static FILE *OpenKeyForWrite(const char *path)
{
	return fopen(path, "w");
}
#else
static void FillPersona(char *persona, size_t personaSize)
{
	struct passwd *pw = getpwuid(getuid());
	if (pw != NULL && pw->pw_name != NULL && pw->pw_name[0] != '\0') {
		strncpy(persona, pw->pw_name, personaSize - 1);
		persona[personaSize - 1] = '\0';
	} else {
		strncpy(persona, "Vellum", personaSize - 1);
		persona[personaSize - 1] = '\0';
	}
}

/* $XDG_DATA_HOME/raspad or ~/.local/share/raspad, mode 0700. */
static int KeyDir(char *dir, size_t dirSize)
{
	const char *xdg = getenv("XDG_DATA_HOME");
	const char *home = getenv("HOME");
	char local[MAX_PATH];
	char base[MAX_PATH];

	if (xdg != NULL && xdg[0] == '/') {
		strncpy(base, xdg, sizeof(base) - 1);
		base[sizeof(base) - 1] = '\0';
	} else if (home != NULL && home[0] == '/') {
		JoinPath(local, sizeof(local), home, ".local");
		mkdir(local, 0700);
		JoinPath(base, sizeof(base), local, "share");
	} else {
		return 0;
	}
	mkdir(base, 0700);
	JoinPath(dir, dirSize, base, "raspad");
	mkdir(dir, 0700);
	return 1;
}

static int RandomBytes(uint8_t *out, size_t n)
{
	FILE *f = fopen("/dev/urandom", "rb");
	size_t got;
	if (f == NULL) {
		return 0;
	}
	got = fread(out, 1, n, f);
	fclose(f);
	return got == n;
}

static FILE *OpenKeyForWrite(const char *path)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	return fd >= 0 ? fdopen(fd, "w") : NULL;
}
#endif

/* Key file: "vellum-key 1 <64 hex seed>". The seed is the whole secret. */
static int LoadSeed(const char *path, uint8_t seed[32])
{
	FILE *f = fopen(path, "r");
	char hex[65] = {};
	unsigned version = 0;
	unsigned byte;
	int ok;
	int i;

	if (f == NULL) {
		return 0;
	}
	ok = fscanf(f, "vellum-key %u %64s", &version, hex) == 2;
	fclose(f);
	if (!ok || version != 1 || strlen(hex) != 64) {
		return 0;
	}
	for (i = 0; i < 32; i++) {
		if (sscanf(hex + i * 2, "%2x", &byte) != 1) {
			return 0;
		}
		seed[i] = (uint8_t)byte;
	}
	return 1;
}

static int SaveSeed(const char *path, const uint8_t seed[32])
{
	FILE *f = OpenKeyForWrite(path);
	int ok;
	int i;

	if (f == NULL) {
		return 0;
	}
	ok = fprintf(f, "vellum-key 1 ") > 0;
	for (i = 0; i < 32 && ok; i++) {
		ok = fprintf(f, "%02x", (unsigned)seed[i]) > 0;
	}
	ok = ok && fputc('\n', f) != EOF;
	ok = (fclose(f) == 0) && ok;
	return ok;
}

/* Load the profile key, or make and save a new one. */
static void LoadOrCreateKey(VellumIdentity *id)
{
	uint8_t seed[AUTHUM_CRYPTO_ED25519_SEED_SIZE];
	char dir[MAX_PATH];
	char path[MAX_PATH];
	int have_path = KeyDir(dir, sizeof(dir));

	if (have_path) {
		JoinPath(path, sizeof(path), dir, "vellum.key");
	}
	if (have_path && LoadSeed(path, seed)) {
		id->key_saved = true;
		Vellum_Log("ident key loaded from %s", path);
	} else {
		if (!RandomBytes(seed, sizeof(seed))) {
			Vellum_Log("ident FATAL: no system randomness - cannot create a key");
			memset(seed, 0, sizeof(seed));
		}
		id->key_saved = have_path && SaveSeed(path, seed);
		if (id->key_saved) {
			Vellum_Log("ident new key saved to %s", path);
		} else {
			Vellum_Log("ident WARNING: key not saved (%s) - SteamID will change next run",
			           have_path ? path : "no profile dir");
		}
	}
	/* key_pair wipes its seed argument. */
	authum_crypto_ed25519_key_pair(id->secret_key, id->public_key, seed);
}

#ifdef VELLUM_AUTH_REVEMU2013
#ifndef _WIN32
static uint32 ReadMachineSerial(void)
{
	FILE *f = fopen("/etc/machine-id", "r");
	char buf[64] = {};
	size_t n;
	size_t i;
	unsigned v = 0;
	int digits = 0;

	if (f == NULL) {
		f = fopen("/var/lib/dbus/machine-id", "r");
	}
	if (f == NULL) {
		return 1;
	}
	n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';

	for (i = 0; buf[i] != '\0' && digits < 8; i++) {
		unsigned d;
		char c = buf[i];
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			continue;
		}
		if (c >= '0' && c <= '9') {
			d = (unsigned)(c - '0');
		} else if (c >= 'a' && c <= 'f') {
			d = (unsigned)(c - 'a' + 10);
		} else if (c >= 'A' && c <= 'F') {
			d = (unsigned)(c - 'A' + 10);
		} else {
			break;
		}
		v = (v << 4) | d;
		digits++;
	}
	return digits == 8 && v != 0 ? (uint32)v : 1u;
}

#endif
static void FillRevEmuAuthKey(char *key, size_t size)
{
#ifdef _WIN32
	DWORD serial = 0;
	GetVolumeInformationA("C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0);
#else
	uint32 serial = ReadMachineSerial();
#endif
	vellum_snprintf(key, size, "%u", (unsigned)(serial ? serial : 1));
	key[size - 1] = '\0';
}
#endif

void Vellum_InitIdentity()
{
	if (g_ready) {
		return;
	}
	memset(&g_id, 0, sizeof(g_id));

	char dir[MAX_PATH];
	DirFromThisDll(dir, sizeof(dir));

	FillPersona(g_id.persona, sizeof(g_id.persona));
#ifdef VELLUM_AUTH_REVEMU2013
	FillRevEmuAuthKey(g_id.auth_key, sizeof(g_id.auth_key));
	g_id.account_id = RevEmu2013_AccountId(g_id.auth_key);
#else
	LoadOrCreateKey(&g_id);
	g_id.account_id = Vellum_AccountIdFromKey(g_id.public_key);
#endif
	g_id.app_id = ReadAppId(dir);
	g_id.steam_id = CSteamID(g_id.account_id, k_EUniversePublic, k_EAccountTypeIndividual);
	g_ready = 1;
#ifdef VELLUM_AUTH_REVEMU2013
	Vellum_Log("ident kind=revemu2013 persona=%s account=%u app=%u steamid=%llu",
#else
	Vellum_Log("ident kind=vellum-v2 persona=%s account=%u app=%u steamid=%llu",
#endif
	           g_id.persona, (unsigned)g_id.account_id, (unsigned)g_id.app_id,
	           (unsigned long long)g_id.steam_id.ConvertToUint64());
}

const VellumIdentity &Vellum_GetIdentity()
{
	Vellum_InitIdentity();
	return g_id;
}

int Vellum_WriteAuthBlob(void *blob, int maxBytes, uint32 serverIp, uint16 serverPort)
{
	const VellumIdentity &id = Vellum_GetIdentity();
#ifdef VELLUM_AUTH_REVEMU2013
	return RevEmu2013_WriteTicket(blob, maxBytes, id.auth_key);
#else
	VellumTicket ticket;
	if (blob == NULL || maxBytes < (int)sizeof(VellumTicket)) {
		return 0;
	}
	Vellum_PrepareTicket(&ticket, serverIp, serverPort, (uint64_t)time(NULL), id.public_key);
	authum_crypto_ed25519_sign(ticket.signature, id.secret_key, (const authum_byte_t *)&ticket,
	                           (authum_usize_t)VELLUM_TICKET_SIGNED_SIZE);
	memcpy(blob, &ticket, sizeof(ticket));
	return (int)sizeof(ticket);
#endif
}
