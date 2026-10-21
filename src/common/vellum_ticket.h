#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Vellum ticket v2 — GoldSrc connect blob emitted by this steamclient and
 * checked by qproto (Authum module) as qproto_client_auth_kind_vellum.
 *
 * The player's Ed25519 key pair lives in their user profile (identity.cpp).
 * Each connect is signed over the server address the engine dialled and the
 * local clock, so a ticket one server collects is useless on another. The
 * SteamID is derived from the public key; qproto pins SteamID -> key on first
 * sight. Nothing about this machine (hostname, disk serial) is sent.
 *
 * v1 (64 bytes, FNV checksum, plaintext "HOSTNAME-SERIAL") is refused by
 * qproto 2.x.
 *
 * Keep this header in sync with raspad-qproto/include/qproto/vellum/ticket.h. */

#define VELLUM_TICKET_MAGIC     0x4D4C4C56u /* 'VLLM' little-endian */
#define VELLUM_TICKET_VERSION   2
#define VELLUM_PUBLIC_KEY_SIZE  32
#define VELLUM_SIGNATURE_SIZE   64

#pragma pack(push, 1)
typedef struct VellumTicket {
	uint32_t magic;
	uint16_t version;
	uint16_t flags;       /* 0 — reserved */
	uint32_t server_ip;   /* as dialled, 1.2.3.4 -> 0x01020304 (Steam host order) */
	uint16_t server_port; /* host order */
	uint16_t reserved;    /* 0 */
	uint64_t timestamp;   /* local clock, UTC seconds since 1970 */
	uint8_t  public_key[VELLUM_PUBLIC_KEY_SIZE];
	uint8_t  signature[VELLUM_SIGNATURE_SIZE]; /* Ed25519 over every byte before it */
} VellumTicket;
#pragma pack(pop)

/* Bytes covered by the signature: the whole ticket up to `signature`. */
#define VELLUM_TICKET_SIGNED_SIZE offsetof(VellumTicket, signature)

/* Account id (low 32 bits of the SteamID) for a public key. 0/1 are LAN/PENDING. */
static inline uint32_t Vellum_AccountIdFromKey(const uint8_t public_key[VELLUM_PUBLIC_KEY_SIZE])
{
	uint32_t id = (uint32_t)public_key[0] | ((uint32_t)public_key[1] << 8) |
	              ((uint32_t)public_key[2] << 16) | ((uint32_t)public_key[3] << 24);
	id &= 0x7FFFFFFFu;
	return id >= 2u ? id : id + 2u;
}

/* Everything but the signature, which is then made over
 * VELLUM_TICKET_SIGNED_SIZE bytes of `t`. */
static inline void Vellum_PrepareTicket(VellumTicket *t, uint32_t server_ip, uint16_t server_port,
                                        uint64_t timestamp, const uint8_t public_key[VELLUM_PUBLIC_KEY_SIZE])
{
	memset(t, 0, sizeof(*t));
	t->magic = VELLUM_TICKET_MAGIC;
	t->version = VELLUM_TICKET_VERSION;
	t->server_ip = server_ip;
	t->server_port = server_port;
	t->timestamp = timestamp;
	memcpy(t->public_key, public_key, VELLUM_PUBLIC_KEY_SIZE);
}
