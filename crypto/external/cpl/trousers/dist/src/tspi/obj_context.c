
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
#include <assert.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "tcs_tsp.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
obj_context_add(TSS_HOBJECT *phObject)
{
	TSS_RESULT result;
	struct tr_context_obj *context = calloc(1, sizeof(struct tr_context_obj));
	unsigned len = strlen(TSS_LOCALHOST_STRING) + 1;

	if (context == NULL) {
		LogError("malloc of %zd bytes failed.", sizeof(struct tr_context_obj));
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

#ifndef TSS_NO_GUI
	context->silentMode = TSS_TSPATTRIB_CONTEXT_NOT_SILENT;
#else
	context->silentMode = TSS_TSPATTRIB_CONTEXT_SILENT;
#endif
	if ((context->machineName = calloc(1, len)) == NULL) {
		LogError("malloc of %u bytes failed", len);
		free(context);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	memcpy(context->machineName, TSS_LOCALHOST_STRING, len);
	context->machineNameLength = len;

	context->hashMode = TSS_TSPATTRIB_HASH_MODE_NOT_NULL;
	context->connection_policy = TSS_TSPATTRIB_CONTEXT_VERSION_V1_1;

	if ((result = obj_list_add(&context_list, NULL_HCONTEXT, 0, context, phObject))) {
		free(context->machineName);
		free(context);
		return result;
	}

	/* Add the default policy */
	if ((result = obj_policy_add(*phObject, TSS_POLICY_USAGE, &context->policy))) {
		obj_list_remove(&context_list, &__tspi_obj_context_free, *phObject, *phObject);
		return result;
	}

	context->tcs_api = &tcs_normal_api;

	return TSS_SUCCESS;
}

struct tcs_api_table *
obj_context_get_tcs_api(TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;
	struct tcs_api_table *t;

	/* If the object cannot be found with the given handle, return a safe value, the normal TCS
	 * API pointer.  Since the handle is bad, the RPC_ function will barf in looking up the
	 * corresponding TCS context handle and an invalid handle error will be returned. */
	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return &tcs_normal_api;

	context = (struct tr_context_obj *)obj->data;

	/* Return the current API set we're using, either the normal API, or the transport encrypted
	 * API.  The context->tcs_api variable is switched back and forth between the two sets by
	 * the obj_context_transport_set_control function through a set attrib. */
	t = context->tcs_api;

	obj_list_put(&context_list);

	return t;
}

void
__tspi_obj_context_free(void *data)
{
	struct tr_context_obj *context = (struct tr_context_obj *)data;

	free(context->machineName);
	free(context);
}

TSS_BOOL
obj_is_context(TSS_HOBJECT hObject)
{
	TSS_BOOL answer = FALSE;

	if ((obj_list_get_obj(&context_list, hObject))) {
		answer = TRUE;
		obj_list_put(&context_list);
	}

	return answer;
}

/* Clean up transport session if necessary. */
void
obj_context_close(TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return;

	context = (struct tr_context_obj *)obj->data;

#ifdef TSS_BUILD_TRANSPORT
	if (context->transAuth.AuthHandle) {
		RPC_FlushSpecific(tspContext, context->transAuth.AuthHandle, TPM_RT_TRANS);

		memset(&context->transPub, 0, sizeof(TPM_TRANSPORT_PUBLIC));
		memset(&context->transMod, 0, sizeof(TPM_MODIFIER_INDICATOR));
		memset(&context->transSecret, 0, sizeof(TPM_TRANSPORT_AUTH));
		memset(&context->transAuth, 0, sizeof(TPM_AUTH));
		memset(&context->transLogIn, 0, sizeof(TPM_TRANSPORT_LOG_IN));
		memset(&context->transLogOut, 0, sizeof(TPM_TRANSPORT_LOG_OUT));
		memset(&context->transLogDigest, 0, sizeof(TPM_DIGEST));
	}
#endif

	obj_list_put(&context_list);
}

TSS_RESULT
obj_context_get_policy(TSS_HCONTEXT tspContext, UINT32 policyType, TSS_HPOLICY *phPolicy)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;
	TSS_RESULT result = TSS_SUCCESS;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (policyType) {
		case TSS_POLICY_USAGE:
			*phPolicy = context->policy;
			break;
		default:
			result = TSPERR(TSS_E_BAD_PARAMETER);
	}

	obj_list_put(&context_list);

	return result;
}

TSS_RESULT
obj_context_get_machine_name(TSS_HCONTEXT tspContext, UINT32 *size, BYTE **data)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;
	TSS_RESULT result;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	if (context->machineNameLength == 0) {
		*data = NULL;
		*size = 0;
	} else {
		/*
		 * Don't use calloc_tspi because this memory is
		 * not freed using "free_tspi"
		 */
		*data = calloc(1, context->machineNameLength);
		if (*data == NULL) {
			LogError("malloc of %u bytes failed.",
					context->machineNameLength);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		*size = context->machineNameLength;
		memcpy(*data, context->machineName, *size);
	}

	result = TSS_SUCCESS;

done:
	obj_list_put(&context_list);

	return result;
}

/* This function converts the machine name to a TSS_UNICODE string before
 * returning it, as Tspi_GetAttribData would like. We could do the conversion
 * in Tspi_GetAttribData, but we don't have access to the TSP context there */
TSS_RESULT
obj_context_get_machine_name_attrib(TSS_HCONTEXT tspContext, UINT32 *size, BYTE **data)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;
	BYTE *utf_string;
	UINT32 utf_size;
	TSS_RESULT result;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	if (context->machineNameLength == 0) {
		*data = NULL;
		*size = 0;
	} else {
		utf_size = context->machineNameLength;
		utf_string = Trspi_Native_To_UNICODE(context->machineName,
						     &utf_size);
		if (utf_string == NULL) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*data = calloc_tspi(obj->tspContext, utf_size);
		if (*data == NULL) {
			free(utf_string);
			LogError("malloc of %u bytes failed.", utf_size);
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}
		*size = utf_size;
		memcpy(*data, utf_string, utf_size);
		free(utf_string);
	}

	result = TSS_SUCCESS;

done:
	obj_list_put(&context_list);

	return result;
}

