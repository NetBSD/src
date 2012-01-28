
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
#include <sys/mman.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


TSS_RESULT
internal_GetCap(TSS_HCONTEXT tspContext, TSS_FLAG capArea, UINT32 subCap,
		UINT32 * respSize, BYTE ** respData)
{
	UINT64 offset = 0;
	TSS_VERSION v = INTERNAL_CAP_VERSION;
	TSS_BOOL bValue = FALSE;
	UINT32 u32value = 0;

	switch (capArea) {
	case TSS_TSPCAP_VERSION:
		if ((*respData = calloc_tspi(tspContext, sizeof(TSS_VERSION))) == NULL) {
			LogError("malloc of %zd bytes failed", sizeof(TSS_VERSION));
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_LoadBlob_TSS_VERSION(&offset, *respData, v);
		*respSize = offset;
		break;
	case TSS_TSPCAP_ALG:
		switch (subCap) {
		case TSS_ALG_RSA:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TSP_ALG_RSA;
			break;
		case TSS_ALG_AES:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TSP_ALG_AES;
			break;
		case TSS_ALG_SHA:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TSP_ALG_SHA;
			break;
		case TSS_ALG_HMAC:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TSP_ALG_HMAC;
			break;
		case TSS_ALG_DES:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TSP_ALG_DES;
			break;
		case TSS_ALG_3DES:
			*respSize = sizeof(TSS_BOOL);
			bValue = INTERNAL_CAP_TSP_ALG_3DES;
			break;
		case TSS_ALG_DEFAULT:
			*respSize = sizeof(UINT32);
			u32value = INTERNAL_CAP_TSP_ALG_DEFAULT;
			break;
		case TSS_ALG_DEFAULT_SIZE:
			*respSize = sizeof(UINT32);
			u32value = INTERNAL_CAP_TSP_ALG_DEFAULT_SIZE;
			break;
		default:
			LogError("Unknown TSP subCap: %u", subCap);
			return TSPERR(TSS_E_BAD_PARAMETER);
		}

		if ((*respData = calloc_tspi(tspContext, *respSize)) == NULL) {
			LogError("malloc of %u bytes failed", *respSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		if (*respSize == sizeof(TSS_BOOL))
			*(TSS_BOOL *)respData = bValue;
		else
			*(UINT32 *)respData = u32value;
		break;
	case TSS_TSPCAP_PERSSTORAGE:
		if ((*respData = calloc_tspi(tspContext, sizeof(TSS_BOOL))) == NULL) {
			LogError("malloc of %zd bytes failed", sizeof(TSS_BOOL));
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		*respSize = sizeof(TSS_BOOL);
		(*respData)[0] = INTERNAL_CAP_TSP_PERSSTORAGE;
		break;
	case TSS_TSPCAP_RETURNVALUE_INFO:
		if (subCap != TSS_TSPCAP_PROP_RETURNVALUE_INFO)
			return TSPERR(TSS_E_BAD_PARAMETER);

		if ((*respData = calloc_tspi(tspContext, sizeof(UINT32))) == NULL) {
			LogError("malloc of %zd bytes failed", sizeof(UINT32));
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		*respSize = sizeof(UINT32);
		*(UINT32 *)(*respData) = INTERNAL_CAP_TSP_RETURNVALUE_INFO;
		break;
	case TSS_TSPCAP_PLATFORM_INFO:
		switch (subCap) {
		case TSS_TSPCAP_PLATFORM_TYPE:
			if ((*respData = calloc_tspi(tspContext, sizeof(UINT32))) == NULL) {
				LogError("malloc of %zd bytes failed", sizeof(UINT32));
				return TSPERR(TSS_E_OUTOFMEMORY);
			}

			*respSize = sizeof(UINT32);
			*(UINT32 *)(*respData) = INTERNAL_CAP_TSP_PLATFORM_TYPE;
			break;
		case TSS_TSPCAP_PLATFORM_VERSION:
			if ((*respData = calloc_tspi(tspContext, sizeof(UINT32))) == NULL) {
				LogError("malloc of %zd bytes failed", sizeof(UINT32));
				return TSPERR(TSS_E_OUTOFMEMORY);
			}

			*respSize = sizeof(UINT32);
			*(UINT32 *)(*respData) = INTERNAL_CAP_TSP_PLATFORM_VERSION;
			break;
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
	case TSS_TSPCAP_MANUFACTURER:
		switch (subCap) {
		case TSS_TSPCAP_PROP_MANUFACTURER_ID:
			if ((*respData = calloc_tspi(tspContext, sizeof(UINT32))) == NULL) {
				LogError("malloc of %zd bytes failed", sizeof(UINT32));
				return TSPERR(TSS_E_OUTOFMEMORY);
			}

			*respSize = sizeof(UINT32);
			*(UINT32 *)(*respData) = INTERNAL_CAP_MANUFACTURER_ID;
			break;
		case TSS_TSPCAP_PROP_MANUFACTURER_STR:
		{
			BYTE str[] = INTERNAL_CAP_MANUFACTURER_STR;

			if ((*respData = calloc_tspi(tspContext,
						     INTERNAL_CAP_MANUFACTURER_STR_LEN)) == NULL) {
				LogError("malloc of %d bytes failed",
					 INTERNAL_CAP_MANUFACTURER_STR_LEN);
				return TSPERR(TSS_E_OUTOFMEMORY);
			}

			*respSize = INTERNAL_CAP_MANUFACTURER_STR_LEN;
			memcpy(*respData, str, INTERNAL_CAP_MANUFACTURER_STR_LEN);
			break;
		}
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
	default:
		return TSPERR(TSS_E_BAD_PARAMETER);
	}

	return TSS_SUCCESS;
}
