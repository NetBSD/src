
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcsps.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "req_mgr.h"
#include "tcsd_wrap.h"
#include "tcsd.h"

TSS_RESULT
UnloadBlob_MIGRATIONKEYAUTH(UINT64 *offset, BYTE *blob, TCPA_MIGRATIONKEYAUTH *mkAuth)
{
	TSS_RESULT result;

	if (!mkAuth) {
		if ((result = UnloadBlob_PUBKEY(offset, blob, NULL)))
			return result;

		UnloadBlob_UINT16(offset, NULL, blob);
		UnloadBlob(offset, 20, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = UnloadBlob_PUBKEY(offset, blob, &mkAuth->migrationKey)))
		return result;

	UnloadBlob_UINT16(offset, &mkAuth->migrationScheme, blob);
	UnloadBlob(offset, 20, blob, mkAuth->digest.digest);

	return result;
}