TSS_RESULT
obj_context_set_machine_name(TSS_HCONTEXT tspContext, BYTE *name, UINT32 len)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	free(context->machineName);
	context->machineName = name;
	context->machineNameLength = len;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_BOOL
obj_context_is_silent(TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;
	TSS_BOOL silent = FALSE;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return FALSE;

	context = (struct tr_context_obj *)obj->data;
	if (context->silentMode == TSS_TSPATTRIB_CONTEXT_SILENT)
		silent = TRUE;

	obj_list_put(&context_list);

	return silent;
}

TSS_RESULT
obj_context_get_mode(TSS_HCONTEXT tspContext, UINT32 *mode)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;
	*mode = context->silentMode;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_context_set_mode(TSS_HCONTEXT tspContext, UINT32 mode)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;
	context->silentMode = mode;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

/* search the list of all policies bound to context @tspContext. If
 * one is found of type popup, return TRUE, else return FALSE. */
TSS_BOOL
obj_context_has_popups(TSS_HCONTEXT tspContext)
{
	struct tsp_object *obj;
	struct tr_policy_obj *policy;
	struct obj_list *list = &policy_list;
	TSS_BOOL ret = FALSE;

	MUTEX_LOCK(list->lock);

	for (obj = list->head; obj; obj = obj->next) {
		if (obj->tspContext == tspContext) {
			policy = (struct tr_policy_obj *)obj->data;
			if (policy->SecretMode == TSS_SECRET_MODE_POPUP)
				ret = TRUE;
			break;
		}
	}

	MUTEX_UNLOCK(list->lock);

	return ret;
}

TSS_RESULT
obj_context_get_hash_mode(TSS_HCONTEXT tspContext, UINT32 *mode)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;
	*mode = context->hashMode;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_context_set_hash_mode(TSS_HCONTEXT tspContext, UINT32 mode)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	switch (mode) {
		case TSS_TSPATTRIB_HASH_MODE_NULL:
		case TSS_TSPATTRIB_HASH_MODE_NOT_NULL:
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_DATA);
	}

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;
	context->hashMode = mode;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_context_get_connection_version(TSS_HCONTEXT tspContext, UINT32 *version)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	*version = context->current_connection;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_context_set_connection_policy(TSS_HCONTEXT tspContext, UINT32 policy)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	switch (policy) {
		case TSS_TSPATTRIB_CONTEXT_VERSION_V1_1:
		case TSS_TSPATTRIB_CONTEXT_VERSION_V1_2:
		case TSS_TSPATTRIB_CONTEXT_VERSION_AUTO:
			break;
		default:
			return TSPERR(TSS_E_INVALID_ATTRIB_DATA);
	}

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	context->connection_policy = policy;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

#ifdef TSS_BUILD_TRANSPORT
TSS_RESULT
obj_context_set_transport_key(TSS_HCONTEXT tspContext, TSS_HKEY hKey)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	context->transKey = hKey;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_context_transport_get_mode(TSS_HCONTEXT tspContext, UINT32 value, UINT32 *out)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (value) {
		case TSS_TSPATTRIB_TRANSPORT_NO_DEFAULT_ENCRYPTION:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT ?
			       FALSE : TRUE;
			break;
		case TSS_TSPATTRIB_TRANSPORT_DEFAULT_ENCRYPTION:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT ?
			       TRUE : FALSE;
			break;
		case TSS_TSPATTRIB_TRANSPORT_AUTHENTIC_CHANNEL:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC ?
			       TRUE : FALSE;
			break;
		case TSS_TSPATTRIB_TRANSPORT_EXCLUSIVE:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_EXCLUSIVE ?
			       TRUE : FALSE;
			break;
		case TSS_TSPATTRIB_TRANSPORT_STATIC_AUTH:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_STATIC_AUTH ?
			       TRUE : FALSE;
			break;
		default:
			LogError("Invalid attribute subflag: 0x%x", value);
			result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			break;
	}

	obj_list_put(&context_list);

	return result;
}

TSS_RESULT
obj_context_transport_get_control(TSS_HCONTEXT tspContext, UINT32 value, UINT32 *out)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (value) {
		case TSS_TSPATTRIB_DISABLE_TRANSPORT:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED ? FALSE : TRUE;
			break;
		case TSS_TSPATTRIB_ENABLE_TRANSPORT:
			*out = context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED ? TRUE : FALSE;
			break;
		default:
			LogError("Invalid attribute subflag: 0x%x", value);
			result = TSPERR(TSS_E_INVALID_ATTRIB_SUBFLAG);
			break;
	}

	obj_list_put(&context_list);

	return result;
}

TSS_RESULT
obj_context_transport_set_control(TSS_HCONTEXT tspContext, UINT32 value)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (value) {
		case TSS_TSPATTRIB_ENABLE_TRANSPORT:
			context->flags |= TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED;
			context->tcs_api = &tcs_transport_api;
			break;
		case TSS_TSPATTRIB_DISABLE_TRANSPORT:
			context->flags &= ~TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED;
			context->tcs_api = &tcs_normal_api;
			break;
		default:
			LogError("Invalid attribute subflag: 0x%x", value);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			break;
	}

	obj_list_put(&context_list);

	return result;
}

