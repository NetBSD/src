
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsps.h"
#include "tcslog.h"
#include "tddl.h"
#include "req_mgr.h"


TSS_RESULT
get_current_version(TPM_VERSION *version)
{
	TCPA_CAPABILITY_AREA capArea = TPM_CAP_VERSION_VAL;
	UINT32 respSize;
	BYTE *resp;
	TSS_RESULT result;
	UINT64 offset;

	/* try the 1.2 way first */
	result = TCSP_GetCapability_Internal(InternalContext, capArea, 0, NULL, &respSize, &resp);
	if (result == TSS_SUCCESS) {
		offset = sizeof(UINT16); // XXX hack
		UnloadBlob_VERSION(&offset, resp, version);
		free(resp);
	} else if (result == TCPA_E_BAD_MODE) {
		/* if the TPM doesn't understand VERSION_VAL, try the 1.1 way */
		capArea = TCPA_CAP_VERSION;
		result = TCSP_GetCapability_Internal(InternalContext, capArea, 0, NULL, &respSize,
						     &resp);
		if (result == TSS_SUCCESS) {
			offset = 0;
			UnloadBlob_VERSION(&offset, resp, version);
			free(resp);
		}
	}

	return result;
}

TSS_RESULT
get_cap_uint32(TCPA_CAPABILITY_AREA capArea, BYTE *subCap, UINT32 subCapSize, UINT32 *v)
{
	UINT32 respSize;
	BYTE *resp;
	TSS_RESULT result;
	UINT64 offset;

	result = TCSP_GetCapability_Internal(InternalContext, capArea, subCapSize, subCap,
					     &respSize, &resp);
	if (!result) {
		offset = 0;
		switch (respSize) {
			case 1:
				UnloadBlob_BYTE(&offset, (BYTE *)v, resp);
				break;
			case sizeof(UINT16):
				UnloadBlob_UINT16(&offset, (UINT16 *)v, resp);
				break;
			case sizeof(UINT32):
				UnloadBlob_UINT32(&offset, v, resp);
				break;
			default:
				LogDebug("TCSP_GetCapability_Internal returned"
					  " %u bytes", respSize);
				result = TCSERR(TSS_E_FAIL);
				break;
		}
		free(resp);
	}

	return result;
}


TSS_RESULT
get_max_auths(UINT32 *auths)
{
	TCS_AUTHHANDLE handles[TSS_MAX_AUTHS_CAP];
	TCPA_NONCE nonce;
	UINT32 subCap;
	TSS_RESULT result;
	int i;

	if (TPM_VERSION_IS(1,2)) {
		UINT32ToArray(TPM_CAP_PROP_MAX_AUTHSESS, (BYTE *)(&subCap));
		result = get_cap_uint32(TPM_CAP_PROPERTY, (BYTE *)&subCap, sizeof(subCap), auths);
	} else if (TPM_VERSION_IS(1,1)) {
		/* open auth sessions until we get a failure */
		for (i = 0; i < TSS_MAX_AUTHS_CAP; i++) {
			result = TCSP_OIAP_Internal(InternalContext, &(handles[i]), &nonce);
			if (result != TSS_SUCCESS) {
				/* this is not off by one since we're 0 indexed */
				*auths = i;
				break;
			}
		}

		if (i == TSS_MAX_AUTHS_CAP)
			*auths = TSS_MAX_AUTHS_CAP;

		/* close the auth sessions */
		for (i = 0; (UINT32)i < *auths; i++) {
			internal_TerminateHandle(handles[i]);
		}
	} else {
		result = TCSERR(TSS_E_INTERNAL_ERROR);
		*auths = 0;
	}

	if (*auths < 2) {
		LogError("%s reported only %u auth available!", __FUNCTION__, *auths);
		LogError("Your TPM must be reset before the TCSD can be started.");
	} else {
		LogDebug("get_max_auths reports %u auth contexts found", *auths);
		result = TSS_SUCCESS;
	}

	return result;
}

/* This is only called from init paths, so printing an error message is
 * appropriate if something goes wrong */
TSS_RESULT
get_tpm_metrics(struct tpm_properties *p)
{
	TSS_RESULT result;
	UINT32 subCap, rv = 0;

	if ((result = get_current_version(&p->version)))
		goto err;

	UINT32ToArray(TPM_ORD_SaveKeyContext, (BYTE *)&subCap);
	if ((result = get_cap_uint32(TCPA_CAP_ORD, (BYTE *)&subCap, sizeof(UINT32), &rv)))
		goto err;
	p->keyctx_swap = rv ? TRUE : FALSE;

	rv = 0;
	UINT32ToArray(TPM_ORD_SaveAuthContext, (BYTE *)&subCap);
	if ((result = get_cap_uint32(TCPA_CAP_ORD, (BYTE *)&subCap, sizeof(UINT32), &rv)))
		goto err;
	p->authctx_swap = rv ? TRUE : FALSE;

	UINT32ToArray(TPM_CAP_PROP_PCR, (BYTE *)&subCap);
	if ((result = get_cap_uint32(TCPA_CAP_PROPERTY, (BYTE *)&subCap, sizeof(UINT32),
					&p->num_pcrs)))
		goto err;

	UINT32ToArray(TPM_CAP_PROP_DIR, (BYTE *)&subCap);
	if ((result = get_cap_uint32(TCPA_CAP_PROPERTY, (BYTE *)&subCap, sizeof(UINT32),
					&p->num_dirs)))
		goto err;

	UINT32ToArray(TPM_CAP_PROP_SLOTS, (BYTE *)&subCap);
	if ((result = get_cap_uint32(TCPA_CAP_PROPERTY, (BYTE *)&subCap, sizeof(UINT32),
					&p->num_keys)))
		goto err;

	UINT32ToArray(TPM_CAP_PROP_MANUFACTURER, (BYTE *)&subCap);
	if ((result = get_cap_uint32(TCPA_CAP_PROPERTY, (BYTE *)&subCap, sizeof(UINT32),
					(UINT32 *)&p->manufacturer)))
		goto err;

	result = get_max_auths(&(p->num_auths));

err:
	if (result)
		LogError("TCS GetCapability failed with result = 0x%x", result);

	return result;
}

