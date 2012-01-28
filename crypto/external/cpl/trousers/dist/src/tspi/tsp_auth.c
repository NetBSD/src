
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
#include <assert.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "authsess.h"


TSS_RESULT
secret_PerformAuth_OIAP(TSS_HOBJECT hAuthorizedObject,
			UINT32 ulPendingFn,
			TSS_HPOLICY hPolicy,
			TSS_BOOL cas, /* continue auth session */
			TCPA_DIGEST *hashDigest,
			TPM_AUTH *auth)
{
	TSS_RESULT result;
	TSS_BOOL bExpired;
	UINT32 mode;
	TCPA_SECRET secret;
	TSS_HCONTEXT tspContext;
	TSS_RESULT (*OIAP)(TSS_HCONTEXT, TCS_AUTHHANDLE *, TPM_NONCE *); // XXX hack
	TSS_RESULT (*TerminateHandle)(TSS_HCONTEXT, TCS_HANDLE); // XXX hack

	/* This validates that the secret can be used */
	if ((result = obj_policy_has_expired(hPolicy, &bExpired)))
		return result;

	if (bExpired == TRUE)
		return TSPERR(TSS_E_INVALID_OBJ_ACCESS);

	if ((result = obj_policy_get_tsp_context(hPolicy, &tspContext)))
		return result;

	if ((result = obj_policy_get_mode(hPolicy, &mode)))
		return result;

	if ((result = Init_AuthNonce(tspContext, cas, auth)))
		return result;

	/* XXX hack for opening a transport session */
	if (cas) {
		OIAP = RPC_OIAP;
		TerminateHandle = RPC_TerminateHandle;
	} else {
		OIAP = TCS_API(tspContext)->OIAP;
		TerminateHandle = TCS_API(tspContext)->TerminateHandle;
	}

	/* added retry logic */
	if ((result = OIAP(tspContext, &auth->AuthHandle, &auth->NonceEven))) {
		if (result == TCPA_E_RESOURCES) {
			int retry = 0;
			do {
				/* POSIX sleep time, { secs, nanosecs } */
				struct timespec t = { 0, AUTH_RETRY_NANOSECS };

				nanosleep(&t, NULL);

				result = OIAP(tspContext, &auth->AuthHandle, &auth->NonceEven);
			} while (result == TCPA_E_RESOURCES && ++retry < AUTH_RETRY_COUNT);
		}

		if (result)
			return result;
	}

	switch (mode) {
		case TSS_SECRET_MODE_CALLBACK:
			result = obj_policy_do_hmac(hPolicy, hAuthorizedObject,
						    TRUE, ulPendingFn,
						    auth->fContinueAuthSession,
						    20,
						    auth->NonceEven.nonce,
						    auth->NonceOdd.nonce,
						    NULL, NULL, 20,
						    hashDigest->digest,
						    (BYTE *)&auth->HMAC);
			break;
		case TSS_SECRET_MODE_SHA1:
		case TSS_SECRET_MODE_PLAIN:
		case TSS_SECRET_MODE_POPUP:
			if ((result = obj_policy_get_secret(hPolicy, TR_SECRET_CTX_NOT_NEW,
							    &secret)))
				break;

			HMAC_Auth(secret.authdata, hashDigest->digest, auth);
			break;
		case TSS_SECRET_MODE_NONE:
			/* fall through */
		default:
			result = TSPERR(TSS_E_POLICY_NO_SECRET);
			break;
	}

	if (result) {
		TerminateHandle(tspContext, auth->AuthHandle);
		return result;
	}

	return obj_policy_dec_counter(hPolicy);
}
#if 0
TSS_RESULT
secret_PerformXOR_OSAP(TSS_HPOLICY hPolicy, TSS_HPOLICY hUsagePolicy,
		       TSS_HPOLICY hMigrationPolicy, TSS_HOBJECT hOSAPObject,
		       UINT16 osapType, UINT32 osapData,
		       TCPA_ENCAUTH * encAuthUsage, TCPA_ENCAUTH * encAuthMig,
		       BYTE *sharedSecret, TPM_AUTH * auth, TCPA_NONCE * nonceEvenOSAP)
{
	TSS_BOOL bExpired;
	TCPA_SECRET keySecret;
	TCPA_SECRET usageSecret;
	TCPA_SECRET migSecret = { { 0, } };
	UINT32 keyMode, usageMode, migMode = 0;
	TSS_RESULT result;
	TSS_HCONTEXT tspContext;


	if ((result = obj_policy_has_expired(hPolicy, &bExpired)))
		return result;

	if (bExpired == TRUE)
		return TSPERR(TSS_E_INVALID_OBJ_ACCESS);

	if ((result = obj_policy_has_expired(hUsagePolicy, &bExpired)))
		return result;

	if (bExpired == TRUE)
		return TSPERR(TSS_E_INVALID_OBJ_ACCESS);

	if (hMigrationPolicy) {
		if ((result = obj_policy_has_expired(hMigrationPolicy, &bExpired)))
			return result;

		if (bExpired == TRUE)
			return TSPERR(TSS_E_INVALID_OBJ_ACCESS);

		if ((result = obj_policy_get_mode(hMigrationPolicy, &migMode)))
			return result;
	}

	if ((result = obj_policy_get_tsp_context(hPolicy, &tspContext)))
		return result;

	if ((result = obj_policy_get_mode(hPolicy, &keyMode)))
		return result;

	if ((result = obj_policy_get_mode(hUsagePolicy, &usageMode)))
		return result;

	if (keyMode == TSS_SECRET_MODE_CALLBACK ||
	    usageMode == TSS_SECRET_MODE_CALLBACK ||
	    (hMigrationPolicy && migMode == TSS_SECRET_MODE_CALLBACK)) {
		if (keyMode != TSS_SECRET_MODE_CALLBACK ||
		    usageMode != TSS_SECRET_MODE_CALLBACK ||
		    (hMigrationPolicy && migMode != TSS_SECRET_MODE_CALLBACK))
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if (keyMode != TSS_SECRET_MODE_CALLBACK) {
		if ((result = obj_policy_get_secret(hPolicy, TR_SECRET_CTX_NOT_NEW, &keySecret)))
			return result;

		if ((result = obj_policy_get_secret(hUsagePolicy, TR_SECRET_CTX_NEW, &usageSecret)))
			return result;

		if (hMigrationPolicy) {
			if ((result = obj_policy_get_secret(hMigrationPolicy, TR_SECRET_CTX_NEW,
							&migSecret)))
				return result;
		}

		if ((result = OSAP_Calc(tspContext, osapType, osapData,
					keySecret.authdata, usageSecret.authdata,
					migSecret.authdata, encAuthUsage,
					encAuthMig, sharedSecret, auth)))
			return result;
	} else {
		/* If the secret mode is NONE here, we don't return an error. This is
		 * because there are commands such as CreateKey, which require an auth
		 * session even when creating no-auth keys. A secret of all 0's will be
		 * used in this case. */
		if ((result = TCS_API(tspContext)->OSAP(tspContext, osapType, osapData,
							&auth->NonceOdd, &auth->AuthHandle,
							&auth->NonceEven, nonceEvenOSAP)))
			return result;

		if ((result = obj_policy_do_xor(hPolicy, hOSAPObject,
						hPolicy, TRUE, 20,
						auth->NonceEven.nonce, NULL,
						nonceEvenOSAP->nonce,
						auth->NonceOdd.nonce, 20,
						encAuthUsage->authdata,
						encAuthMig->authdata))) {
			TCS_API(tspContext)->TerminateHandle(tspContext, auth->AuthHandle);
			return result;
		}
	}

	return TSS_SUCCESS;
}

TSS_RESULT
secret_PerformAuth_OSAP(TSS_HOBJECT hAuthorizedObject, UINT32 ulPendingFn,
			TSS_HPOLICY hPolicy, TSS_HPOLICY hUsagePolicy,
			TSS_HPOLICY hMigPolicy, BYTE sharedSecret[20],
			TPM_AUTH *auth, BYTE *hashDigest,
			TCPA_NONCE *nonceEvenOSAP)
{
	TSS_RESULT result;
	UINT32 keyMode, usageMode, migMode = 0;

	if ((result = obj_policy_get_mode(hPolicy, &keyMode)))
		return result;

	if ((result = obj_policy_get_mode(hUsagePolicy, &usageMode)))
		return result;

	if (hMigPolicy) {
		if ((result = obj_policy_get_mode(hMigPolicy, &migMode)))
			return result;
	}

	/* ---  If any of them is a callback */
	if (keyMode == TSS_SECRET_MODE_CALLBACK ||
	    usageMode == TSS_SECRET_MODE_CALLBACK ||
	    (hMigPolicy && migMode == TSS_SECRET_MODE_CALLBACK)) {
		/* ---  And they're not all callback */
		if (keyMode != TSS_SECRET_MODE_CALLBACK ||
		    usageMode != TSS_SECRET_MODE_CALLBACK ||
		    (hMigPolicy && migMode != TSS_SECRET_MODE_CALLBACK))
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if (keyMode == TSS_SECRET_MODE_CALLBACK) {
		if ((result = obj_policy_do_hmac(hPolicy, hAuthorizedObject,
						 TRUE, ulPendingFn,
						 auth->fContinueAuthSession,
						 20,
						 auth->NonceEven.nonce,
						 NULL,
						 nonceEvenOSAP->nonce,
						 auth->NonceOdd.nonce, 20,
						 hashDigest,
						 (BYTE *)&auth->HMAC)))
			return result;
	} else {
		HMAC_Auth(sharedSecret, hashDigest, auth);
	}

	if ((result = obj_policy_dec_counter(hPolicy)))
		return result;

	if ((result = obj_policy_dec_counter(hUsagePolicy)))
		return result;

	if (hMigPolicy) {
		if ((result = obj_policy_dec_counter(hMigPolicy)))
			return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
secret_ValidateAuth_OSAP(TSS_HOBJECT hAuthorizedObject, UINT32 ulPendingFn,
			 TSS_HPOLICY hPolicy, TSS_HPOLICY hUsagePolicy,
			 TSS_HPOLICY hMigPolicy, BYTE sharedSecret[20],
			 TPM_AUTH *auth, BYTE *hashDigest,
			 TCPA_NONCE *nonceEvenOSAP)
{
	TSS_RESULT result;
	UINT32 keyMode, usageMode, migMode = 0;

	if ((result = obj_policy_get_mode(hPolicy, &keyMode)))
		return result;

	if ((result = obj_policy_get_mode(hUsagePolicy, &usageMode)))
		return result;

	if (hMigPolicy) {
		if ((result = obj_policy_get_mode(hMigPolicy, &migMode)))
			return result;
	}

	/* ---  If any of them is a callback */
	if (keyMode == TSS_SECRET_MODE_CALLBACK ||
	    usageMode == TSS_SECRET_MODE_CALLBACK ||
	    (hMigPolicy && migMode == TSS_SECRET_MODE_CALLBACK)) {
		/* ---  And they're not all callback */
		if (keyMode != TSS_SECRET_MODE_CALLBACK ||
		    usageMode != TSS_SECRET_MODE_CALLBACK ||
		    (hMigPolicy && migMode != TSS_SECRET_MODE_CALLBACK))
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	if (keyMode != TSS_SECRET_MODE_CALLBACK) {
		if (validateReturnAuth(sharedSecret, hashDigest, auth))
			return TSPERR(TSS_E_TSP_AUTHFAIL);
	} else {
		if ((result = obj_policy_do_hmac(hPolicy, hAuthorizedObject,
						 FALSE, ulPendingFn,
						 auth->fContinueAuthSession,
						 20,
						 auth->NonceEven.nonce,
						 NULL,
						 nonceEvenOSAP->nonce,
						 auth->NonceOdd.nonce, 20,
						 hashDigest,
						 (BYTE *)&auth->HMAC)))
			return result;
	}

	return TSS_SUCCESS;
}
#endif
TSS_RESULT
Init_AuthNonce(TSS_HCONTEXT tspContext, TSS_BOOL cas, TPM_AUTH * auth)
{
	TSS_RESULT result;

	auth->fContinueAuthSession = cas;
	if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
				       (BYTE **)auth->NonceOdd.nonce))) {
		LogError("Failed creating random nonce");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_BOOL
validateReturnAuth(BYTE *secret, BYTE *hash, TPM_AUTH *auth)
{
	BYTE digest[20];
	/* auth is expected to have both nonces and the digest from the TPM */
	memcpy(digest, &auth->HMAC, 20);
	HMAC_Auth(secret, hash, auth);

	return ((TSS_BOOL) memcmp(digest, &auth->HMAC, 20) != 0);
}

void
HMAC_Auth(BYTE * secret, BYTE * Digest, TPM_AUTH * auth)
{
	UINT64 offset;
	BYTE Blob[61];

	offset = 0;
	Trspi_LoadBlob(&offset, 20, Blob, Digest);
	Trspi_LoadBlob(&offset, 20, Blob, auth->NonceEven.nonce);
	Trspi_LoadBlob(&offset, 20, Blob, auth->NonceOdd.nonce);
	Blob[offset++] = auth->fContinueAuthSession;

	Trspi_HMAC(TSS_HASH_SHA1, 20, secret, offset, Blob, (BYTE *)&auth->HMAC);
}

TSS_RESULT
OSAP_Calc(TSS_HCONTEXT tspContext, UINT16 EntityType, UINT32 EntityValue,
	  BYTE * authSecret, BYTE * usageSecret, BYTE * migSecret,
	  TCPA_ENCAUTH * encAuthUsage, TCPA_ENCAUTH * encAuthMig,
	  BYTE * sharedSecret, TPM_AUTH * auth)
{
	TSS_RESULT rc;
	TCPA_NONCE nonceEvenOSAP;
	UINT64 offset;
	BYTE hmacBlob[0x200];
	BYTE hashBlob[0x200];
	BYTE xorUsageAuth[20];
	BYTE xorMigAuth[20];
	UINT32 i;

	if ((rc = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
				   (BYTE **)auth->NonceOdd.nonce))) {
		LogError("Failed creating random nonce");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	auth->fContinueAuthSession = 0x00;

	if ((rc = TCS_API(tspContext)->OSAP(tspContext, EntityType, EntityValue, &auth->NonceOdd,
					    &auth->AuthHandle, &auth->NonceEven, &nonceEvenOSAP))) {
		if (rc == TCPA_E_RESOURCES) {
			int retry = 0;
			do {
				/* POSIX sleep time, { secs, nanosecs } */
				struct timespec t = { 0, AUTH_RETRY_NANOSECS };

				nanosleep(&t, NULL);

				rc = TCS_API(tspContext)->OSAP(tspContext, EntityType, EntityValue,
							       &auth->NonceOdd, &auth->AuthHandle,
							       &auth->NonceEven, &nonceEvenOSAP);
			} while (rc == TCPA_E_RESOURCES && ++retry < AUTH_RETRY_COUNT);
		}

		if (rc)
			return rc;
	}

	offset = 0;
	Trspi_LoadBlob(&offset, 20, hmacBlob, nonceEvenOSAP.nonce);
	Trspi_LoadBlob(&offset, 20, hmacBlob, auth->NonceOdd.nonce);

	Trspi_HMAC(TSS_HASH_SHA1, 20, authSecret, offset, hmacBlob, sharedSecret);

	offset = 0;
	Trspi_LoadBlob(&offset, 20, hashBlob, sharedSecret);
	Trspi_LoadBlob(&offset, 20, hashBlob, auth->NonceEven.nonce);

	if ((rc = Trspi_Hash(TSS_HASH_SHA1, offset, hashBlob, xorUsageAuth)))
		return rc;

	offset = 0;
	Trspi_LoadBlob(&offset, 20, hashBlob, sharedSecret);
	Trspi_LoadBlob(&offset, 20, hashBlob, auth->NonceOdd.nonce);
	if ((rc = Trspi_Hash(TSS_HASH_SHA1, offset, hashBlob, xorMigAuth)))
		return rc;

	for (i = 0; i < sizeof(TCPA_ENCAUTH); i++)
		encAuthUsage->authdata[i] = usageSecret[i] ^ xorUsageAuth[i];
	for (i = 0; i < sizeof(TCPA_ENCAUTH); i++)
		encAuthMig->authdata[i] = migSecret[i] ^ xorMigAuth[i];

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_validate_auth_oiap(TSS_HPOLICY hPolicy, TCPA_DIGEST *hashDigest, TPM_AUTH *auth)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	BYTE wellKnown[TCPA_SHA1_160_HASH_LEN] = TSS_WELL_KNOWN_SECRET;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	switch (policy->SecretMode) {
		case TSS_SECRET_MODE_CALLBACK:
			result = policy->Tspicb_CallbackHMACAuth(
					policy->hmacAppData,
					hPolicy,
					0,
					auth->fContinueAuthSession,
					FALSE,
					20,
					auth->NonceEven.nonce,
					auth->NonceOdd.nonce,
					NULL, NULL, 20,
					hashDigest->digest,
					(BYTE *)&auth->HMAC);
			break;
		case TSS_SECRET_MODE_SHA1:
		case TSS_SECRET_MODE_PLAIN:
		case TSS_SECRET_MODE_POPUP:
			if (validateReturnAuth(policy->Secret, hashDigest->digest, auth))
				result = TSPERR(TSS_E_TSP_AUTHFAIL);
			break;
		case TSS_SECRET_MODE_NONE:
			if (validateReturnAuth(wellKnown, hashDigest->digest, auth))
				result = TSPERR(TSS_E_TSP_AUTHFAIL);
			break;
		default:
			result = TSPERR(TSS_E_POLICY_NO_SECRET);
			break;
	}

	obj_list_put(&policy_list);

	return result;
}

#if 0
TSS_RESULT
authsess_oiap_get(TSS_HOBJECT obj, TPM_COMMAND_CODE ord, TPM_DIGEST *digest, TPM_AUTH *auth)
{
	TSS_RESULT result = TSS_SUCCESS;
	TSS_BOOL bExpired;
	UINT32 mode;
	TPM_SECRET secret;
	TSS_HCONTEXT tspContext;
	TSS_RESULT (*OIAP)(TSS_HCONTEXT, TCS_AUTHHANDLE *, TPM_NONCE *); // XXX hack
	TSS_RESULT (*TerminateHandle)(TSS_HCONTEXT, TCS_HANDLE); // XXX hack


	if (obj_is_tpm(obj))
		result = obj_tpm_get_tsp_context(obj, hContext);
	else if (obj_is_rsakey(obj))
		result = obj_rsakey_get_tsp_context(obj, hContext);
	else if (obj_is_encdata(obj))
		result = obj_encdata_get_tsp_context(obj, hContext);
	else if (obj_is_nvstore(obj))
		result = obj_nvstore_get_tsp_context(obj, hContext);
	else
		result = TSPERR(TSS_E_INVALID_HANDLE);

#if 0
	/* This validates that the secret can be used */
	if ((result = obj_policy_has_expired(hPolicy, &bExpired)))
		return result;

	if (bExpired == TRUE)
		return TSPERR(TSS_E_INVALID_OBJ_ACCESS);

	if ((result = obj_policy_get_tsp_context(hPolicy, &tspContext)))
		return result;

	if ((result = obj_policy_get_mode(hPolicy, &mode)))
		return result;
#else
	if ((result = obj_policy_get_authsess_params()))
		return result;
#endif
	if ((result = Init_AuthNonce(tspContext, cas, auth)))
		return result;

	/* XXX hack for opening a transport session */
	if (cas) {
		OIAP = RPC_OIAP;
		TerminateHandle = RPC_TerminateHandle;
	} else {
		OIAP = TCS_API(tspContext)->OIAP;
		TerminateHandle = TCS_API(tspContext)->TerminateHandle;
	}

	/* added retry logic */
	if ((result = OIAP(tspContext, &auth->AuthHandle, &auth->NonceEven))) {
		if (result == TCPA_E_RESOURCES) {
			int retry = 0;
			do {
				/* POSIX sleep time, { secs, nanosecs } */
				struct timespec t = { 0, AUTH_RETRY_NANOSECS };

				nanosleep(&t, NULL);

				result = OIAP(tspContext, &auth->AuthHandle, &auth->NonceEven);
			} while (result == TCPA_E_RESOURCES && ++retry < AUTH_RETRY_COUNT);
		}

		if (result)
			return result;
	}

	switch (mode) {
		case TSS_SECRET_MODE_CALLBACK:
			result = obj_policy_do_hmac(hPolicy, hAuthorizedObject,
						    TRUE, ulPendingFn,
						    auth->fContinueAuthSession,
						    20,
						    auth->NonceEven.nonce,
						    auth->NonceOdd.nonce,
						    NULL, NULL, 20,
						    hashDigest->digest,
						    (BYTE *)&auth->HMAC);
			break;
		case TSS_SECRET_MODE_SHA1:
		case TSS_SECRET_MODE_PLAIN:
		case TSS_SECRET_MODE_POPUP:
			if ((result = obj_policy_get_secret(hPolicy, TR_SECRET_CTX_NOT_NEW,
							    &secret)))
				break;

			HMAC_Auth(secret.authdata, hashDigest->digest, auth);
			break;
		case TSS_SECRET_MODE_NONE:
			/* fall through */
		default:
			result = TSPERR(TSS_E_POLICY_NO_SECRET);
			break;
	}

	if (result) {
		TerminateHandle(tspContext, auth->AuthHandle);
		return result;
	}

	return obj_policy_dec_counter(hPolicy);
}

TSS_RESULT
authsess_oiap_put(TPM_AUTH *auth)
{
}
#endif

#ifdef TSS_BUILD_DELEGATION
TSS_RESULT
authsess_do_dsap(struct authsess *sess)
{
	TSS_RESULT result;

	if ((result = TCS_API(sess->tspContext)->DSAP(sess->tspContext, sess->entity_type,
						      sess->obj_parent, &sess->nonceOddxSAP,
						      sess->entityValueSize, sess->entityValue,
						      &sess->pAuth->AuthHandle,
						      &sess->pAuth->NonceEven,
						      &sess->nonceEvenxSAP))) {
		if (result == TCPA_E_RESOURCES) {
			int retry = 0;
			do {
				/* POSIX sleep time, { secs, nanosecs } */
				struct timespec t = { 0, AUTH_RETRY_NANOSECS };

				nanosleep(&t, NULL);

				result = TCS_API(sess->tspContext)->DSAP(sess->tspContext,
									 sess->entity_type,
									 sess->obj_parent,
									 &sess->nonceOddxSAP,
									 sess->entityValueSize,
									 sess->entityValue,
									 &sess->pAuth->AuthHandle,
									 &sess->pAuth->NonceEven,
									 &sess->nonceEvenxSAP);
			} while (result == TCPA_E_RESOURCES && ++retry < AUTH_RETRY_COUNT);
		}
	}

	return result;
}
#endif

TSS_RESULT
authsess_do_osap(struct authsess *sess)
{
	TSS_RESULT result;

	if ((result = TCS_API(sess->tspContext)->OSAP(sess->tspContext, sess->entity_type,
						      sess->obj_parent, &sess->nonceOddxSAP,
						      &sess->pAuth->AuthHandle,
						      &sess->pAuth->NonceEven,
						      &sess->nonceEvenxSAP))) {
		if (result == TCPA_E_RESOURCES) {
			int retry = 0;
			do {
				/* POSIX sleep time, { secs, nanosecs } */
				struct timespec t = { 0, AUTH_RETRY_NANOSECS };

				nanosleep(&t, NULL);

				result = TCS_API(sess->tspContext)->OSAP(sess->tspContext,
									 sess->entity_type,
									 sess->obj_parent,
									 &sess->nonceOddxSAP,
									 &sess->pAuth->AuthHandle,
									 &sess->pAuth->NonceEven,
									 &sess->nonceEvenxSAP);
			} while (result == TCPA_E_RESOURCES && ++retry < AUTH_RETRY_COUNT);
		}
	}

	return result;
}

TSS_RESULT
authsess_callback_xor(PVOID lpAppData,
		      TSS_HOBJECT hOSAPObject,
		      TSS_HOBJECT hObject,
		      TSS_FLAG PurposeSecret,
		      UINT32 ulSizeNonces,
		      BYTE *rgbNonceEven,
		      BYTE *rgbNonceOdd,
		      BYTE *rgbNonceEvenOSAP,
		      BYTE *rgbNonceOddOSAP,
		      UINT32 ulSizeEncAuth,
		      BYTE *rgbEncAuthUsage,
		      BYTE *rgbEncAuthMigration)
{
	TSS_RESULT result;
	BYTE xorUseAuth[sizeof(TPM_DIGEST)];
	BYTE xorMigAuth[sizeof(TPM_DIGEST)];
	Trspi_HashCtx hashCtx;
	UINT32 i;
	struct authsess *sess = (struct authsess *)lpAppData;

	/* sess->sharedSecret was calculated in authsess_xsap_init */

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_SECRET(&hashCtx, sess->sharedSecret.digest);
	result |= Trspi_Hash_NONCE(&hashCtx, rgbNonceEven);
	if ((result |= Trspi_HashFinal(&hashCtx, xorUseAuth)))
		return result;

	for (i = 0; i < ulSizeEncAuth; i++)
		rgbEncAuthUsage[i] ^= xorUseAuth[i];

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_SECRET(&hashCtx, sess->sharedSecret.digest);
	result |= Trspi_Hash_NONCE(&hashCtx, rgbNonceOdd);
	if ((result |= Trspi_HashFinal(&hashCtx, xorMigAuth)))
		return result;

	for (i = 0; i < ulSizeEncAuth; i++)
		rgbEncAuthMigration[i] ^= xorMigAuth[i];

	return TSS_SUCCESS;
}

TSS_RESULT
authsess_callback_hmac(PVOID lpAppData,
		       TSS_HOBJECT hAuthorizedObject,
		       TSS_BOOL ReturnOrVerify,
		       UINT32 ulPendingFunction,
		       TSS_BOOL ContinueUse,
		       UINT32 ulSizeNonces,
		       BYTE *rgbNonceEven,
		       BYTE *rgbNonceOdd,
		       BYTE *rgbNonceEvenOSAP,
		       BYTE *rgbNonceOddOSAP,
		       UINT32 ulSizeDigestHmac,
		       BYTE *rgbParamDigest,
		       BYTE *rgbHmacData)
{
	struct authsess *sess = (struct authsess *)lpAppData;
	TSS_RESULT result = TSS_SUCCESS;
	UINT64 offset;
	BYTE Blob[61];

	offset = 0;
	Trspi_LoadBlob(&offset, ulSizeDigestHmac, Blob, rgbParamDigest);
	Trspi_LoadBlob(&offset, ulSizeNonces, Blob, rgbNonceEven);
	Trspi_LoadBlob(&offset, ulSizeNonces, Blob, rgbNonceOdd);
	Blob[offset++] = ContinueUse;

	if (ReturnOrVerify) {
		Trspi_HMAC(TSS_HASH_SHA1, ulSizeDigestHmac, sess->sharedSecret.digest, offset, Blob,
			   rgbHmacData);
	} else {
		TPM_HMAC hmacVerify;

		Trspi_HMAC(TSS_HASH_SHA1, ulSizeDigestHmac, sess->sharedSecret.digest, offset, Blob,
			   hmacVerify.digest);
		result = memcmp(rgbHmacData, hmacVerify.digest, ulSizeDigestHmac);
		if (result)
			result = TPM_E_AUTHFAIL;
	}

	return result;
}

/* Create an OSAP session. @requirements is used in different ways depending on the command to
 * indicate whether we should require a policy or auth value */
TSS_RESULT
authsess_xsap_init(TSS_HCONTEXT     tspContext,
		   TSS_HOBJECT	    obj_parent,
		   TSS_HOBJECT      obj_child,
		   TSS_BOOL	    requirements,
		   TPM_COMMAND_CODE command,
		   TPM_ENTITY_TYPE  entity_type,
		   struct authsess  **xsess)
{
	TSS_RESULT result;
	TSS_BOOL authdatausage = FALSE, req_auth = TRUE, get_child_auth = TRUE, secret_set = FALSE;
	BYTE hmacBlob[2 * sizeof(TPM_DIGEST)];
	UINT64 offset;
	TSS_BOOL new_secret = TR_SECRET_CTX_NOT_NEW;
	struct authsess *sess;

	if ((sess = calloc(1, sizeof(struct authsess))) == NULL) {
		LogError("malloc of %zd bytes failed", sizeof(struct authsess));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	switch (command) {
	/* Parent is Key for the cases below */
	case TPM_ORD_Delegate_CreateKeyDelegation:
	case TPM_ORD_CreateWrapKey:
	case TPM_ORD_CMK_CreateKey:
	case TPM_ORD_Seal:
	case TPM_ORD_Sealx:
	case TPM_ORD_Unseal:
	case TPM_ORD_ChangeAuth:
		if ((result = obj_rsakey_get_policy(obj_parent, TSS_POLICY_USAGE,
						    &sess->hUsageParent, NULL)))
			goto error;
		break;
	/* Parent is TPM for the cases below */
	case TPM_ORD_Delegate_CreateOwnerDelegation:
		req_auth = FALSE;
		/* fall through */
	case TPM_ORD_MakeIdentity:
	case TPM_ORD_NV_DefineSpace:
		if ((result = obj_tpm_get_policy(obj_parent, TSS_POLICY_USAGE,
						 &sess->hUsageParent)))
			goto error;
		break;
	case TPM_ORD_ChangeAuthOwner:
		/* Special case, ChangeAuthOwner is used to change Owner and SRK auth */
		if (obj_is_rsakey(obj_parent)) {
			if ((result = obj_rsakey_get_policy(obj_parent, TSS_POLICY_USAGE,
							    &sess->hUsageParent, NULL)))
				goto error;
		} else {
			if ((result = obj_tpm_get_policy(obj_parent, TSS_POLICY_USAGE,
							 &sess->hUsageParent)))
				goto error;
		}
		break;
	default:
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto error;
	}

	if (requirements && !sess->hUsageParent) {
		result = TSPERR(TSS_E_TSP_AUTHREQUIRED);
		goto error;
	}

	if (sess->hUsageParent) {
		/* These are trousers callback functions which will be used to process the auth
		 * session. If the policy type is callback for hUsageParent, they will be
		 * overwritten by the application defined callback functions in the policy */
		sess->cb_xor.callback = authsess_callback_xor;
		sess->cb_xor.appData = (PVOID)sess;
		sess->cb_hmac.callback = authsess_callback_hmac;
		sess->cb_hmac.appData = (PVOID)sess;

		/* XXX the parent object doesn't always hold the callbacks */
		if ((result = obj_policy_get_xsap_params(sess->hUsageParent, command,
							 &sess->entity_type, &sess->entityValueSize,
							 &sess->entityValue,
							 sess->parentSecret.authdata, &sess->cb_xor,
							 &sess->cb_hmac, NULL, &sess->parentMode, 
							 new_secret)))
			goto error;
	} else
		sess->parentMode = TSS_SECRET_MODE_NONE;

	switch (command) {
	/* Child is a Key object */
	case TPM_ORD_CreateWrapKey:
	case TPM_ORD_CMK_CreateKey:
		if ((result = obj_rsakey_get_policies(obj_child, &sess->hUsageChild,
						      &sess->hMigChild, &authdatausage)))
			goto error;

		if (authdatausage && !sess->hUsageChild) {
			result = TSPERR(TSS_E_TSP_AUTHREQUIRED);
			goto error;
		}

		if (obj_rsakey_is_migratable(obj_child)) {
			if (!sess->hMigChild) {
				result = TSPERR(TSS_E_KEY_NO_MIGRATION_POLICY);
				goto error;
			}

			if ((result = obj_policy_get_xsap_params(sess->hMigChild, 0, NULL, NULL,
								 NULL, sess->encAuthMig.authdata,
								 NULL, NULL, NULL, &sess->mMode,
								 new_secret)))
				goto error;
		}

		if ((result = obj_rsakey_get_tcs_handle(obj_parent, &sess->obj_parent)))
			goto error;
		break;
	/* Child is an Encdata object */
	case TPM_ORD_Unseal:
#ifdef TSS_BUILD_SEALX
	case TPM_ORD_Sealx:
		/* These may be overwritten down below, when obj_policy_get_xsap_params is called
		 * on the child usage policy */
		sess->cb_sealx.callback = sealx_mask_cb;
		sess->cb_sealx.appData = (PVOID)sess;
		/* fall through */
#endif
	case TPM_ORD_Seal:
		if ((result = obj_encdata_get_policy(obj_child, TSS_POLICY_USAGE,
						     &sess->hUsageChild)))
			goto error;

		if ((result = obj_rsakey_get_tcs_handle(obj_parent, &sess->obj_parent)))
			goto error;
		break;
#ifdef TSS_BUILD_NV
	/* Child is an NV object */
	case TPM_ORD_NV_DefineSpace:
		/* The requirements variable tells us whether nv object auth is required */
		req_auth = requirements;

		if (req_auth) {
			if (sess->parentMode == TSS_SECRET_MODE_NONE) {
				result = TSPERR(TSS_E_TSP_AUTHREQUIRED);
				goto error;
			}

			if ((result = obj_nvstore_get_policy(obj_child, TSS_POLICY_USAGE,
							     &sess->hUsageChild)))
				goto error;

			/* According to the spec, we must fall back on the TSP context's policy for
			 * auth if none is set in the NV object */
			if (!sess->hUsageChild) {
				if ((result = obj_context_get_policy(tspContext, TSS_POLICY_USAGE,
								     &sess->hUsageChild)))
					goto error;
			}

			if ((result = obj_policy_is_secret_set(sess->hUsageChild, &secret_set)))
				goto error;

			if (!secret_set) {
				result = TSPERR(TSS_E_TSP_AUTHREQUIRED);
				goto error;
			}
		} else {
			/* In this case, the TPM is owned, but we're creating a no-auth NV area */
			get_child_auth = FALSE;
		}

		break;
#endif
	/* Child is a Key object */
	case TPM_ORD_MakeIdentity:
		if ((result = obj_rsakey_get_policy(obj_child, TSS_POLICY_USAGE,
						    &sess->hUsageChild, NULL)))
			goto error;
		break;
	/* Child is a Policy object */
	case TPM_ORD_Delegate_CreateKeyDelegation:
	case TPM_ORD_ChangeAuth:
		if ((result = obj_rsakey_get_tcs_handle(obj_parent, &sess->obj_parent)))
			goto error;
		/* fall through */
	case TPM_ORD_Delegate_CreateOwnerDelegation:
	case TPM_ORD_ChangeAuthOwner:
		sess->hUsageChild = obj_child;
		new_secret = TR_SECRET_CTX_NEW;
		break;
	default:
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto error;
	}

	/* If req_auth is FALSE here, we don't actually need to set up an auth session, so returning
	 * is OK.  At this point, authsess->pAuth is NULL, so the TCS API will not get any
	 * authdata. */
	if (req_auth == FALSE && sess->parentMode == TSS_SECRET_MODE_NONE)
		goto done;

	if (get_child_auth) {
		if ((result = obj_policy_get_xsap_params(sess->hUsageChild, 0, 0, NULL, NULL,
							 sess->encAuthUse.authdata, NULL, NULL,
							 &sess->cb_sealx, &sess->uMode,
							 new_secret)))
			goto error;
	}

	if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
				       (BYTE **)sess->nonceOddxSAP.nonce)))
		goto error;

	sess->obj_child = obj_child;
	sess->tspContext = tspContext;
	sess->pAuth = &sess->auth;
	sess->command = command;

#ifdef TSS_BUILD_DELEGATION
	/* if entityValue is set, we have a custom entity, i.e. delegation blob or row */
	if (sess->entityValue) {
		/* DSAP's entity type was pulled from the policy in the authsess_xsap_init call
		 * above */
		if ((result = authsess_do_dsap(sess)))
			goto error;
	}
#endif
	if (!sess->entityValue) {
		sess->entity_type = entity_type;
		if ((result = authsess_do_osap(sess)))
			goto error;
	}

	if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
				       (BYTE **)sess->auth.NonceOdd.nonce)))
		goto error;

	/* We have both OSAP nonces, so calculate the shared secret if we're responsible for it */
	if (sess->parentMode != TSS_SECRET_MODE_CALLBACK) {
		offset = 0;
		Trspi_LoadBlob(&offset, sizeof(TPM_NONCE), hmacBlob, sess->nonceEvenxSAP.nonce);
		Trspi_LoadBlob(&offset, sizeof(TPM_NONCE), hmacBlob, sess->nonceOddxSAP.nonce);

		if ((result = Trspi_HMAC(TSS_HASH_SHA1, sizeof(TPM_ENCAUTH),
					 sess->parentSecret.authdata, offset, hmacBlob,
					 sess->sharedSecret.digest)))
			goto error;
	}

	/* XXX What does a PurposeSecret of TRUE mean here? */
	if ((result =
	     ((TSS_RESULT (*)(PVOID, TSS_HOBJECT, TSS_HOBJECT, TSS_FLAG,
	       UINT32, BYTE *, BYTE *, BYTE *, BYTE *, UINT32, BYTE *,
	       BYTE *))sess->cb_xor.callback)(sess->cb_xor.appData, sess->hUsageParent,
					      sess->hUsageChild, TRUE, sizeof(TPM_DIGEST),
					      sess->auth.NonceEven.nonce, sess->auth.NonceOdd.nonce,
					      sess->nonceEvenxSAP.nonce, sess->nonceOddxSAP.nonce,
					      sizeof(TPM_ENCAUTH), sess->encAuthUse.authdata,
					      sess->encAuthMig.authdata)))
		return result;