TSS_RESULT
obj_context_transport_set_mode(TSS_HCONTEXT tspContext, UINT32 value)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (value) {
		case TSS_TSPATTRIB_TRANSPORT_NO_DEFAULT_ENCRYPTION:
			context->flags &= ~TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT;
			break;
		case TSS_TSPATTRIB_TRANSPORT_DEFAULT_ENCRYPTION:
			context->flags |= TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT;
			break;
		case TSS_TSPATTRIB_TRANSPORT_AUTHENTIC_CHANNEL:
			context->flags |= TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC;
			break;
		case TSS_TSPATTRIB_TRANSPORT_EXCLUSIVE:
			context->flags |= TSS_CONTEXT_FLAGS_TRANSPORT_EXCLUSIVE;
			break;
		case TSS_TSPATTRIB_TRANSPORT_STATIC_AUTH:
			context->flags |= TSS_CONTEXT_FLAGS_TRANSPORT_STATIC_AUTH;
			break;
		default:
			LogError("Invalid attribute subflag: 0x%x", value);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			break;
	}

	obj_list_put(&context_list);

	return result;
}

#if 0
TSS_RESULT
get_trans_props(TSS_HCONTEXT tspContext, UINT32 *alg, UINT16 *enc)
{
	TSS_RESULT result;
	UINT32 algs[] = { TPM_ALG_MGF1, TPM_ALG_AES128, 0 }, a = 0;
	UINT16 encs[] = { TPM_ES_SYM_OFB, TPM_ES_SYM_CNT, TPM_ES_SYM_CBC_PKCS5PAD, 0 }, e = 0;
	BYTE *respData;
	UINT32 respLen, tcsSubCap32;
	UINT16 tcsSubCap16;

	if (*alg)
		goto check_es;

	for (a = 0; algs[a]; a++) {
		tcsSubCap32 = endian32(algs[a]);

		if ((result = RPC_GetTPMCapability(tspContext, TPM_CAP_TRANS_ALG, sizeof(UINT32),
						   (BYTE *)&tcsSubCap32, &respLen, &respData)))
			return result;

		if (*(TSS_BOOL *)respData == TRUE) {
			free(respData);
			break;
		}
		free(respData);
	}

	if (!algs[a]) {
		LogError("TPM reports no usable sym algorithms for transport session");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

check_es:
	if (*enc || algs[a] == TPM_ALG_MGF1)
		goto done;

	for (e = 0; encs[e]; e++) {
		tcsSubCap16 = endian16(encs[e]);

		if ((result = RPC_GetTPMCapability(tspContext, TPM_CAP_TRANS_ES, sizeof(UINT16),
						   (BYTE *)&tcsSubCap16, &respLen, &respData)))
			return result;

		if (*(TSS_BOOL *)respData == TRUE) {
			free(respData);
			break;
		}
		free(respData);
	}

	if (!encs[e]) {
		LogError("TPM reports no usable sym modes for transport session");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	*alg = algs[a];
	*enc = encs[e];
done:
	return TSS_SUCCESS;
}
#endif

/* called before each TCSP_ExecuteTransport call */
TSS_RESULT
obj_context_transport_init(TSS_HCONTEXT tspContext)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	/* return immediately if we're not in a transport session */
	if (!(context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED)) {
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}

	/* if the session is not yet established, setup and call EstablishTransport */
	if (!(context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_ESTABLISHED)) {
		if ((result = obj_context_transport_establish(tspContext, context)))
			goto done;
	}

	context->flags |= TSS_CONTEXT_FLAGS_TRANSPORT_ESTABLISHED;

	result = TSS_SUCCESS;
done:
	obj_list_put(&context_list);

	return result;
}

TSS_RESULT
obj_context_transport_establish(TSS_HCONTEXT tspContext, struct tr_context_obj *context)
{
	TSS_RESULT result;
	UINT32 tickLen, secretLen, transPubLen, exclusive = TSS_TCSATTRIB_TRANSPORT_DEFAULT;
	BYTE *ticks, *secret;
	UINT64 offset;
	Trspi_HashCtx hashCtx;
	TPM_DIGEST digest;
	TSS_HPOLICY hTransKeyPolicy;
	TPM_AUTH auth, *pAuth, *pTransAuth;
	TCS_KEY_HANDLE tcsTransKey;
	TSS_BOOL usesAuth = FALSE;
	UINT32 encKeyLen;
	BYTE encKey[256];
	BYTE transPubBlob[sizeof(TPM_TRANSPORT_PUBLIC)];
	BYTE transAuthBlob[sizeof(TPM_TRANSPORT_AUTH)];


	context->transPub.tag = TPM_TAG_TRANSPORT_PUBLIC;
	context->transSecret.tag = TPM_TAG_TRANSPORT_AUTH;

	if ((result = get_local_random(tspContext, FALSE, TPM_SHA1_160_HASH_LEN,
				       (BYTE **)context->transSecret.authData.authdata)))
		return result;

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_STATIC_AUTH)
		context->transKey = TPM_KH_TRANSPORT;

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC)
		context->transPub.transAttributes |= TPM_TRANSPORT_LOG;

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_EXCLUSIVE) {
		context->transPub.transAttributes |= TPM_TRANSPORT_EXCLUSIVE;
		exclusive = TSS_TCSATTRIB_TRANSPORT_EXCLUSIVE;
	}

	/* XXX implement AES128+CTR (Winbond, Infineon), then AES256+CTR (Atmel) */
	context->transPub.algId = TPM_ALG_MGF1;
	context->transPub.encScheme = TPM_ES_NONE;

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT) {
		context->transPub.transAttributes |= TPM_TRANSPORT_ENCRYPT;

		if (context->transKey == TPM_KH_TRANSPORT) {
			LogError("No transport key handle has been set yet. Use "
				 "Tspi_Context_SetTransEncryptionKey to set this handle");
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	if (context->transKey == TPM_KH_TRANSPORT) {
		secret = context->transSecret.authData.authdata;
		secretLen = TPM_SHA1_160_HASH_LEN;
	} else {
		offset = 0;
		Trspi_LoadBlob_TRANSPORT_AUTH(&offset, transAuthBlob, &context->transSecret);
		secretLen = offset;

		/* encrypt the sym key with the wrapping RSA key */
		encKeyLen = sizeof(encKey);
		if ((result = __tspi_rsa_encrypt(context->transKey, secretLen, transAuthBlob, &encKeyLen,
					  encKey)))
			return result;

		secret = encKey;
		secretLen = encKeyLen;
	}

	offset = 0;
	Trspi_LoadBlob_TRANSPORT_PUBLIC(&offset, transPubBlob, &context->transPub);
	transPubLen = offset;

	if (context->transKey != TPM_KH_TRANSPORT) {
		if ((result = obj_rsakey_get_tcs_handle(context->transKey, &tcsTransKey)))
			return result;

		if ((result = obj_rsakey_get_policy(context->transKey, TSS_POLICY_USAGE,
						    &hTransKeyPolicy, &usesAuth)))
			return result;

		if (!usesAuth) {
			LogError("Key used to establish a transport session must use auth");
			return TSPERR(TSS_E_TSP_TRANS_AUTHREQUIRED);
		}
	} else
		tcsTransKey = TPM_KH_TRANSPORT;

	/* If logging is on, do TPM commands spec rev106 step 8.a */
	memset(context->transLogDigest.digest, 0, sizeof(TPM_DIGEST));
	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC) {
		context->transLogIn.tag = TPM_TAG_TRANSPORT_LOG_IN;

		/* step 8.a, i */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_EstablishTransport);
		result |= Trspi_HashUpdate(&hashCtx, transPubLen, transPubBlob);
		result |= Trspi_Hash_UINT32(&hashCtx, secretLen);
		result |= Trspi_HashUpdate(&hashCtx, secretLen, secret);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogIn.parameters.digest)))
			return result;

		/* step 8.a, ii */
		memset(context->transLogIn.pubKeyHash.digest, 0, sizeof(TPM_DIGEST));

		/* step 8.a, iii */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_DIGEST(&hashCtx, context->transLogDigest.digest);
		result |= Trspi_Hash_TRANSPORT_LOG_IN(&hashCtx, &context->transLogIn);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogDigest.digest)))
			return result;
	}

	if (usesAuth) {
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_EstablishTransport);
		result |= Trspi_HashUpdate(&hashCtx, (UINT32)offset, (BYTE *)transPubBlob);
		result |= Trspi_Hash_UINT32(&hashCtx, secretLen);
		result |= Trspi_HashUpdate(&hashCtx, secretLen, secret);
		if ((result |= Trspi_HashFinal(&hashCtx, (BYTE *)&digest)))
			return result;

		/* open OIAP session with continueAuthSession = TRUE */
		if ((result = secret_PerformAuth_OIAP(context->transKey, TPM_ORD_EstablishTransport,
						      hTransKeyPolicy, TRUE, &digest, &auth)))
			return result;

		pAuth = &auth;
	} else
		pAuth = NULL;

	result = RPC_EstablishTransport(tspContext, exclusive, tcsTransKey, transPubLen,
					transPubBlob, secretLen, secret, pAuth, &context->transMod,
					&context->transAuth.AuthHandle, &tickLen, &ticks,
					&context->transAuth.NonceEven);
	if (result) {
		LogError("Establish Transport command failed: %s", Trspi_Error_String(result));
		return result;
	}

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_EstablishTransport);
	result |= Trspi_Hash_UINT32(&hashCtx, context->transMod);
	result |= Trspi_HashUpdate(&hashCtx, tickLen, ticks);
	result |= Trspi_Hash_NONCE(&hashCtx, context->transAuth.NonceEven.nonce);
	if ((result |= Trspi_HashFinal(&hashCtx, digest.digest)))
		return result;

	if (usesAuth) {
		if ((result = obj_policy_validate_auth_oiap(hTransKeyPolicy, &digest, pAuth)))
			return result;
	}

	/* step 8.b iii */
	offset = 0;
	Trspi_UnloadBlob_CURRENT_TICKS(&offset, ticks, &context->transLogOut.currentTicks);
	free(ticks);

	/* If logging is on, do TPM commands spec rev106 step 8.b */
	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC) {
		context->transLogOut.tag = TPM_TAG_TRANSPORT_LOG_OUT;

		/* step 8.b i */
		memcpy(context->transLogOut.parameters.digest, digest.digest, sizeof(TPM_DIGEST));

		/* step 8.b ii */
		context->transLogOut.locality = context->transMod;

		/* step 8.b iii was done above */
		/* step 8.b iv */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_DIGEST(&hashCtx, context->transLogDigest.digest);
		result |= Trspi_Hash_TRANSPORT_LOG_OUT(&hashCtx, &context->transLogOut);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogDigest.digest)))
			return result;
	}

	LogDebug("Transport session established successfully");

	pTransAuth = &context->transAuth;
	pTransAuth->fContinueAuthSession = TRUE;
	if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
				       (BYTE **)pTransAuth->NonceOdd.nonce))) {
		LogError("Failed creating random nonce");
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
do_transport_decryption(TPM_TRANSPORT_PUBLIC *transPub,
			TPM_AUTH *pTransAuth,
			BYTE *secret,
			UINT32 inLen,
			BYTE *in,
			UINT32 *outLen,
			BYTE **out)
{
	TSS_RESULT result;
	UINT32 i, decLen;
	UINT32 seedLen, ivLen;
	BYTE *dec;
	BYTE seed[(2 * sizeof(TPM_NONCE)) + strlen("out") + TPM_SHA1_160_HASH_LEN];

	/* allocate the most data anyone below might need */
	decLen = inLen;//((inLen / TSS_MAX_SYM_BLOCK_SIZE) + 1) * TSS_MAX_SYM_BLOCK_SIZE;
	if ((dec = malloc(decLen)) == NULL) {
		LogError("malloc of %u bytes failed", decLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	/* set the common 3 initial values of 'seed', which is used to generate either the IV or
	 * mask */
	memcpy(seed, pTransAuth->NonceEven.nonce, sizeof(TPM_NONCE));
	memcpy(&seed[sizeof(TPM_NONCE)], pTransAuth->NonceOdd.nonce, sizeof(TPM_NONCE));
	memcpy(&seed[2 * sizeof(TPM_NONCE)], "out", strlen("out"));

	switch (transPub->algId) {
	case TPM_ALG_MGF1:
	{
		decLen = inLen;
		seedLen = sizeof(seed);

		/* add the secret data to the seed for MGF1 */
		memcpy(&seed[2 * sizeof(TPM_NONCE) + strlen("out")], secret, TPM_SHA1_160_HASH_LEN);

		if ((result = Trspi_MGF1(TSS_HASH_SHA1, seedLen, seed, decLen, dec))) {
			free(dec);
			return result;
		}

		for (i = 0; i < inLen; i++)
			dec[i] ^= in[i];
		break;
	}
	case TPM_ALG_AES128:
	{
		BYTE iv[TSS_MAX_SYM_BLOCK_SIZE];

		ivLen = TSS_MAX_SYM_BLOCK_SIZE;
		seedLen = (2 * sizeof(TPM_NONCE)) + strlen("out");

		if ((result = Trspi_MGF1(TSS_HASH_SHA1, seedLen, seed, ivLen, iv))) {
			free(dec);
			return result;
		}

		/* use the secret data as the key for AES */
		if ((result = Trspi_SymEncrypt(transPub->algId, transPub->encScheme, secret, iv, in,
					       inLen, dec, &decLen))) {
			free(dec);
			return result;
		}

		break;
	}
	default:
		LogDebug("Unknown algorithm for encrypted transport session: 0x%x",
			 transPub->algId);
		free(dec);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	*out = dec;
	*outLen = decLen;

	return result;
}

TSS_RESULT
do_transport_encryption(TPM_TRANSPORT_PUBLIC *transPub,
			TPM_AUTH *pTransAuth,
			BYTE *secret,
			UINT32 inLen,
			BYTE *in,
			UINT32 *outLen,
			BYTE **out)
{
	TSS_RESULT result;
	UINT32 i, encLen;
	UINT32 seedLen, ivLen;
	BYTE *enc;
	BYTE seed[(2 * sizeof(TPM_NONCE)) + strlen("in") + TPM_SHA1_160_HASH_LEN];

	/* allocate the most data anyone below might need */
	encLen = ((inLen / TSS_MAX_SYM_BLOCK_SIZE) + 1) * TSS_MAX_SYM_BLOCK_SIZE;
	if ((enc = malloc(encLen)) == NULL) {
		LogError("malloc of %u bytes failed", encLen);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	/* set the common 3 initial values of 'seed', which is used to generate either the IV or
	 * mask */
	memcpy(seed, pTransAuth->NonceEven.nonce, sizeof(TPM_NONCE));
	memcpy(&seed[sizeof(TPM_NONCE)], pTransAuth->NonceOdd.nonce, sizeof(TPM_NONCE));
	memcpy(&seed[2 * sizeof(TPM_NONCE)], "in", strlen("in"));

	switch (transPub->algId) {
	case TPM_ALG_MGF1:
	{
		encLen = inLen;
		seedLen = sizeof(seed);

		/* add the secret data to the seed for MGF1 */
		memcpy(&seed[2 * sizeof(TPM_NONCE) + strlen("in")], secret, TPM_SHA1_160_HASH_LEN);

		if ((result = Trspi_MGF1(TSS_HASH_SHA1, seedLen, seed, encLen, enc))) {
			free(enc);
			return result;
		}

		for (i = 0; i < inLen; i++)
			enc[i] ^= in[i];
		break;
	}
	case TPM_ALG_AES128:
	{
		BYTE iv[TSS_MAX_SYM_BLOCK_SIZE];

		ivLen = TSS_MAX_SYM_BLOCK_SIZE;
		seedLen = (2 * sizeof(TPM_NONCE)) + strlen("in");

		if ((result = Trspi_MGF1(TSS_HASH_SHA1, seedLen, seed, ivLen, iv))) {
			free(enc);
			return result;
		}

		/* use the secret data as the key for AES */
		if ((result = Trspi_SymEncrypt(transPub->algId, transPub->encScheme, secret, iv, in,
					       inLen, enc, &encLen))) {
			free(enc);
			return result;
		}

		break;
	}
	default:
		LogDebug("Unknown algorithm for encrypted transport session: 0x%x",
			 transPub->algId);
		free(enc);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	*out = enc;
	*outLen = encLen;

	return result;
}

TSS_RESULT
obj_context_transport_execute(TSS_HCONTEXT     tspContext,
			      TPM_COMMAND_CODE ordinal,
			      UINT32           ulDataLen,
			      BYTE*            rgbData,
			      TPM_DIGEST*      pubKeyHash,
			      UINT32*          handlesLen,
			      TCS_HANDLE**     handles,
			      TPM_AUTH*        pAuth1,
			      TPM_AUTH*        pAuth2,
			      UINT32*          outLen,
			      BYTE**           out)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;
	UINT32 encLen, ulWrappedDataLen = 0;
	BYTE *pEnc = NULL, *rgbWrappedData = NULL;
	TPM_RESULT tpmResult;
	Trspi_HashCtx hashCtx;
	TPM_DIGEST etDigest, wDigest;
	TPM_AUTH *pTransAuth;
	UINT64 currentTicks;
	TSS_BOOL free_enc = FALSE;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	pTransAuth = &context->transAuth;

	/* TPM Commands spec rev106 step 6 */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, ordinal);

	switch (ordinal) {
		case TPM_ORD_OSAP:
		case TPM_ORD_OIAP:
			break;
		default:
			result |= Trspi_HashUpdate(&hashCtx, ulDataLen, rgbData);
			break;
	}

	if ((result |= Trspi_HashFinal(&hashCtx, wDigest.digest)))
		goto done;

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC) {
		/* TPM Commands spec rev106 step 10.b */
		memcpy(context->transLogIn.parameters.digest, wDigest.digest, sizeof(TPM_DIGEST));
		/* TPM Commands spec rev106 step 10.c, d or e, calculated by the caller */
		if (pubKeyHash)
			memcpy(context->transLogIn.pubKeyHash.digest, pubKeyHash->digest,
			       sizeof(TPM_DIGEST));
		else
			memset(context->transLogIn.pubKeyHash.digest, 0, sizeof(TPM_DIGEST));

		/* TPM Commands spec rev106 step 10.f */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_DIGEST(&hashCtx, context->transLogDigest.digest);
		result |= Trspi_Hash_TRANSPORT_LOG_IN(&hashCtx, &context->transLogIn);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogDigest.digest)))
			goto done;
	}

	/* TPM Commands spec rev106 step 7.a */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ExecuteTransport);
	result |= Trspi_Hash_UINT32(&hashCtx, ulDataLen + TSS_TPM_TXBLOB_HDR_LEN
							+ (*handlesLen * sizeof(UINT32))
							+ (pAuth1 ? TPM_AUTH_RQU_SIZE : 0)
							+ (pAuth2 ? TPM_AUTH_RQU_SIZE : 0));
	result |= Trspi_HashUpdate(&hashCtx, TPM_SHA1_160_HASH_LEN, wDigest.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, etDigest.digest)))
		goto done;

	/* encrypt the data if necessary */
	if (ulDataLen && context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT) {
		switch (ordinal) {
		case TPM_ORD_OSAP:
		case TPM_ORD_OIAP:
			encLen = ulDataLen;
			pEnc = rgbData;
			break;
		case TPM_ORD_DSAP:
		{
			UINT64 offset;
			UINT32 tmpLen, entityValueLen;
			BYTE *tmpEnc, *entityValuePtr;

			/* DSAP is a special case where only entityValue is encrypted. So, we'll
			 * parse through rgbData until we get to entityValue, encrypt it, alloc
			 * new space for rgbData (since it could be up to a block length larger
			 * than it came in) and copy the unencrypted data and the encrypted
			 * entityValue to the new block, setting pEnc and encLen to new values. */

			offset = (2 * sizeof(UINT32)) + sizeof(TPM_NONCE);
			Trspi_UnloadBlob_UINT32(&offset, &entityValueLen, rgbData);

			entityValuePtr = &rgbData[offset];
			if ((result = do_transport_encryption(&context->transPub, pTransAuth,
							context->transSecret.authData.authdata,
							entityValueLen, entityValuePtr, &tmpLen,
							&tmpEnc)))
				goto done;

			/* offset is the amount of data before the block we encrypted and tmpLen is
			 * the size of the encrypted data */
			encLen = offset + tmpLen;
			if ((pEnc = malloc(encLen)) == NULL) {
				LogError("malloc of %u bytes failed.", encLen);
				result = TSPERR(TSS_E_OUTOFMEMORY);
				goto done;
			}
			memcpy(pEnc, rgbData, offset);
			memcpy(&pEnc[offset], tmpEnc, tmpLen);
			free(tmpEnc);

			free_enc = TRUE;
			break;
		}
		default:
			if ((result = do_transport_encryption(&context->transPub, pTransAuth,
							context->transSecret.authData.authdata,
							ulDataLen, rgbData, &encLen, &pEnc)))
				goto done;

			free_enc = TRUE;
			break;
		}
	} else {
		encLen = ulDataLen;
		pEnc = rgbData;
	}

	/* TPM Commands spec rev106 step 7.b */
	HMAC_Auth(context->transSecret.authData.authdata, etDigest.digest, pTransAuth);

	if ((result = RPC_ExecuteTransport(tspContext, ordinal, encLen, pEnc, handlesLen, handles,
					   pAuth1, pAuth2, pTransAuth, &currentTicks,
					   &context->transMod, &tpmResult, &ulWrappedDataLen,
					   &rgbWrappedData))) {
		LogDebugFn("Execute Transport failed: %s", Trspi_Error_String(result));
		goto done;
	}

	if (tpmResult) {
		LogDebug("Wrapped command ordinal 0x%x failed with result: 0x%x", ordinal,
			 tpmResult);
		result = tpmResult;
		goto done;
	}

	/* decrypt the returned wrapped data if necessary */
	if (ulWrappedDataLen && context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_DEFAULT_ENCRYPT) {
		switch (ordinal) {
		case TPM_ORD_OSAP:
		case TPM_ORD_OIAP:
		case TPM_ORD_DSAP:
			*outLen = ulWrappedDataLen;
			*out = rgbWrappedData;
			break;
		default:
			if ((result = do_transport_decryption(&context->transPub, pTransAuth,
							context->transSecret.authData.authdata,
							ulWrappedDataLen, rgbWrappedData, outLen,
							out)))
					goto done;

			free(rgbWrappedData);
		}
	} else {
		if (outLen) {
			*outLen = ulWrappedDataLen;
			*out = rgbWrappedData;
		}
	}

	/* TPM Commands spec rev106 step 14 */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, tpmResult);
	result |= Trspi_Hash_UINT32(&hashCtx, ordinal);

	switch (ordinal) {
		case TPM_ORD_OSAP:
		case TPM_ORD_OIAP:
			break;
		default:
			if (outLen)
				result |= Trspi_HashUpdate(&hashCtx, *outLen, *out);
			break;
	}

	if ((result |= Trspi_HashFinal(&hashCtx, wDigest.digest)))
		goto done;

	/* TPM Commands spec rev106 step 15 */
	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ExecuteTransport);
	result |= Trspi_Hash_UINT64(&hashCtx, currentTicks);
	result |= Trspi_Hash_UINT32(&hashCtx, context->transMod);
	result |= Trspi_Hash_UINT32(&hashCtx, (outLen ? *outLen : 0)
					      + TSS_TPM_TXBLOB_HDR_LEN
					      + (*handlesLen * sizeof(UINT32))
					      + (pAuth1 ? TPM_AUTH_RSP_SIZE : 0)
					      + (pAuth2 ? TPM_AUTH_RSP_SIZE : 0));
	result |= Trspi_HashUpdate(&hashCtx, TPM_SHA1_160_HASH_LEN, wDigest.digest);
	if ((result |= Trspi_HashFinal(&hashCtx, etDigest.digest)))
		goto done;

	if (validateReturnAuth(context->transSecret.authData.authdata, etDigest.digest,
			       pTransAuth)) {
		result = TSPERR(TSS_E_TSP_TRANS_AUTHFAIL);
		goto done;
	}

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC) {
		context->transLogOut.currentTicks.currentTicks = currentTicks;

		/* TPM Commands spec rev106 step 16.b */
		memcpy(context->transLogOut.parameters.digest, wDigest.digest, sizeof(TPM_DIGEST));
		/* TPM Commands spec rev106 step 16.c done above */
		/* TPM Commands spec rev106 step 16.d */
		context->transLogOut.locality = context->transMod;

		/* TPM Commands spec rev106 step 16.d */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_DIGEST(&hashCtx, context->transLogDigest.digest);
		result |= Trspi_Hash_TRANSPORT_LOG_OUT(&hashCtx, &context->transLogOut);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogDigest.digest)))
			goto done;
	}

	/* Refresh nonceOdd for continued transport auth session */
	if ((result = get_local_random(tspContext, FALSE, sizeof(TPM_NONCE),
				       (BYTE **)pTransAuth->NonceOdd.nonce))) {
		LogError("Failed creating random nonce");
	}

