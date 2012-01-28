
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2005, 2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"
#include "tsp_delegate.h"
#include "authsess.h"


TSS_RESULT
obj_policy_add(TSS_HCONTEXT tsp_context, UINT32 type, TSS_HOBJECT *phObject)
{
	struct tr_policy_obj *policy;
	TSS_RESULT result;

	if ((policy = calloc(1, sizeof(struct tr_policy_obj))) == NULL) {
		LogError("malloc of %zd bytes failed", sizeof(struct tr_policy_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	policy->type = type;
#ifndef TSS_SPEC_COMPLIANCE
	policy->SecretMode = TSS_SECRET_MODE_NONE;
#else
	policy->SecretMode = TSS_SECRET_MODE_POPUP;
#endif
	/* The policy object will inherit this attribute from the context */
	if ((result = obj_context_get_hash_mode(tsp_context, &policy->hashMode))) {
		free(policy);
		return result;
	}
	policy->SecretLifetime = TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS;
#ifdef TSS_BUILD_DELEGATION
	policy->delegationType = TSS_DELEGATIONTYPE_NONE;
#endif

	if ((result = obj_list_add(&policy_list, tsp_context, 0, policy, phObject))) {
		free(policy);
		return result;
	}

	return TSS_SUCCESS;
}

void
__tspi_policy_free(void *data)
{
	struct tr_policy_obj *policy = (struct tr_policy_obj *)data;

	free(policy->popupString);
#ifdef TSS_BUILD_DELEGATION
	free(policy->delegationBlob);
#endif
	free(policy);
}

TSS_RESULT
obj_policy_remove(TSS_HOBJECT hObject, TSS_HCONTEXT tspContext)
{
	TSS_RESULT result;

	if ((result = obj_list_remove(&policy_list, &__tspi_policy_free, hObject, tspContext)))
		return result;

	obj_lists_remove_policy_refs(hObject, tspContext);

	return TSS_SUCCESS;
}

TSS_BOOL
obj_is_policy(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&policy_list, hObject))) {
		answer = TRUE;
		obj_list_put(&policy_list);
	}

	return answer;
}