done:
	*xsess = sess;

	return TSS_SUCCESS;
error:
	free(sess);
	return result;
}

TSS_RESULT
authsess_xsap_hmac(struct authsess *sess, TPM_DIGEST *digest)
{
	TSS_RESULT result;

	/* If no auth session was established using this authsess object, return success */
	if (!sess->pAuth)
		return TSS_SUCCESS;

	/* XXX Placeholder for future continueAuthSession support:
	 *      conditionally bump NonceOdd if continueAuthSession == TRUE here
	 */

	if ((result =
	    ((TSS_RESULT (*)(PVOID, TSS_HOBJECT, TSS_BOOL,
	      UINT32, TSS_BOOL, UINT32, BYTE *, BYTE *,
	      BYTE *, BYTE *, UINT32, BYTE *,
	      BYTE *))sess->cb_hmac.callback)(sess->cb_hmac.appData,
					      sess->hUsageParent, TRUE, sess->command,
					      sess->auth.fContinueAuthSession, sizeof(TPM_NONCE),
					      sess->auth.NonceEven.nonce,
					      sess->auth.NonceOdd.nonce,
					      sess->nonceEvenxSAP.nonce,
					      sess->nonceOddxSAP.nonce, sizeof(TPM_DIGEST),
					      digest->digest, sess->auth.HMAC.authdata)))
		return result;

	if (sess->hUsageParent)
		obj_policy_dec_counter(sess->hUsageParent);

	if (sess->hUsageChild)
		obj_policy_dec_counter(sess->hUsageChild);

	if (sess->hMigChild)
		obj_policy_dec_counter(sess->hMigChild);

	return TSS_SUCCESS;
}

