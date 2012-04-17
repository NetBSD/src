
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004
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
#include "tcsd_wrap.h"
#include "tcsd.h"

extern struct tcsd_config tcsd_options;

TSS_RESULT
internal_TCSGetCap(TCS_CONTEXT_HANDLE hContext,
		   TCPA_CAPABILITY_AREA capArea,
		   UINT32 subCap,
		   UINT32 * respSize, BYTE ** resp)
{
	UINT32 u32value = 0;
	UINT64 offset;
	TPM_VERSION tcsVersion = INTERNAL_CAP_VERSION;
	struct tcsd_config *config = &tcsd_options;
	struct platform_class *platClass;
	TSS_BOOL bValue = FALSE;

	LogDebug("Checking Software Cap of TCS");
	switch (capArea) {
	case TSS_TCSCAP_ALG:
		LogDebug("TSS_TCSCAP_ALG");

		switch (subCap) {
		case TSS_ALG_RSA:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TCS_ALG_RSA;
			break;
		case TSS_ALG_DES:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TCS_ALG_DES;
			break;
		case TSS_ALG_3DES:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TCS_ALG_3DES;
			break;
		case TSS_ALG_SHA:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TCS_ALG_SHA;
			break;
		case TSS_ALG_AES:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TCS_ALG_AES;
			break;
		case TSS_ALG_HMAC:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TCS_ALG_HMAC;
			break;
		case TSS_ALG_DEFAULT:
			*respSize = sizeof(UINT32);
			u32value = INTERNAL_CAP_TCS_ALG_DEFAULT;
			break;
		case TSS_ALG_DEFAULT_SIZE:
			*respSize = sizeof(UINT32);
			u32value = INTERNAL_CAP_TCS_ALG_DEFAULT_SIZE;
			break;
		default:
			*respSize = 0;
			LogDebugFn("Bad subcap");
			return TCSERR(TSS_E_BAD_PARAMETER);
		}

		if ((*resp = malloc(*respSize)) == NULL) {
			LogError("malloc of %u bytes failed.", *respSize);
			*respSize = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		offset = 0;
		if (*respSize == sizeof(TSS_BOOL))
			*(TSS_BOOL *)(*resp) = bValue;
		else
			*(UINT32 *)(*resp) = u32value;
		break;
	case TSS_TCSCAP_VERSION:
		LogDebug("TSS_TCSCAP_VERSION");
		if ((*resp = calloc(1, sizeof(TSS_VERSION))) == NULL) {
			LogError("malloc of %zd bytes failed.", sizeof(TSS_VERSION));
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		offset = 0;
		LoadBlob_VERSION(&offset, *resp, &tcsVersion);
		*respSize = sizeof(TSS_VERSION);
		break;
	case TSS_TCSCAP_PERSSTORAGE:
		LogDebug("TSS_TCSCAP_PERSSTORAGE");
		if ((*resp = malloc(sizeof(TSS_BOOL))) == NULL) {
			LogError("malloc of %zd byte failed.", sizeof(TSS_BOOL));
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		*(TSS_BOOL *)(*resp) = INTERNAL_CAP_TCS_PERSSTORAGE;
		*respSize = sizeof(TSS_BOOL);
		break;
	case TSS_TCSCAP_CACHING:
		LogDebug("TSS_TCSCAP_CACHING");

		if (subCap == TSS_TCSCAP_PROP_KEYCACHE)
			bValue = INTERNAL_CAP_TCS_CACHING_KEYCACHE;
		else if (subCap == TSS_TCSCAP_PROP_AUTHCACHE)
			bValue = INTERNAL_CAP_TCS_CACHING_AUTHCACHE;
		else {
			LogDebugFn("Bad subcap");
			return TCSERR(TSS_E_BAD_PARAMETER);
		}

		if ((*resp = malloc(sizeof(TSS_BOOL))) == NULL) {
			LogError("malloc of %zd byte failed.", sizeof(TSS_BOOL));
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		*respSize = sizeof(TSS_BOOL);
		*(TSS_BOOL *)(*resp) = bValue;
		break;
	case TSS_TCSCAP_MANUFACTURER:
		if (subCap == TSS_TCSCAP_PROP_MANUFACTURER_ID) {
			if ((*resp = malloc(sizeof(UINT32))) == NULL) {
				LogError("malloc of %zd byte failed.", sizeof(UINT32));
				return TCSERR(TSS_E_OUTOFMEMORY);
			}
			*(UINT32 *)(*resp) = INTERNAL_CAP_MANUFACTURER_ID;
			*respSize = sizeof(UINT32);
		} else if (subCap == TSS_TCSCAP_PROP_MANUFACTURER_STR) {
			BYTE str[] = INTERNAL_CAP_MANUFACTURER_STR;

			if ((*resp = malloc(INTERNAL_CAP_MANUFACTURER_STR_LEN)) == NULL) {
				LogError("malloc of %d bytes failed.",
					 INTERNAL_CAP_MANUFACTURER_STR_LEN);
				return TCSERR(TSS_E_OUTOFMEMORY);
			}
			memcpy(*resp, str, INTERNAL_CAP_MANUFACTURER_STR_LEN);
			*respSize = INTERNAL_CAP_MANUFACTURER_STR_LEN;
		} else {
			LogDebugFn("Bad subcap");
			return TCSERR(TSS_E_BAD_PARAMETER);
		}
		break;
	case TSS_TCSCAP_TRANSPORT:
		/* A zero value here means the TSP is asking whether we support transport sessions
		 * at all */
		if (subCap == TSS_TCSCAP_TRANS_EXCLUSIVE || subCap == 0) {
			*respSize = sizeof(TSS_BOOL);
			if ((*resp = malloc(sizeof(TSS_BOOL))) == NULL) {
				LogError("malloc of %zd byte failed.", sizeof(TSS_BOOL));
				return TCSERR(TSS_E_OUTOFMEMORY);
			}

			if (subCap == TSS_TCSCAP_TRANS_EXCLUSIVE)
				*(TSS_BOOL *)(*resp) = config->exclusive_transport ? TRUE : FALSE;
			else
				*(TSS_BOOL *)(*resp) = TRUE;
		} else {
			LogDebugFn("Bad subcap");
			return TCSERR(TSS_E_BAD_PARAMETER);
		}
		break;
	case TSS_TCSCAP_PLATFORM_CLASS:
		LogDebug("TSS_TCSCAP_PLATFORM_CLASS");

		switch (subCap) {
		case TSS_TCSCAP_PROP_HOST_PLATFORM:
			/* Return the TSS_PLATFORM_CLASS */
			LogDebugFn("TSS_TCSCAP_PROP_HOST_PLATFORM");
			platClass = config->host_platform_class;
			/* Computes the size of host platform structure */
			*respSize = (2 * sizeof(UINT32)) + platClass->classURISize;
			*resp = malloc(*respSize);
			if (*resp == NULL) {
				LogError("malloc of %u bytes failed.", *respSize);
				return TCSERR(TSS_E_OUTOFMEMORY);
			}
			memset(*resp, 0, *respSize);
			offset = 0;
			LoadBlob_UINT32(&offset, platClass->simpleID, *resp);
			LoadBlob_UINT32(&offset, platClass->classURISize, *resp);
			memcpy(&(*resp)[offset], platClass->classURI, platClass->classURISize);
			LogBlob(*respSize, *resp);
			break;
		case TSS_TCSCAP_PROP_ALL_PLATFORMS:
			/* Return an array of TSS_PLATFORM_CLASSes, when existent */
			LogDebugFn("TSS_TCSCAP_PROP_ALL_PLATFORMS");
			*respSize = 0;
			*resp = NULL;
			if ((platClass = config->all_platform_classes) != NULL) {
				/* Computes the size of all Platform Structures */
				while (platClass != NULL) {
					*respSize += (2 * sizeof(UINT32)) + platClass->classURISize;
					platClass = platClass->next;
				}
				*resp = malloc(*respSize);
				if (*resp == NULL) {
					LogError("malloc of %u bytes failed.", *respSize);
					return TCSERR(TSS_E_OUTOFMEMORY);
				}
				memset(*resp, 0, *respSize);
				offset = 0;
				/* Concatenates all the structures on the BYTE * resp */
				platClass = config->all_platform_classes;
				while (platClass != NULL){
					LoadBlob_UINT32(&offset, platClass->simpleID, *resp);
					LoadBlob_UINT32(&offset, platClass->classURISize, *resp);
					memcpy(&(*resp)[offset], platClass->classURI,
					       platClass->classURISize);
					offset += platClass->classURISize;
					platClass = platClass->next;
				}
				LogBlob(*respSize, *resp);
			}
			break;
		default:
			LogDebugFn("Bad subcap");
			return TCSERR(TSS_E_BAD_PARAMETER);
		}
		break;
	default:
		LogDebugFn("Bad cap area");
		return TCSERR(TSS_E_BAD_PARAMETER);
	}

	return TSS_SUCCESS;
}

TSS_RESULT
TCS_GetCapability_Internal(TCS_CONTEXT_HANDLE hContext,	/* in */
			   TCPA_CAPABILITY_AREA capArea,	/* in */
			   UINT32 subCapSize,	/* in */
			   BYTE * subCap,	/* in */
			   UINT32 * respSize,	/* out */
			   BYTE ** resp	/* out */
    )
{
	TSS_RESULT result;
	UINT32 ulSubCap;

	if ((result = ctx_verify_context(hContext)))
		return result;

	if (subCapSize == sizeof(UINT32))
		ulSubCap = *(UINT32 *)subCap;
	else if (subCapSize == 0)
		ulSubCap = 0;
	else
		return TCSERR(TSS_E_BAD_PARAMETER);

	return internal_TCSGetCap(hContext, capArea, ulSubCap, respSize, resp);
}

