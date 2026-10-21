#pragma once

#include "types.h"
#include "vellum_ticket.h"

struct VellumIdentity {
	char persona[64];
	uint8_t secret_key[64]; /* Ed25519: seed + public key */
	uint8_t public_key[VELLUM_PUBLIC_KEY_SIZE];
	uint32 account_id;
	uint32 app_id;
	CSteamID steam_id;
	bool key_saved; /* false: key lives only in this process, SteamID changes next run */
};

void Vellum_InitIdentity();
const VellumIdentity &Vellum_GetIdentity();
/* Signed v2 ticket for the server the engine is dialling (ip/port 0 when unknown). */
int Vellum_WriteAuthBlob(void *blob, int maxBytes, uint32 serverIp, uint16 serverPort);