TSS_RESULT
authsess_xsap_verify(struct authsess *sess, TPM_DIGEST *digest)
{
	/* If no auth session was established using this authsess object, return success */
	if (!sess->pAuth)
		return TSS_SUCCESS;

	return ((TSS_RESULT (*)(PVOID, TSS_HOBJECT, TSS_BOOL,
		 UINT32, TSS_BOOL, UINT32, BYTE *, BYTE *,
		 BYTE *, BYTE *, UINT32, BYTE *,
		 BYTE *))sess->cb_hmac.callback)(sess->cb_hmac.appData,
						 sess->hUsageParent, FALSE, sess->command,
						 sess->auth.fContinueAuthSession, sizeof(TPM_NONCE),
						 sess->auth.NonceEven.nonce,
						 sess->auth.NonceOdd.nonce,
						 sess->nonceEvenxSAP.nonce,
						 sess->nonceOddxSAP.nonce, sizeof(TPM_DIGEST),
						 digest->digest, sess->auth.HMAC.authdata);
}

TSS_RESULT
__tspi_free_resource(TSS_HCONTEXT tspContext, UINT32 handle, UINT32 resourceType)
{
	TSS_RESULT result = TSS_SUCCESS;
#ifdef TSS_BUILD_TSS12
	UINT32 version = 0;

	if ((result = obj_context_get_tpm_version(tspContext, &version)))
		return result;

	if (version == 2) {
		return TCS_API(tspContext)->FlushSpecific(tspContext, handle, resourceType);
	}
#endif

	switch (resourceType) {
		case TPM_RT_KEY:
			result = TCS_API(tspContext)->EvictKey(tspContext, handle);
			break;
		case TPM_RT_AUTH:
			result = TCS_API(tspContext)->TerminateHandle(tspContext, handle);
			break;
		default:
			LogDebugFn("Trying to free TPM 1.2 resource type 0x%x on 1.1 TPM!",
				   resourceType);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			break;
	}

	return result;
}

