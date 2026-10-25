#include "vellum_config.h"
#include "steam_http.h" /* Vellum_Log */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#	include <windows.h>
#else
#	include <unistd.h>
#endif

#define VELLUM_CFG_MAX_LINE 512

static char g_cfg_masters[VELLUM_CFG_MAX_MASTERS][512];
static int g_cfg_master_n;
static int g_cfg_internet = 1;
static int g_cfg_loaded;

void Vellum_GameDir(char *dir, size_t dirSize)
{
	if (dir == NULL || dirSize == 0) {
		return;
	}
	dir[0] = '\0';
#ifdef _WIN32
	{
		HMODULE mod = NULL;
		char *slash;

		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		                   (LPCSTR)&Vellum_GameDir, &mod);
		GetModuleFileNameA(mod, dir, (DWORD)dirSize);
		dir[dirSize - 1] = '\0';
		slash = strrchr(dir, '\\');
		if (slash != NULL) {
			slash[1] = '\0';
		}
	}
#else
	{
		ssize_t n = readlink("/proc/self/exe", dir, dirSize - 1);
		char *slash;

		if (n > 0) {
			dir[n] = '\0';
			slash = strrchr(dir, '/');
			if (slash != NULL) {
				slash[1] = '\0';
			}
		} else {
			strncpy(dir, "./", dirSize - 1);
			dir[dirSize - 1] = '\0';
		}
	}
#endif
}

static void Vellum_Trim(char *s)
{
	size_t n;

	while (*s == ' ' || *s == '\t') {
		s++;
	}
	n = strlen(s);
	while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n')) {
		s[--n] = '\0';
	}
}

/* "a:1, b:2" -> two masters.  A trailing comma or an empty item is ignored. */
static void Vellum_ConfigParseMasters(char *value)
{
	char *p = value;

	while (*p != '\0' && g_cfg_master_n < VELLUM_CFG_MAX_MASTERS) {
		char *sep = strchr(p, ',');
		char *end = (sep != NULL) ? sep : (p + strlen(p));
		size_t n;

		while (*p == ' ' || *p == '\t') {
			p++;
		}
		n = (size_t)(end - p);
		while (n > 0 && (p[n - 1] == ' ' || p[n - 1] == '\t')) {
			n--;
		}
		if (n > 0 && n < sizeof(g_cfg_masters[0])) {
			memcpy(g_cfg_masters[g_cfg_master_n], p, n);
			g_cfg_masters[g_cfg_master_n][n] = '\0';
			g_cfg_master_n++;
		}
		if (sep == NULL) {
			break;
		}
		p = sep + 1;
	}
}

static int Vellum_ConfigParseBool(const char *value, int fallback)
{
	if (value == NULL || value[0] == '\0') {
		return fallback;
	}
	if (strcmp(value, "0") == 0 || strcmp(value, "off") == 0 ||
	    strcmp(value, "false") == 0 || strcmp(value, "no") == 0) {
		return 0;
	}
	if (strcmp(value, "1") == 0 || strcmp(value, "on") == 0 ||
	    strcmp(value, "true") == 0 || strcmp(value, "yes") == 0) {
		return 1;
	}
	return fallback;
}

void Vellum_ConfigLoad(void)
{
	char dir[512];
	char path[512];
	char line[VELLUM_CFG_MAX_LINE];
	FILE *f;

	if (g_cfg_loaded) {
		return;
	}
	g_cfg_loaded = 1;
	g_cfg_master_n = 0;
	g_cfg_internet = 1;

	Vellum_GameDir(dir, sizeof(dir));
#ifdef _WIN32
	_snprintf(path, sizeof(path), "%sconfig\\vellum.cfg", dir);
#else
	snprintf(path, sizeof(path), "%sconfig/vellum.cfg", dir);
#endif
	path[sizeof(path) - 1] = '\0';

	f = fopen(path, "r");
	if (f == NULL) {
		Vellum_Log("config: no %s - internet on, no master configured", path);
		return;
	}

	while (fgets(line, sizeof(line), f) != NULL) {
		char *eq;
		char *key;
		char *value;

		Vellum_Trim(line);
		if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
			continue;
		}
		eq = strchr(line, '=');
		if (eq == NULL) {
			continue;
		}
		*eq = '\0';
		key = line;
		value = eq + 1;
		Vellum_Trim(key);
		Vellum_Trim(value);

		if (strcmp(key, "masters") == 0) {
			Vellum_ConfigParseMasters(value);
		} else if (strcmp(key, "internet") == 0) {
			g_cfg_internet = Vellum_ConfigParseBool(value, 1);
		}
	}
	fclose(f);

	Vellum_Log("config: %s, internet=%d masters=%d", path, g_cfg_internet, g_cfg_master_n);
}

int Vellum_ConfigInternetEnabled(void)
{
	Vellum_ConfigLoad();
	return g_cfg_internet;
}

int Vellum_ConfigMasterCount(void)
{
	return g_cfg_master_n;
}

const char *Vellum_ConfigMaster(int index)
{
	if (index < 0 || index >= g_cfg_master_n) {
		return NULL;
	}
	return g_cfg_masters[index];
}