done:
	if (free_enc)
		free(pEnc);
	obj_list_put(&context_list);

	return result;
}

/* called to close a transport session */
TSS_RESULT
obj_context_transport_close(TSS_HCONTEXT   tspContext,
			    TSS_HKEY       hKey,
			    TSS_HPOLICY    hPolicy,
			    TSS_BOOL       usesAuth,
			    TPM_SIGN_INFO* signInfo,
			    UINT32*        sigLen,
			    BYTE**         sig)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;
	Trspi_HashCtx hashCtx;
	TPM_DIGEST digest;
	TPM_AUTH auth, *pAuth;
	TCS_KEY_HANDLE tcsKey;
	BYTE *ticks = NULL;
	UINT32 tickLen;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	/* return immediately if we're not in a transport session */
	if (!(context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_ENABLED)) {
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto done;
	}

	if ((result = obj_rsakey_get_tcs_handle(hKey, &tcsKey)))
		goto done;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ReleaseTransportSigned);
	result |= Trspi_Hash_NONCE(&hashCtx, signInfo->replay.nonce);
	if ((result |= Trspi_HashFinal(&hashCtx, (BYTE *)&digest)))
		goto done;

	if (usesAuth) {
		if ((result = secret_PerformAuth_OIAP(hKey, TPM_ORD_ReleaseTransportSigned,
						      hPolicy, FALSE, &digest, &auth)))
			goto done;

		pAuth = &auth;
	} else
		pAuth = NULL;

	/* continue the auth session established in obj_context_transport_establish */
	HMAC_Auth(context->transSecret.authData.authdata, digest.digest, &context->transAuth);

	if ((result = RPC_ReleaseTransportSigned(tspContext, tcsKey, &signInfo->replay, pAuth,
						 &context->transAuth,
						 &context->transLogOut.locality, &tickLen, &ticks,
						 sigLen, sig)))
		goto done;

	result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
	result |= Trspi_Hash_UINT32(&hashCtx, result);
	result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ReleaseTransportSigned);
	result |= Trspi_Hash_UINT32(&hashCtx, context->transLogOut.locality);
	result |= Trspi_HashUpdate(&hashCtx, tickLen, ticks);
	result |= Trspi_Hash_UINT32(&hashCtx, *sigLen);
	result |= Trspi_HashUpdate(&hashCtx, *sigLen, *sig);
	if ((result |= Trspi_HashFinal(&hashCtx, (BYTE *)&digest)))
		goto done_disabled;

	/* validate the return data using the key's auth */
	if (pAuth) {
		if ((result = obj_policy_validate_auth_oiap(hPolicy, &digest, pAuth)))
			goto done_disabled;
	}

	/* validate again using the transport session's auth */
	if ((result = validateReturnAuth(context->transSecret.authData.authdata, digest.digest,
					 &context->transAuth))) {
		result = TSPERR(TSS_E_TSP_TRANS_AUTHFAIL);
		goto done_disabled;
	}

	if (context->flags & TSS_CONTEXT_FLAGS_TRANSPORT_AUTHENTIC) {
		UINT64 offset;

		/* TPM Commands Spec step 6.b */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_UINT32(&hashCtx, TPM_ORD_ReleaseTransportSigned);
		result |= Trspi_Hash_NONCE(&hashCtx, signInfo->replay.nonce);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogOut.parameters.digest)))
			goto done_disabled;

		/* TPM Commands Spec step 6.c */
		offset = 0;
		Trspi_UnloadBlob_CURRENT_TICKS(&offset, ticks, &context->transLogOut.currentTicks);
		free(ticks);

		/* TPM Commands Spec step 6.d was set above */
		/* TPM Commands Spec step 6.e */
		result = Trspi_HashInit(&hashCtx, TSS_HASH_SHA1);
		result |= Trspi_Hash_DIGEST(&hashCtx, context->transLogDigest.digest);
		result |= Trspi_Hash_TRANSPORT_LOG_OUT(&hashCtx, &context->transLogOut);
		if ((result |= Trspi_HashFinal(&hashCtx, context->transLogDigest.digest)))
			goto done_disabled;
	}

	if ((signInfo->data = malloc(sizeof(TPM_DIGEST))) == NULL) {
		LogError("malloc %zd bytes failed.", sizeof(TPM_DIGEST));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto done_disabled;
	}
	memcpy(signInfo->data, context->transLogDigest.digest, sizeof(TPM_DIGEST));
	signInfo->dataLen = sizeof(TPM_DIGEST);

	/* destroy all transport session info, except the key handle */
	memset(&context->transPub, 0, sizeof(TPM_TRANSPORT_PUBLIC));
	memset(&context->transMod, 0, sizeof(TPM_MODIFIER_INDICATOR));
	memset(&context->transSecret, 0, sizeof(TPM_TRANSPORT_AUTH));
	memset(&context->transAuth, 0, sizeof(TPM_AUTH));
	memset(&context->transLogIn, 0, sizeof(TPM_TRANSPORT_LOG_IN));
	memset(&context->transLogOut, 0, sizeof(TPM_TRANSPORT_LOG_OUT));
	memset(&context->transLogDigest, 0, sizeof(TPM_DIGEST));

