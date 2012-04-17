
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
#include <inttypes.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
Tspi_TPM_TakeOwnership(TSS_HTPM hTPM,			/* in */
		       TSS_HKEY hKeySRK,		/* in */
		       TSS_HKEY hEndorsementPubKey)	/* in */
{
	TPM_AUTH privAuth;
	BYTE encOwnerAuth[256];
	UINT32 encOwnerAuthLength;
	BYTE encSRKAuth[256];
	UINT32 encSRKAuthLength;
	TCPA_DIGEST digest;
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;
	UINT32 srkKeyBlobLength;
	BYTE *srkKeyBlob;
	TSS_HPOLICY hOwnerPolicy;
	UINT32 newSrkBlobSize;
	BYTE *newSrkBlob = NULL;
	BYTE oldAuthDataUsage;
	TSS_HKEY hPubEK;
	Trspi_HashCtx hashCtx;


	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (hEndorsementPubKey == NULL_HKEY) {
		if ((result = Tspi_TPM_GetPubEndorsementKey(hTPM, FALSE, NULL, &hPubEK))) {
			return result;
		}
	} else {
		hPubEK = hEndorsementPubKey;
	}

	/* Get the srkKeyData */
	if ((result = obj_rsakey_get_blob(hKeySRK, &srkKeyBlobLength, &srkKeyBlob)))
		return result;

	/* Need to check for Atmel bug where authDataUsage is changed */
	oldAuthDataUsage = srkKeyBlob[10];
	LogDebug("oldAuthDataUsage is %.2X.  Wait to see if it changes", oldAuthDataUsage);

	/* Now call the module that will encrypt the secrets.  This
	 * will either get the secrets from the policy objects or
	 * use the callback function to encrypt the secrets */

	if ((result = secret_TakeOwnership(hPubEK, hTPM, hKeySRK, &privAuth, &encOwnerAuthLength,
					   encOwnerAuth, &encSRKAuthLength, encSRKAuth)))
		return result;

	/* Now, take ownership is ready to call.  The auth structure should be complete
	 * and the encrypted data structures should be ready */
	if ((result = RPC_TakeOwnership(tspContext, TPM_PID_OWNER, encOwnerAuthLength, encOwnerAuth,
					encSRKAuthLength, encSRKAuth, srkKeyBlobLength, srkKeyBlob,
					&privAuth, &newSrkBlobSize, &newSrkBlob)))
		return result;

	/* The final step is to validate the return Auth */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_TakeOwnership);
	result |= Trspi_HashUpdate(&hashCtx, newSrkBlobSize, newSrkBlob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hOwnerPolicy))) {
		free(newSrkBlob);
		return result;
	}
	if ((result = obj_policy_validate_auth_oiap(hOwnerPolicy, &digest, &privAuth))) {
		free(newSrkBlob);
		return result;
	}

	/* Now that it's all happy, stuff the keyBlob into the object
	 * If atmel, need to adjust the authDataUsage if it changed */
	if (oldAuthDataUsage != newSrkBlob[10]) {	/* hardcoded blob stuff */
		LogDebug("auth data usage changed. Atmel bug. Fixing in key object");
		newSrkBlob[10] = oldAuthDataUsage;	/* this will fix it  */
	}

	result = obj_rsakey_set_tcpakey(hKeySRK, newSrkBlobSize, newSrkBlob);
	free(newSrkBlob);

	if (result)
		return result;

	/* The SRK is loaded at this point, so insert it into the key handle list */
	return obj_rsakey_set_tcs_handle(hKeySRK, TPM_KEYHND_SRK);
}

TSS_RESULT
Tspi_TPM_ClearOwner(TSS_HTPM hTPM,		/* in */
		    TSS_BOOL fForcedClear)	/* in */
{
	TCPA_RESULT result;
	TPM_AUTH auth;
	TSS_HCONTEXT tspContext;
	TCPA_DIGEST hashDigest;
	TSS_HPOLICY hPolicy;
	Trspi_HashCtx hashCtx;

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	if (!fForcedClear) {	/*  TPM_OwnerClear */
		if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hPolicy)))
			return result;

		/* Now do some Hash'ing */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_OwnerClear);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		/* hashDigest now has the hash result */
		if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_OwnerClear, hPolicy, FALSE,
						      &hashDigest, &auth)))
			return result;

		if ((result = TCS_API(tspContext)->OwnerClear(tspContext, &auth)))
			return result;

		/* validate auth */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, result);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_OwnerClear);
		if ((result |= Trspi_HashFinal(&hashCtx, hashDigest.digest)))
			return result;

		if ((result = obj_policy_validate_auth_oiap(hPolicy, &hashDigest, &auth)))
			return result;
	} else {
		if ((result = TCS_API(tspContext)->ForceClear(tspContext)))
			return result;
	}

	return TSS_SUCCESS;
}
