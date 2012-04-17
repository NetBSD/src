
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2005
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"

#define	PGSIZE sysconf(_SC_PAGESIZE)
#define PGOFFSET (PGSIZE - 1)
#define PGMASK (~PGOFFSET)

/*
 *  popup_GetSecret()
 *
 *    newPIN - non-zero to popup the dialog to enter a new PIN, zero to popup a dialog
 *      to enter an existing PIN
 *    hash_mode - flag indicating whether to include null terminating data in the hash
 *      of the secret (1.2 backport only).
 *    popup_str - string to appear in the title bar of the popup dialog
 *    auth_hash - the 20+ byte buffer that receives the SHA1 hash of the auth data
 *      entered into the dialog box
 *
 */
TSS_RESULT
popup_GetSecret(UINT32 new_pin, UINT32 hash_mode, BYTE *popup_str, void *auth_hash)
{
	BYTE secret[UI_MAX_SECRET_STRING_LENGTH] = { 0 };
	BYTE *dflt = (BYTE *)"TSS Authentication Dialog";
	UINT32 secret_len = 0;
	TSS_RESULT result;

	if (popup_str == NULL)
		popup_str = dflt;

	/* pin the area where the secret will be put in memory */
	if (pin_mem(&secret, UI_MAX_SECRET_STRING_LENGTH)) {
		LogError("Failed to pin secret in memory.");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (new_pin)
		DisplayNewPINWindow(secret, &secret_len, popup_str);
	else
		DisplayPINWindow(secret, &secret_len, popup_str);

	if (!secret_len) {
		unpin_mem(&secret, UI_MAX_SECRET_STRING_LENGTH);
		return TSPERR(TSS_E_POLICY_NO_SECRET);
	}

	if (hash_mode == TSS_TSPATTRIB_HASH_MODE_NOT_NULL)
		secret_len -= sizeof(TSS_UNICODE); // Take off the NULL terminator

	LogDebug("Hashing these %u bytes as the secret:", secret_len);
	LogDebugData(secret_len, secret);
	result = Trspi_Hash(TSS_HASH_SHA1, secret_len, secret, auth_hash);

	/* zero, then unpin the memory */
	memset(secret, 0, secret_len);
	unpin_mem(&secret, UI_MAX_SECRET_STRING_LENGTH);

	return result;
}

int
pin_mem(void *addr, size_t len)
{
	/* only root can lock pages into RAM */
	if (getuid() != (uid_t)0) {
		LogWarn("Not pinning secrets in memory due to insufficient perms.");
		return 0;
	}

	len += (uintptr_t)addr & PGOFFSET;
	addr = (void *)((uintptr_t)addr & PGMASK);
	if (mlock(addr, len) == -1) {
		LogError("mlock: %s", strerror(errno));
		return 1;
	}

	return 0;
}

int
unpin_mem(void *addr, size_t len)
{
	/* only root can lock pages into RAM */
	if (getuid() != (uid_t)0) {
		return 0;
	}

	len += (uintptr_t)addr & PGOFFSET;
	addr = (void *)((uintptr_t)addr & PGMASK);
	if (munlock(addr, len) == -1) {
		LogError("mlock: %s", strerror(errno));
		return 1;
	}

	return 0;
}