void
authsess_free(struct authsess *xsap)
{
	if (xsap) {
		if (xsap->auth.AuthHandle && xsap->auth.fContinueAuthSession)
			(void)__tspi_free_resource(xsap->tspContext, xsap->auth.AuthHandle, TPM_RT_AUTH);

		free(xsap->entityValue);
		free(xsap);
		xsap = NULL;
	}
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
Transport_OIAP(TSS_HCONTEXT    tspContext,   /* in */
	       TCS_AUTHHANDLE* authHandle,   /* out */
	       TPM_NONCE*      nonce0)       /* out */
{
	TSS_RESULT result;
	UINT32 decLen = 0;
	BYTE *dec = NULL;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_OIAP, 0, NULL, NULL,
						    &handlesLen, NULL, NULL, NULL, &decLen, &dec)))
		return result;

	if (decLen != sizeof(TCS_AUTHHANDLE) + sizeof(TPM_NONCE))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, authHandle, dec);
	Trspi_UnloadBlob_NONCE(&offset, dec, nonce0);

	return result;
}

TSS_RESULT
Transport_OSAP(TSS_HCONTEXT    tspContext,	/* in */
	       TPM_ENTITY_TYPE entityType,	/* in */
	       UINT32          entityValue,	/* in */
	       TPM_NONCE*      nonceOddOSAP,	/* in */
	       TCS_AUTHHANDLE* authHandle,	/* out */
	       TPM_NONCE*      nonceEven,	/* out */
	       TPM_NONCE*      nonceEvenOSAP)	/* out */
{
	TSS_RESULT result;
	UINT32 decLen = 0;
	BYTE *dec = NULL;
	UINT64 offset;
	TCS_HANDLE handlesLen = 0;
	BYTE data[sizeof(UINT16) + sizeof(UINT32) + sizeof(TPM_NONCE)];

	if ((result = obj_context_transport_init(tspContext)))
		return result;

	LogDebugFn("Executing in a transport session");

	offset = 0;
	Trspi_LoadBlob_UINT16(&offset, entityType, data);
	Trspi_LoadBlob_UINT32(&offset, entityValue, data);
	Trspi_LoadBlob_NONCE(&offset, data, nonceOddOSAP);

	if ((result = obj_context_transport_execute(tspContext, TPM_ORD_OSAP, sizeof(data), data,
						    NULL, &handlesLen, NULL, NULL, NULL, &decLen,
						    &dec)))
		return result;

	offset = 0;
	Trspi_UnloadBlob_UINT32(&offset, authHandle, dec);
	Trspi_UnloadBlob_NONCE(&offset, dec, nonceEven);
	Trspi_UnloadBlob_NONCE(&offset, dec, nonceEvenOSAP);

	return TSS_SUCCESS;
}

TSS_RESULT
Transport_TerminateHandle(TSS_HCONTEXT tspContext, /* in */
			  TCS_AUTHHANDLE handle)   /* in */
{
	TSS_RESULT result;
	TCS_HANDLE handlesLen = 0, *handles;

	/* Call ExecuteTransport */
	handlesLen = 1;
	if ((handles = malloc(sizeof(TCS_HANDLE))) == NULL) {
		LogError("malloc of %zd bytes failed", sizeof(TCS_HANDLE));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	*handles = handle;

	result = obj_context_transport_execute(tspContext, TPM_ORD_Terminate_Handle, 0, NULL,
					       NULL, &handlesLen, &handles, NULL, NULL, NULL, NULL);

	return result;
}
#endif