TSS_RESULT
obj_policy_get_type(TSS_HPOLICY hPolicy, UINT32 *type)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	*type = policy->type;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_set_type(TSS_HPOLICY hPolicy, UINT32 type)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	policy->type = type;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_get_tsp_context(TSS_HPOLICY hPolicy, TSS_HCONTEXT *tspContext)
{
	struct tsp_object *obj;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	*tspContext = obj->tspContext;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_do_hmac(TSS_HPOLICY hPolicy, TSS_HOBJECT hAuthorizedObject,
		   TSS_BOOL returnOrVerify, UINT32 ulPendingFunction,
		   TSS_BOOL continueUse, UINT32 ulSizeNonces,
		   BYTE *rgbNonceEven, BYTE *rgbNonceOdd,
		   BYTE *rgbNonceEvenOSAP, BYTE *rgbNonceOddOSAP,
		   UINT32 ulSizeDigestHmac, BYTE *rgbParamDigest,
		   BYTE *rgbHmacData)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	result = policy->Tspicb_CallbackHMACAuth(
			policy->hmacAppData, hAuthorizedObject,
			returnOrVerify,
			ulPendingFunction,
			continueUse,
			ulSizeNonces,
			rgbNonceEven,
			rgbNonceOdd,
			rgbNonceEvenOSAP, rgbNonceOddOSAP, ulSizeDigestHmac,
			rgbParamDigest,
			rgbHmacData);

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_secret(TSS_HPOLICY hPolicy, TSS_BOOL ctx, TCPA_SECRET *secret)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;
	TCPA_SECRET null_secret;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	memset(&null_secret, 0, sizeof(TCPA_SECRET));

	switch (policy->SecretMode) {
		case TSS_SECRET_MODE_POPUP:
			/* if the secret is still NULL, grab it using the GUI */
			if (policy->SecretSet == FALSE) {
				if ((result = popup_GetSecret(ctx,
							      policy->hashMode,
							      policy->popupString,
							      policy->Secret)))
					break;
			}
			policy->SecretSet = TRUE;
			/* fall through */
		case TSS_SECRET_MODE_PLAIN:
		case TSS_SECRET_MODE_SHA1:
			if (policy->SecretSet == FALSE) {
				result = TSPERR(TSS_E_POLICY_NO_SECRET);
				break;
			}

			memcpy(secret, policy->Secret, sizeof(TCPA_SECRET));
			break;
		case TSS_SECRET_MODE_NONE:
			memcpy(secret, &null_secret, sizeof(TCPA_SECRET));
			break;
		default:
			result = TSPERR(TSS_E_POLICY_NO_SECRET);
			break;
	}
#ifdef TSS_DEBUG
	if (!result) {
		LogDebug("Got a secret:");
		LogDebugData(20, (BYTE *)secret);
	}
#endif
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_flush_secret(TSS_HPOLICY hPolicy)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	memset(&policy->Secret, 0, policy->SecretSize);
	policy->SecretSet = FALSE;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_set_secret_object(TSS_HPOLICY hPolicy, TSS_FLAG mode, UINT32 size,
			     TCPA_DIGEST *digest, TSS_BOOL set)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	/* if this is going to be a callback policy, the
	 * callbacks need to already be set. (See TSS 1.1b
	 * spec pg. 62). */
	if (mode == TSS_SECRET_MODE_CALLBACK) {
		if (policy->Tspicb_CallbackHMACAuth == NULL) {
			result = TSPERR(TSS_E_FAIL);
			goto done;
		}
	}

	if (policy->SecretLifetime == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER) {
		policy->SecretCounter = policy->SecretTimeStamp;
	} else if (policy->SecretLifetime == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER) {
		time_t t = time(NULL);
		if (t == ((time_t)-1)) {
			LogError("time failed: %s", strerror(errno));
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		policy->SecretTimeStamp = t;
	}

	memcpy(policy->Secret, digest, size);
	policy->SecretMode = mode;
	policy->SecretSize = size;
	policy->SecretSet = set;
done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_is_secret_set(TSS_HPOLICY hPolicy, TSS_BOOL *secretSet) 
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	
	*secretSet = policy->SecretSet;
	obj_list_put(&policy_list);
	
	return result;
}
	

TSS_RESULT
obj_policy_set_secret(TSS_HPOLICY hPolicy, TSS_FLAG mode, UINT32 size, BYTE *data)
{
	TCPA_DIGEST digest;
	UINT32 secret_size = 0;
	TSS_BOOL secret_set = TRUE;
	TSS_RESULT result;

	memset(&digest.digest, 0, sizeof(TCPA_DIGEST));

	switch (mode) {
		case TSS_SECRET_MODE_PLAIN:
			if ((result = Trspi_Hash(TSS_HASH_SHA1, size, data,
						 (BYTE *)&digest.digest)))
				return result;
			secret_size = TCPA_SHA1_160_HASH_LEN;
			break;
		case TSS_SECRET_MODE_SHA1:
			if (size != TCPA_SHA1_160_HASH_LEN)
				return TSPERR(TSS_E_BAD_PARAMETER);

			memcpy(&digest.digest, data, size);
			secret_size = TCPA_SHA1_160_HASH_LEN;
			break;
		case TSS_SECRET_MODE_POPUP:
		case TSS_SECRET_MODE_NONE:
			secret_set = FALSE;
		case TSS_SECRET_MODE_CALLBACK:
			break;
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	return obj_policy_set_secret_object(hPolicy, mode, secret_size,
					    &digest, secret_set);
}

TSS_RESULT
obj_policy_get_cb11(TSS_HPOLICY hPolicy, TSS_FLAG type, UINT32 *cb)
{
#ifndef __LP64__
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	switch (type) {
		case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			*cb = (UINT32)policy->Tspicb_CallbackHMACAuth;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			*cb = (UINT32)policy->Tspicb_CallbackXorEnc;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			*cb = (UINT32)policy->Tspicb_CallbackTakeOwnership;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
			*cb = (UINT32)policy->Tspicb_CallbackChangeAuthAsym;
			break;
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}

	obj_list_put(&policy_list);

	return result;
#else
	return TSPERR(TSS_E_FAIL);
#endif
}

TSS_RESULT
obj_policy_set_cb11(TSS_HPOLICY hPolicy, TSS_FLAG type, TSS_FLAG app_data, UINT32 cb)
{
#ifndef __LP64__
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	switch (type) {
		case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			policy->Tspicb_CallbackHMACAuth = (PVOID)cb;
			policy->hmacAppData = (PVOID)app_data;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			policy->Tspicb_CallbackXorEnc = (PVOID)cb;
			policy->xorAppData = (PVOID)app_data;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			policy->Tspicb_CallbackTakeOwnership = (PVOID)cb;
			policy->takeownerAppData = (PVOID)app_data;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
			policy->Tspicb_CallbackChangeAuthAsym = (PVOID)cb;
			policy->changeauthAppData = (PVOID)app_data;
			break;
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}

	obj_list_put(&policy_list);

	return result;
#else
	return TSPERR(TSS_E_FAIL);
#endif
}

TSS_RESULT
obj_policy_set_cb12(TSS_HPOLICY hPolicy, TSS_FLAG flag, BYTE *in)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;
	TSS_CALLBACK *cb = (TSS_CALLBACK *)in;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	switch (flag) {
		case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			if (!cb) {
				policy->Tspicb_CallbackHMACAuth = NULL;
				break;
			}

			policy->Tspicb_CallbackHMACAuth =
				(TSS_RESULT (*)(PVOID, TSS_HOBJECT, TSS_BOOL,
				UINT32, TSS_BOOL, UINT32, BYTE *, BYTE *,
				BYTE *, BYTE *, UINT32, BYTE *, BYTE *))
				cb->callback;
			policy->hmacAppData = cb->appData;
			policy->hmacAlg = cb->alg;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			if (!cb) {
				policy->Tspicb_CallbackXorEnc = NULL;
				break;
			}

			policy->Tspicb_CallbackXorEnc =
				(TSS_RESULT (*)(PVOID, TSS_HOBJECT,
				TSS_HOBJECT, TSS_FLAG, UINT32, BYTE *, BYTE *,
				BYTE *, BYTE *, UINT32, BYTE *, BYTE *))
				cb->callback;
			policy->xorAppData = cb->appData;
			policy->xorAlg = cb->alg;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			if (!cb) {
				policy->Tspicb_CallbackTakeOwnership = NULL;
				break;
			}

			policy->Tspicb_CallbackTakeOwnership =
				(TSS_RESULT (*)(PVOID, TSS_HOBJECT, TSS_HKEY,
				UINT32, BYTE *))cb->callback;
			policy->takeownerAppData = cb->appData;
			policy->takeownerAlg = cb->alg;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
			if (!cb) {
				policy->Tspicb_CallbackChangeAuthAsym = NULL;
				break;
			}

			policy->Tspicb_CallbackChangeAuthAsym =
				(TSS_RESULT (*)(PVOID, TSS_HOBJECT, TSS_HKEY,
				UINT32, UINT32, BYTE *, BYTE *))cb->callback;
			policy->changeauthAppData = cb->appData;
			policy->changeauthAlg = cb->alg;
			break;
#ifdef TSS_BUILD_SEALX
		case TSS_TSPATTRIB_POLICY_CALLBACK_SEALX_MASK:
			if (!cb) {
				policy->Tspicb_CallbackSealxMask = NULL;
				policy->sealxAppData = NULL;
				policy->sealxAlg = 0;
				break;
			}

			policy->Tspicb_CallbackSealxMask =
				(TSS_RESULT (*)(PVOID, TSS_HKEY, TSS_HENCDATA,
				TSS_ALGORITHM_ID, UINT32, BYTE *, BYTE *, BYTE *, BYTE *,
				UINT32, BYTE *, BYTE *))cb->callback;
			policy->sealxAppData = cb->appData;
			policy->sealxAlg = cb->alg;
			break;
#endif
		default:
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_cb12(TSS_HPOLICY hPolicy, TSS_FLAG flag, UINT32 *size, BYTE **out)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;
	TSS_CALLBACK *cb;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if ((cb = calloc_tspi(obj->tspContext, sizeof(TSS_CALLBACK))) == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(TSS_CALLBACK));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	switch (flag) {
		case TSS_TSPATTRIB_POLICY_CALLBACK_HMAC:
			cb->callback = policy->Tspicb_CallbackHMACAuth;
			cb->appData = policy->hmacAppData;
			cb->alg = policy->hmacAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_XOR_ENC:
			cb->callback = policy->Tspicb_CallbackXorEnc;
			cb->appData = policy->xorAppData;
			cb->alg = policy->xorAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_TAKEOWNERSHIP:
			cb->callback = policy->Tspicb_CallbackTakeOwnership;
			cb->appData = policy->takeownerAppData;
			cb->alg = policy->takeownerAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
		case TSS_TSPATTRIB_POLICY_CALLBACK_CHANGEAUTHASYM:
			cb->callback = policy->Tspicb_CallbackChangeAuthAsym;
			cb->appData = policy->changeauthAppData;
			cb->alg = policy->changeauthAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
#ifdef TSS_BUILD_SEALX
		case TSS_TSPATTRIB_POLICY_CALLBACK_SEALX_MASK:
			cb->callback = policy->Tspicb_CallbackSealxMask;
			cb->appData = policy->sealxAppData;
			cb->alg = policy->sealxAlg;
			*size = sizeof(TSS_CALLBACK);
			*out = (BYTE *)cb;
			break;
#endif
		default:
			free_tspi(obj->tspContext, cb);
			result = TSPERR(TSS_E_INVALID_ATTRIB_FLAG);
			break;
	}
done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_lifetime(TSS_HPOLICY hPolicy, UINT32 *lifetime)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	*lifetime = policy->SecretLifetime;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_set_lifetime(TSS_HPOLICY hPolicy, UINT32 type, UINT32 value)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;
	time_t t;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	switch (type) {
		case TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS:
			policy->SecretCounter = 0;
			policy->SecretLifetime = TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS;
			policy->SecretTimeStamp = 0;
			break;
		case TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER:
			/* Both SecretCounter and SecretTimeStamp will receive value. Every time the
			 * policy is used, SecretCounter will be decremented. Each time SetSecret is
			 * called, SecretCounter will get the value stored in SecretTimeStamp */
			policy->SecretCounter = value;
			policy->SecretLifetime = TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER;
			policy->SecretTimeStamp = value;
			break;
		case TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER:
			t = time(NULL);
			if (t == ((time_t)-1)) {
				LogError("time failed: %s", strerror(errno));
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				break;
			}

			/* For mode time, we'll use the SecretCounter variable to hold the number
			 * of seconds we're valid and the SecretTimeStamp var to record the current
			 * timestamp. This should protect against overflows. */
			policy->SecretCounter = value;
			policy->SecretLifetime = TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER;
			policy->SecretTimeStamp = t;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
			break;
	}

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_mode(TSS_HPOLICY hPolicy, UINT32 *mode)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	*mode = policy->SecretMode;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_get_counter(TSS_HPOLICY hPolicy, UINT32 *counter)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->SecretLifetime == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER)
		*counter = policy->SecretCounter;
	else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_dec_counter(TSS_HPOLICY hPolicy)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	/* Only decrement if SecretCounter > 0, otherwise it could loop and become valid again */
	if (policy->SecretLifetime == TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER &&
	    policy->SecretCounter > 0) {
		policy->SecretCounter--;
	}

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

/* return a unicode string to the Tspi_GetAttribData function */
TSS_RESULT
obj_policy_get_string(TSS_HPOLICY hPolicy, UINT32 *size, BYTE **data)
{
	TSS_RESULT result = TSS_SUCCESS;
	BYTE *utf_string;
	UINT32 utf_size;
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	*size = policy->popupStringLength;
	if (policy->popupStringLength == 0) {
		*data = NULL;
	} else {
		utf_size = policy->popupStringLength;
		utf_string = Trspi_Native_To_UNICODE(policy->popupString,
						     &utf_size);
		if (utf_string == NULL) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*data = calloc_tspi(obj->tspContext, utf_size);
		if (*data == NULL) {
			free(utf_string);
			LogError("malloc of %d bytes failed.", utf_size);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		*size = utf_size;
		memcpy(*data, utf_string, utf_size);
		free(utf_string);
	}

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_set_string(TSS_HPOLICY hPolicy, UINT32 size, BYTE *data)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	free(policy->popupString);
	policy->popupString = data;
	policy->popupStringLength = size;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_get_secs_until_expired(TSS_HPOLICY hPolicy, UINT32 *secs)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	int seconds_elapsed;
	time_t t;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->SecretLifetime != TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	if ((t = time(NULL)) == ((time_t)-1)) {
		LogError("time failed: %s", strerror(errno));
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}
	/* curtime - SecretTimeStamp is the number of seconds elapsed since we started the timer.
	 * SecretCounter is the number of seconds the secret is valid.  If
	 * seconds_elspased > SecretCounter, we've expired. */
	seconds_elapsed = t - policy->SecretTimeStamp;
	if ((UINT32)seconds_elapsed >= policy->SecretCounter) {
		*secs = 0;
	} else {
		*secs = policy->SecretCounter - seconds_elapsed;
	}

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
policy_has_expired(struct tr_policy_obj *policy, TSS_BOOL *answer)
{
	switch (policy->SecretLifetime) {
	case TSS_TSPATTRIB_POLICYSECRET_LIFETIME_ALWAYS:
		*answer = FALSE;
		break;
	case TSS_TSPATTRIB_POLICYSECRET_LIFETIME_COUNTER:
		*answer = (policy->SecretCounter == 0 ? TRUE : FALSE);
		break;
	case TSS_TSPATTRIB_POLICYSECRET_LIFETIME_TIMER:
	{
		int seconds_elapsed;
		time_t t = time(NULL);

		if (t == ((time_t)-1)) {
			LogError("time failed: %s", strerror(errno));
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		/* curtime - SecretTimer is the number of seconds elapsed since we
		 * started the timer. SecretCounter is the number of seconds the
		 * secret is valid.  If seconds_elspased > SecretCounter, we've
		 * expired.
		 */
		seconds_elapsed = t - policy->SecretTimeStamp;
		*answer = ((UINT32)seconds_elapsed >= policy->SecretCounter ? TRUE : FALSE);
		break;
	}
	default:
		LogError("policy has an undefined secret lifetime!");
		return TSPERR(TSS_E_INVALID_OBJ_ACCESS);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_has_expired(TSS_HPOLICY hPolicy, TSS_BOOL *answer)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	result = policy_has_expired(policy, answer);

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_xsap_params(TSS_HPOLICY hPolicy,
			   TPM_COMMAND_CODE command,
			   TPM_ENTITY_TYPE *et,
			   UINT32 *entity_value_size,
			   BYTE **entity_value,
			   BYTE *secret,
			   TSS_CALLBACK *cb_xor,
			   TSS_CALLBACK *cb_hmac,
			   TSS_CALLBACK *cb_sealx,
			   UINT32 *mode,
			   TSS_BOOL new_secret)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result;
	TSS_BOOL answer = FALSE;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if ((result = policy_has_expired(policy, &answer)))
		goto done;

	if (answer) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}
#ifdef TSS_BUILD_DELEGATION
	/* if the delegation index or blob is set, check to see if the command is delegated, if so,
	 * return the blob or index as the secret data */
	if (command && (policy->delegationType != TSS_DELEGATIONTYPE_NONE)) {
		if (policy->delegationBlob) {
			if ((*entity_value = malloc(policy->delegationBlobLength)) == NULL) {
				LogError("malloc of %u bytes failed.",
					 policy->delegationBlobLength);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			memcpy(*entity_value, policy->delegationBlob, policy->delegationBlobLength);
			*entity_value_size = policy->delegationBlobLength;
			if (policy->delegationType == TSS_DELEGATIONTYPE_OWNER)
				*et = TPM_ET_DEL_OWNER_BLOB;
			else
				*et = TPM_ET_DEL_KEY_BLOB;
		} else {
			if ((*entity_value = malloc(sizeof(UINT32))) == NULL) {
				LogError("malloc of %zd bytes failed.", sizeof(UINT32));
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}

			*(UINT32 *)entity_value = policy->delegationIndex;
			*entity_value_size = sizeof(UINT32);
			*et = TPM_ET_DEL_ROW;
		}
	}
#endif
	/* Either this is a policy set to mode callback, in which case both xor and hmac addresses
	 * must be set, or this is an encrypted data object's policy, where its mode is independent
	 * of whether a sealx callback is set */
	if (policy->SecretMode == TSS_SECRET_MODE_CALLBACK && cb_xor && cb_hmac) {
		if ((policy->Tspicb_CallbackXorEnc && !policy->Tspicb_CallbackHMACAuth) ||
		    (!policy->Tspicb_CallbackXorEnc && policy->Tspicb_CallbackHMACAuth)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		cb_xor->callback = policy->Tspicb_CallbackXorEnc;
		cb_xor->appData = policy->xorAppData;
		cb_xor->alg = policy->xorAlg;

		cb_hmac->callback = policy->Tspicb_CallbackHMACAuth;
		cb_hmac->appData = policy->hmacAppData;
		cb_hmac->alg = policy->hmacAlg;
#ifdef TSS_BUILD_SEALX
	} else if (cb_sealx && policy->Tspicb_CallbackSealxMask) {
		cb_sealx->callback = policy->Tspicb_CallbackSealxMask;
		cb_sealx->appData = policy->sealxAppData;
		cb_sealx->alg = policy->sealxAlg;
#endif
	}

	if ((policy->SecretMode == TSS_SECRET_MODE_POPUP) &&
	    (policy->SecretSet == FALSE)) {
		if ((result = popup_GetSecret(new_secret,
					      policy->hashMode,
					      policy->popupString,
					      policy->Secret)))
			goto done;
			policy->SecretSet = TRUE;
	}	
	memcpy(secret, policy->Secret, TPM_SHA1_160_HASH_LEN);
	*mode = policy->SecretMode;
done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_do_xor(TSS_HPOLICY hPolicy,
		  TSS_HOBJECT hOSAPObject, TSS_HOBJECT hObject,
		  TSS_FLAG PurposeSecret, UINT32 ulSizeNonces,
		  BYTE *rgbNonceEven, BYTE *rgbNonceOdd,
		  BYTE *rgbNonceEvenOSAP, BYTE *rgbNonceOddOSAP,
		  UINT32 ulSizeEncAuth, BYTE *rgbEncAuthUsage,
		  BYTE *rgbEncAuthMigration)
{
	TSS_RESULT result;
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	result = policy->Tspicb_CallbackXorEnc(policy->xorAppData,
			hOSAPObject, hObject,
			PurposeSecret, ulSizeNonces,
			rgbNonceEven, rgbNonceOdd,
			rgbNonceEvenOSAP, rgbNonceOddOSAP,
			ulSizeEncAuth,
			rgbEncAuthUsage, rgbEncAuthMigration);

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_do_takeowner(TSS_HPOLICY hPolicy,
			TSS_HOBJECT hObject, TSS_HKEY hObjectPubKey,
			UINT32 ulSizeEncAuth, BYTE *rgbEncAuth)
{
	TSS_RESULT result;
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	result = policy->Tspicb_CallbackTakeOwnership(
			policy->takeownerAppData,
			hObject, hObjectPubKey, ulSizeEncAuth,
			rgbEncAuth);

	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_hash_mode(TSS_HPOLICY hPolicy, UINT32 *mode)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	*mode = policy->hashMode;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_set_hash_mode(TSS_HPOLICY hPolicy, UINT32 mode)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	switch (mode) {
		case TSS_TSPATTRIB_HASH_MODE_NULL:
		case TSS_TSPATTRIB_HASH_MODE_NOT_NULL:
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_DATA);
	}

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;
	policy->hashMode = mode;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

#ifdef TSS_BUILD_DELEGATION
TSS_RESULT
obj_policy_set_delegation_type(TSS_HPOLICY hPolicy, UINT32 type)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	switch (type) {
	case TSS_DELEGATIONTYPE_NONE:
		obj_policy_clear_delegation(policy);
		break;
	case TSS_DELEGATIONTYPE_OWNER:
	case TSS_DELEGATIONTYPE_KEY:
		if (policy->delegationIndexSet || policy->delegationBlob) {
			result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
			goto done;
		}
		break;
	}

	policy->delegationType = type;

done:
	obj_list_put(&policy_list);

	return result;
}


TSS_RESULT
obj_policy_get_delegation_type(TSS_HPOLICY hPolicy, UINT32 *type)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	*type = policy->delegationType;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_policy_set_delegation_index(TSS_HPOLICY hPolicy, UINT32 index)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if ((result = get_delegate_index(obj->tspContext, index, &public)))
		goto done;

	free(public.pcrInfo.pcrSelection.pcrSelect);

	obj_policy_clear_delegation(policy);
	switch (public.permissions.delegateType) {
	case TPM_DEL_OWNER_BITS:
		policy->delegationType = TSS_DELEGATIONTYPE_OWNER;
		break;
	case TPM_DEL_KEY_BITS:
		policy->delegationType = TSS_DELEGATIONTYPE_KEY;
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}
	policy->delegationIndex = index;
	policy->delegationIndexSet = TRUE;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_index(TSS_HPOLICY hPolicy, UINT32 *index)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (!policy->delegationIndexSet) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	*index = policy->delegationIndex;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT obj_policy_set_delegation_per1(TSS_HPOLICY hPolicy, UINT32 per1)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	policy->delegationPer1 = per1;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT obj_policy_get_delegation_per1(TSS_HPOLICY hPolicy, UINT32 *per1)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*per1 = public.permissions.per1;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		*per1 = policy->delegationPer1;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT obj_policy_set_delegation_per2(TSS_HPOLICY hPolicy, UINT32 per2)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	policy->delegationPer2 = per2;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT obj_policy_get_delegation_per2(TSS_HPOLICY hPolicy, UINT32 *per2)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*per2 = public.permissions.per2;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		*per2 = policy->delegationPer2;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_set_delegation_blob(TSS_HPOLICY hPolicy, UINT32 type, UINT32 blobLength, BYTE *blob)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	UINT16 tag;
	UINT64 offset;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	obj_policy_clear_delegation(policy);

	if (blobLength == 0) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}

	offset = 0;
	Trspi_UnloadBlob_UINT16(&offset, &tag, blob);
	switch (tag) {
	case TPM_TAG_DELEGATE_OWNER_BLOB:
		if (type && (type != TSS_DELEGATIONTYPE_OWNER)) {
			result = TSPERR(TSS_E_BAD_PARAMETER);
			goto done;
		}
		policy->delegationType = TSS_DELEGATIONTYPE_OWNER;
		break;
	case TPM_TAG_DELG_KEY_BLOB:
		if (type && (type != TSS_DELEGATIONTYPE_KEY)) {
			result = TSPERR(TSS_E_BAD_PARAMETER);
			goto done;
		}
		policy->delegationType = TSS_DELEGATIONTYPE_KEY;
		break;
	default:
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}

	if ((policy->delegationBlob = malloc(blobLength)) == NULL) {
		LogError("malloc of %u bytes failed.", blobLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	policy->delegationBlobLength = blobLength;
	memcpy(policy->delegationBlob, blob, blobLength);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_blob(TSS_HPOLICY hPolicy, UINT32 type, UINT32 *blobLength, BYTE **blob)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationBlobLength == 0) {
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);
		goto done;
	}

	if (type && (type != policy->delegationType)) {
		result = TSPERR(TSS_E_BAD_PARAMETER);
		goto done;
	}

	if ((*blob = calloc_tspi(obj->tspContext, policy->delegationBlobLength)) == NULL) {
		LogError("malloc of %u bytes failed.", policy->delegationBlobLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done;
	}

	memcpy(*blob, policy->delegationBlob, policy->delegationBlobLength);
	*blobLength = policy->delegationBlobLength;

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_label(TSS_HPOLICY hPolicy, BYTE *label)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*label = public.label.label;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_familyid(TSS_HPOLICY hPolicy, UINT32 *familyID)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*familyID = public.familyID;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_vercount(TSS_HPOLICY hPolicy, UINT32 *verCount)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*verCount = public.verificationCount;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_pcr_locality(TSS_HPOLICY hPolicy, UINT32 *locality)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*locality = public.pcrInfo.localityAtRelease;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_pcr_digest(TSS_HPOLICY hPolicy, UINT32 *digestLength, BYTE **digest)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		*digest = calloc_tspi(obj->tspContext, TPM_SHA1_160_HASH_LEN);
		if (*digest == NULL) {
			LogError("malloc of %u bytes failed.", TPM_SHA1_160_HASH_LEN);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		memcpy(*digest, &public.pcrInfo.digestAtRelease.digest, TPM_SHA1_160_HASH_LEN);
		*digestLength = TPM_SHA1_160_HASH_LEN;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_get_delegation_pcr_selection(TSS_HPOLICY hPolicy, UINT32 *selectionLength,
					BYTE **selection)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	TPM_DELEGATE_PUBLIC public;
	UINT64 offset;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet || policy->delegationBlob) {
		if ((result = obj_policy_get_delegate_public(obj, &public)))
			goto done;
		offset = 0;
		Trspi_LoadBlob_PCR_SELECTION(&offset, NULL, &public.pcrInfo.pcrSelection);
		*selection = calloc_tspi(obj->tspContext, offset);
		if (*selection == NULL) {
			LogError("malloc of %u bytes failed.", (UINT32)offset);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		offset = 0;
		Trspi_LoadBlob_PCR_SELECTION(&offset, *selection, &public.pcrInfo.pcrSelection);
		*selectionLength = offset;
		free(public.pcrInfo.pcrSelection.pcrSelect);
	} else
		result = TSPERR(TSS_E_INVALID_OBJ_ACCESS);

done:
	obj_list_put(&policy_list);

	return result;
}

TSS_RESULT
obj_policy_is_delegation_index_set(TSS_HPOLICY hPolicy, TSS_BOOL *indexSet)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;

	if ((obj = obj_list_get_obj(&policy_list, hPolicy)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	policy = (struct tr_policy_obj *)obj->data;

	*indexSet = policy->delegationIndexSet;

	obj_list_put(&policy_list);

	return TSS_SUCCESS;
}

void
obj_policy_clear_delegation(struct tr_policy_obj *policy)
{
	free(policy->delegationBlob);
	policy->delegationType = TSS_DELEGATIONTYPE_NONE;
	policy->delegationPer1 = 0;
	policy->delegationPer2 = 0;
	policy->delegationIndexSet = FALSE;
	policy->delegationIndex = 0;
	policy->delegationBlobLength = 0;
	policy->delegationBlob = NULL;
}

TSS_RESULT
obj_policy_get_delegate_public(struct tsp_object *obj, TPM_DELEGATE_PUBLIC *public)
{
	struct tr_policy_obj *policy;
	UINT16 tag;
	TPM_DELEGATE_OWNER_BLOB ownerBlob;
	TPM_DELEGATE_KEY_BLOB keyBlob;
	UINT64 offset;
	TSS_RESULT result;

	policy = (struct tr_policy_obj *)obj->data;

	if (policy->delegationIndexSet) {
		if ((result = get_delegate_index(obj->tspContext, policy->delegationIndex,
				public)))
			return result;
	} else if (policy->delegationBlob) {
		offset = 0;
		Trspi_UnloadBlob_UINT16(&offset, &tag, policy->delegationBlob);

		offset = 0;
		switch (tag) {
		case TPM_TAG_DELEGATE_OWNER_BLOB:
			if ((result = Trspi_UnloadBlob_TPM_DELEGATE_OWNER_BLOB(&offset,
									policy->delegationBlob,
									&ownerBlob)))
				return result;
			*public = ownerBlob.pub;
			free(ownerBlob.additionalArea);
			free(ownerBlob.sensitiveArea);
			break;
		case TPM_TAG_DELG_KEY_BLOB:
			if ((result = Trspi_UnloadBlob_TPM_DELEGATE_KEY_BLOB(&offset,
									     policy->delegationBlob,
									     &keyBlob)))
				return result;
			*public = keyBlob.pub;
			free(keyBlob.additionalArea);
			free(keyBlob.sensitiveArea);
			break;
		default:
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	} else
		return TSPERR(TSS_E_INTERNAL_ERROR);

	return TSS_SUCCESS;
}
#endif

