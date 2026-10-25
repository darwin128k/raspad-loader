#pragma once

#include "types.h"
#ifndef VELLUM_AUTH_REVEMU2013
#include "vellum_ticket.h"
#endif

struct VellumIdentity {
	char persona[64];
#ifdef VELLUM_AUTH_REVEMU2013
	char auth_key[32];
#else
	uint8_t secret_key[64]; /* Ed25519: seed + public key */
	uint8_t public_key[VELLUM_PUBLIC_KEY_SIZE];
	bool key_saved; /* false: key is transient */
#endif
	uint32 account_id;
	uint32 app_id;
	CSteamID steam_id;
};

void Vellum_InitIdentity();
const VellumIdentity &Vellum_GetIdentity();
/* Selected auth dialect; Vellum v2 is signed for the server the engine is dialling (ip/port 0 when unknown). */
int Vellum_WriteAuthBlob(void *blob, int maxBytes, uint32 serverIp, uint16 serverPort);
