
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
secret_TakeOwnership(TSS_HKEY hEndorsementPubKey,
		     TSS_HTPM hTPM,
		     TSS_HKEY hKeySRK,
		     TPM_AUTH * auth,
		     UINT32 * encOwnerAuthLength,
		     BYTE * encOwnerAuth, UINT32 * encSRKAuthLength, BYTE * encSRKAuth)
{
	TSS_RESULT result;
	UINT32 endorsementKeySize;
	BYTE *endorsementKey;
	TSS_KEY dummyKey;
	UINT64 offset;
	TCPA_SECRET ownerSecret;
	TCPA_SECRET srkSecret;
	TCPA_DIGEST digest;
	TSS_HPOLICY hSrkPolicy;
	TSS_HPOLICY hOwnerPolicy;
	UINT32 srkKeyBlobLength;
	BYTE *srkKeyBlob;
	TSS_HCONTEXT tspContext;
	UINT32 ownerMode, srkMode;
	Trspi_HashCtx hashCtx;


	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	/*************************************************
	 *	First, get the policy objects and check them for how
	 *		to handle the secrets.  If they cannot be found
	 *		or there is an error, then we must fail
	 **************************************************/

	/* First get the Owner Policy */
	if ((result = obj_tpm_get_policy(hTPM, TSS_POLICY_USAGE, &hOwnerPolicy)))
		return result;

	/* Now get the SRK Policy */
	if ((result = obj_rsakey_get_policy(hKeySRK, TSS_POLICY_USAGE, &hSrkPolicy, NULL)))
		return result;

	if ((result = obj_policy_get_mode(hOwnerPolicy, &ownerMode)))
		return result;

	if ((result = obj_policy_get_mode(hSrkPolicy, &srkMode)))
		return result;

	/* If the policy callback's aren't the same, that's an error if one is callback */
	if (srkMode == TSS_SECRET_MODE_CALLBACK || ownerMode == TSS_SECRET_MODE_CALLBACK) {
		if (srkMode != TSS_SECRET_MODE_CALLBACK || ownerMode != TSS_SECRET_MODE_CALLBACK) {
			LogError("Policy callback modes for SRK policy and Owner policy differ.");
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
	}

	if (ownerMode != TSS_SECRET_MODE_CALLBACK) {
		/* First, get the Endorsement Public Key for Encrypting */
		if ((result = obj_rsakey_get_blob(hEndorsementPubKey, &endorsementKeySize,
						  &endorsementKey)))
			return result;

		/* now stick it in a Key Structure */
		offset = 0;
		if ((result = UnloadBlob_TSS_KEY(&offset, endorsementKey, &dummyKey))) {
			free_tspi(tspContext, endorsementKey);
			return result;
		}
		free_tspi(tspContext, endorsementKey);

		if ((result = obj_policy_get_secret(hOwnerPolicy, TR_SECRET_CTX_NEW,
						    &ownerSecret))) {
			free(dummyKey.pubKey.key);
			free(dummyKey.algorithmParms.parms);
			return result;
		}

		if ((result = obj_policy_get_secret(hSrkPolicy, TR_SECRET_CTX_NEW, &srkSecret))) {
			free(dummyKey.pubKey.key);
			free(dummyKey.algorithmParms.parms);
			return result;
		}

		/* Encrypt the Owner, SRK Authorizations */
		if ((result = Trspi_RSA_Encrypt(ownerSecret.authdata, 20, encOwnerAuth,
						encOwnerAuthLength, dummyKey.pubKey.key,
						dummyKey.pubKey.keyLength))) {
			free(dummyKey.pubKey.key);
			free(dummyKey.algorithmParms.parms);
			return result;
		}

		if ((result = Trspi_RSA_Encrypt(srkSecret.authdata, 20, encSRKAuth,
						encSRKAuthLength, dummyKey.pubKey.key,
						dummyKey.pubKey.keyLength))) {
			free(dummyKey.pubKey.key);
			free(dummyKey.algorithmParms.parms);
			return result;
		}

		free(dummyKey.pubKey.key);
		free(dummyKey.algorithmParms.parms);
	} else {
		*encOwnerAuthLength = 256;
		*encSRKAuthLength = 256;
		if ((result = obj_policy_do_takeowner(hOwnerPolicy, hTPM, hEndorsementPubKey,
						      *encOwnerAuthLength, encOwnerAuth)))
			return result;
	}

	if ((result = obj_rsakey_get_blob(hKeySRK, &srkKeyBlobLength, &srkKeyBlob)))
		return result;

	/* Authorizatin Digest Calculation */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_TakeOwnership);
	result |= Trspi_Hash_UINT16(&hashCtx, TCPA_PID_OWNER);
	result |= Trspi_Hash_UINT32(&hashCtx, *encOwnerAuthLength);
	result |= Trspi_HashUpdate(&hashCtx, *encOwnerAuthLength, encOwnerAuth);
	result |= Trspi_Hash_UINT32(&hashCtx, *encSRKAuthLength);
	result |= Trspi_HashUpdate(&hashCtx, *encSRKAuthLength, encSRKAuth);
	result |= Trspi_HashUpdate(&hashCtx, srkKeyBlobLength, srkKeyBlob);
	free_tspi(tspContext, srkKeyBlob);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	/* HMAC for the final digest */
	if ((result = secret_PerformAuth_OIAP(hTPM, TPM_ORD_TakeOwnership, hOwnerPolicy, FALSE,
					      &digest, auth)))
		return result;

	return TSS_SUCCESS;
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_OwnerClear(TSS_HCONTEXT tspContext,	/* in */
		     TPM_AUTH * ownerAuth)	/* in, out */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	return obj_context_transport_execute(tspContext, TPM_ORD_OwnerClear, 0, NULL, NULL,
					     &handlesLen, NULL, ownerAuth, NULL, NULL, NULL);
}

TSS_RESULT
Transport_ForceClear(TSS_HCONTEXT tspContext)	/* in */
{
	TSS_RESULT result;
	UINT32 handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	return obj_context_transport_execute(tspContext, TPM_ORD_ForceClear, 0, NULL, NULL,
					     &handlesLen, NULL, NULL, NULL, NULL, NULL);
}
#endif