done_disabled:
	context->flags &= ~TSS_CONTEXT_FLAGS_TRANSPORT_ESTABLISHED;
done:
	obj_list_put(&context_list);

	return result;
}
#endif

/* XXX change 0,1,2 to #defines */
TSS_RESULT
obj_context_set_tpm_version(TSS_HCONTEXT tspContext, UINT32 ver)
{
	TSS_RESULT result = TSS_SUCCESS;
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (ver) {
		case 1:
			context->flags &= ~TSS_CONTEXT_FLAGS_TPM_VERSION_MASK;
			context->flags |= TSS_CONTEXT_FLAGS_TPM_VERSION_1;
			break;
		case 2:
			context->flags &= ~TSS_CONTEXT_FLAGS_TPM_VERSION_MASK;
			context->flags |= TSS_CONTEXT_FLAGS_TPM_VERSION_2;
			break;
		default:
			LogError("Invalid TPM version set: %u", ver);
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			break;
	}

	obj_list_put(&context_list);

	return result;
}

/* XXX change 0,1,2 to #defines */
TSS_RESULT
obj_context_get_tpm_version(TSS_HCONTEXT tspContext, UINT32 *ver)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	if (context->flags & TSS_CONTEXT_FLAGS_TPM_VERSION_1)
		*ver = 1;
	else if (context->flags & TSS_CONTEXT_FLAGS_TPM_VERSION_2)
		*ver = 2;
	else
		*ver = 0;

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

TSS_RESULT
obj_context_get_loadkey_ordinal(TSS_HCONTEXT tspContext, TPM_COMMAND_CODE *ordinal)
{
	struct tsp_object *obj;
	struct tr_context_obj *context;

	if ((obj = obj_list_get_obj(&context_list, tspContext)) == NULL)
		return TSPERR(TSS_E_INVALID_HANDLE);

	context = (struct tr_context_obj *)obj->data;

	switch (context->flags & TSS_CONTEXT_FLAGS_TPM_VERSION_MASK) {
		case TSS_CONTEXT_FLAGS_TPM_VERSION_2:
			*ordinal = TPM_ORD_LoadKey2;
			break;
		default:
			LogDebugFn("No TPM version set!");
			/* fall through */
		case TSS_CONTEXT_FLAGS_TPM_VERSION_1:
			*ordinal = TPM_ORD_LoadKey;
			break;
	}

	obj_list_put(&context_list);

	return TSS_SUCCESS;
}

