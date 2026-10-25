#include "identity.h"
#ifdef VELLUM_AUTH_REVEMU2013
#include "revemu2013.h"
#endif
#ifndef VELLUM_AUTH_REVEMU2013
#include <authum/crypto/ed25519.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

void Vellum_Log(const char *, ...) {}

int main()
{
	unsigned char blob[256];
	const VellumIdentity &id = Vellum_GetIdentity();
#ifdef VELLUM_AUTH_REVEMU2013
	const int expected = REVEMU2013_TICKET_SIZE;
#else
	const int expected = sizeof(VellumTicket);
#endif
	memset(blob, 0xA5, sizeof(blob));
	assert(Vellum_WriteAuthBlob(NULL, sizeof(blob), 0, 0) == 0);
	assert(Vellum_WriteAuthBlob(blob, expected - 1, 0, 0) == 0);
	assert(blob[0] == 0xA5);
	assert(Vellum_WriteAuthBlob(blob, sizeof(blob), 0x01020304, 27015) == expected);
	assert(blob[expected] == 0xA5);
#ifdef VELLUM_AUTH_REVEMU2013
	uint32_t account = 0;
	assert(RevEmu2013_AccountIdFromTicket(blob, expected, &account));
	assert(account == id.account_id);
	assert(account == RevEmu2013_AccountId(id.auth_key));
	assert(!RevEmu2013_AccountIdFromTicket(blob, 135, &account));
	blob[0] ^= 1;
	assert(!RevEmu2013_AccountIdFromTicket(blob, expected, &account));
#else
	VellumTicket ticket;
	memcpy(&ticket, blob, sizeof(ticket));
	assert(ticket.magic == VELLUM_TICKET_MAGIC && ticket.version == 2);
	assert(ticket.server_ip == 0x01020304 && ticket.server_port == 27015);
	assert(Vellum_AccountIdFromKey(ticket.public_key) == id.account_id);
	assert(authum_crypto_ed25519_check(ticket.signature, ticket.public_key,
		(const authum_byte_t *)&ticket, VELLUM_TICKET_SIGNED_SIZE));
	ticket.server_port++;
	assert(!authum_crypto_ed25519_check(ticket.signature, ticket.public_key,
		(const authum_byte_t *)&ticket, VELLUM_TICKET_SIGNED_SIZE));
#endif
	printf("auth smoke passed: %d bytes, account=%u\n", expected, (unsigned)id.account_id);
}
