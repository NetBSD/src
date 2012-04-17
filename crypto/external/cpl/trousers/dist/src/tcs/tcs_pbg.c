
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
#include <stdarg.h>
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


#define TSS_TPM_RSP_BLOB_AUTH_LEN	(sizeof(TPM_NONCE) + sizeof(TPM_DIGEST) + sizeof(TPM_BOOL))

TSS_RESULT
tpm_rsp_parse(TPM_COMMAND_CODE ordinal, BYTE *b, UINT32 len, ...)
{
	TSS_RESULT result = TSS_SUCCESS;
	UINT64 offset1, offset2;
	va_list ap;

	DBG_ASSERT(ordinal);
	DBG_ASSERT(b);

	va_start(ap, len);

	switch (ordinal) {
	case TPM_ORD_ExecuteTransport:
	{
		UINT32 *val1 = va_arg(ap, UINT32 *);
		UINT32 *val2 = va_arg(ap, UINT32 *);
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (auth1 && auth2) {
			offset1 = offset2 = len - (2 * TSS_TPM_RSP_BLOB_AUTH_LEN);
			UnloadBlob_Auth(&offset1, b, auth1);
			UnloadBlob_Auth(&offset1, b, auth2);
		} else if (auth1) {
			offset1 = offset2 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		} else if (auth2) {
			offset1 = offset2 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth2);
		} else
			offset2 = len;

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		if (val1)
			UnloadBlob_UINT32(&offset1, val1, b);
		if (val2)
			UnloadBlob_UINT32(&offset1, val2, b);

		*len1 = offset2 - offset1;
		if (*len1) {
			if ((*blob1 = malloc(*len1)) == NULL) {
				LogError("malloc of %u bytes failed", *len1);
				return TCSERR(TSS_E_OUTOFMEMORY);
			}
			UnloadBlob(&offset1, *len1, b, *blob1);
		} else
			*blob1 = NULL;

		break;
	}
#ifdef TSS_BUILD_TICK
	/* TPM BLOB: TPM_CURRENT_TICKS, UINT32, BLOB, optional AUTH */
	case TPM_ORD_TickStampBlob:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		UINT32 *len2 = va_arg(ap, UINT32 *);
		BYTE **blob2 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!len1 || !blob1 || !len2 || !blob2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_CURRENT_TICKS(&offset2, b, NULL);
		*len1 = (UINT32)offset2 - offset1;

		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, *len1, b, *blob1);
                UnloadBlob_UINT32(&offset1, len2, b);

		if ((*blob2 = malloc(*len2)) == NULL) {
			LogError("malloc of %u bytes failed", *len2);
			free(*blob1);
			*blob1 = NULL;
			*len1 = 0;
			*len2 = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len2, b, *blob2);

		break;
	}
#endif
#ifdef TSS_BUILD_QUOTE
	/* TPM BLOB: TPM_PCR_COMPOSITE, UINT32, BLOB, 1 optional AUTH
	 * return UINT32*, BYTE**, UINT32*, BYTE**, 1 optional AUTH */
	case TPM_ORD_Quote:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		UINT32 *len2 = va_arg(ap, UINT32 *);
		BYTE **blob2 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!len1 || !blob1 || !len2 || !blob2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_PCR_COMPOSITE(&offset2, b, NULL);
		*len1 = offset2 - offset1;

		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len1, b, *blob1);
                UnloadBlob_UINT32(&offset1, len2, b);

		if ((*blob2 = malloc(*len2)) == NULL) {
			LogError("malloc of %u bytes failed", *len2);
			free(*blob1);
			*blob1 = NULL;
			*len1 = 0;
			*len2 = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len2, b, *blob2);

		break;
	}
#endif
#ifdef TSS_BUILD_TSS12
	/* TPM BLOB: TPM_PCR_INFO_SHORT, (UINT32, BLOB,) UINT32, BLOB, 1 optional AUTH */
	case TPM_ORD_Quote2:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *); /* pcrDataSizeOut */
		BYTE **blob1 = va_arg(ap, BYTE **);  /* pcrDataOut */
		TSS_BOOL *addVersion = va_arg(ap, TSS_BOOL *); /* addVersion */
		UINT32 *len2 = va_arg(ap, UINT32 *); /* versionInfoSize */
		BYTE **blob2 = va_arg(ap, BYTE **);  /* versionInfo */
		UINT32 *len3 = va_arg(ap, UINT32 *); /* sigSize */
		BYTE **blob3 = va_arg(ap, BYTE **);  /* sig */
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *); /* privAuth */
		va_end(ap);

		if (!len1 || !blob1 || !len2 || !blob2 || !len3 || !blob3 || !addVersion) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		/* Adjust the offset to take the TPM_PCR_INFO_SHORT size:
		 * need to allocate this size into blob1
		 */
		UnloadBlob_PCR_INFO_SHORT(&offset2, b, NULL);

		/* Get the size of the TSS_TPM_INFO_SHORT
		 * and copy it into blob1 */
		*len1 = offset2 - offset1;
		LogDebugFn("QUOTE2 Core: PCR_INFO_SHORT is %u size", *len1);
		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len1, b, *blob1); /* TPM_PCR_INFO_SHORT */

		UnloadBlob_UINT32(&offset1, len2,b); /* versionInfoSize */
		LogDebugFn("QUOTE2 Core: versionInfoSize=%u", *len2);
		if ((*blob2 = malloc(*len2)) == NULL) {
			LogError("malloc of %u bytes failed", *len2);
			free(*blob1);
			*blob1 = NULL;
			*len1 = 0;
			*len2 = 0;
			*len3 = 0;
			*blob3 = NULL;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len2, b, *blob2);

		/* Take the sigSize */
		UnloadBlob_UINT32(&offset1, len3, b);
		LogDebugFn("QUOTE2 Core: sigSize=%u", *len3);
		/* sig */
		if ((*blob3 = malloc(*len3)) == NULL) {
			LogError("malloc of %u bytes failed", *len3);
			free(*blob1);
			*blob1 = NULL;
			if (*len2 > 0){
				free(*blob2);
				*blob2 = NULL;
			}
			*len1 = 0;
			*len2 = 0;
			*len3 = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len3, b, *blob3);
		break;
	}
#endif
	/* TPM BLOB: TPM_CERTIFY_INFO, UINT32, BLOB, 2 optional AUTHs
	 * return UINT32*, BYTE**, UINT32*, BYTE**, 2 optional AUTHs */
	case TPM_ORD_CertifyKey:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		UINT32 *len2 = va_arg(ap, UINT32 *);
		BYTE **blob2 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!len1 || !blob1 || !len2 || !blob2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1 && auth2) {
			offset1 = len - (2 * TSS_TPM_RSP_BLOB_AUTH_LEN);
			UnloadBlob_Auth(&offset1, b, auth1);
			UnloadBlob_Auth(&offset1, b, auth2);
		} else if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		} else if (auth2) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth2);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_CERTIFY_INFO(&offset2, b, NULL);
		*len1 = offset2 - offset1;

		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len1, b, *blob1);
                UnloadBlob_UINT32(&offset1, len2, b);

		if ((*blob2 = malloc(*len2)) == NULL) {
			LogError("malloc of %u bytes failed", *len2);
			free(*blob1);
			*blob1 = NULL;
			*len1 = 0;
			*len2 = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len2, b, *blob2);

		break;
	}
#ifdef TSS_BUILD_AUDIT
	/* TPM_BLOB: TPM_COUNTER_VALUE, DIGEST, DIGEST, UINT32, BLOB, optional AUTH
	 * return: UINT32*, BYTE**, DIGEST*, DIGEST*, UINT32*, BYTE**, optional AUTH */
	case TPM_ORD_GetAuditDigestSigned:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		TPM_DIGEST *digest1 = va_arg(ap, TPM_DIGEST *);
		TPM_DIGEST *digest2 = va_arg(ap, TPM_DIGEST *);
		UINT32 *len2 = va_arg(ap, UINT32 *);
		BYTE **blob2 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !digest2 || !len1 || !blob1 || !len2 || !blob2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_COUNTER_VALUE(&offset2, b, NULL);
		*len1 = offset2 - offset1;

		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len1, b, *blob1);

		UnloadBlob_DIGEST(&offset1, b, digest1);
		UnloadBlob_DIGEST(&offset1, b, digest2);
                UnloadBlob_UINT32(&offset1, len2, b);

		if ((*blob2 = malloc(*len2)) == NULL) {
			LogError("malloc of %u bytes failed", *len2);
			free(*blob1);
			*blob1 = NULL;
			*len1 = 0;
			*len2 = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len2, b, *blob2);

		if (auth1)
			UnloadBlob_Auth(&offset1, b, auth1);

		break;
	}
	/* TPM_BLOB: TPM_COUNTER_VALUE, DIGEST, BOOL, UINT32, BLOB
	 * return: DIGEST*, UINT32*, BYTE**, BOOL, UINT32*, BYTE** */
	case TPM_ORD_GetAuditDigest:
	{
		TPM_DIGEST *digest1 = va_arg(ap, TPM_DIGEST *);
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		TSS_BOOL *bool1 = va_arg(ap, TSS_BOOL *);
		UINT32 *len2 = va_arg(ap, UINT32 *);
		BYTE **blob2 = va_arg(ap, BYTE **);
		va_end(ap);

		if (!digest1 || !len1 || !blob1 || !len2 || !blob2 || !bool1) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_COUNTER_VALUE(&offset2, b, NULL);
		*len1 = offset2 - offset1;

		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len1, b, *blob1);

		UnloadBlob_DIGEST(&offset1, b, digest1);
                UnloadBlob_BOOL(&offset1, bool1, b);
                UnloadBlob_UINT32(&offset1, len2, b);

		if ((*blob2 = malloc(*len2)) == NULL) {
			LogError("malloc of %u bytes failed", *len2);
			free(*blob1);
			*blob1 = NULL;
			*len1 = 0;
			*len2 = 0;
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		UnloadBlob(&offset1, *len2, b, *blob2);

		break;
	}
#endif
#ifdef TSS_BUILD_COUNTER
	/* optional UINT32, TPM_COUNTER_VALUE, optional AUTH */
	case TPM_ORD_ReadCounter:
	case TPM_ORD_CreateCounter:
	case TPM_ORD_IncrementCounter:
	{
		UINT32 *val1 = va_arg(ap, UINT32 *);
		TPM_COUNTER_VALUE *ctr = va_arg(ap, TPM_COUNTER_VALUE *);
		TPM_AUTH * auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!ctr) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		if (val1)
			UnloadBlob_UINT32(&offset1, val1, b);
		UnloadBlob_COUNTER_VALUE(&offset1, b, ctr);

		break;
	}
#endif
	/* TPM BLOB: UINT32, BLOB, UINT32, BLOB, optional AUTH, optional AUTH */
	case TPM_ORD_CreateMaintenanceArchive:
	case TPM_ORD_CreateMigrationBlob:
	case TPM_ORD_Delegate_ReadTable:
	case TPM_ORD_CMK_CreateBlob:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		UINT32 *len2 = va_arg(ap, UINT32 *);
		BYTE **blob2 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!len1 || !blob1 || !len2 || !blob2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1 && auth2) {
			offset1 = len - (2 * TSS_TPM_RSP_BLOB_AUTH_LEN);
			UnloadBlob_Auth(&offset1, b, auth1);
			UnloadBlob_Auth(&offset1, b, auth2);
		} else if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		} else if (auth2) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth2);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_UINT32(&offset1, len1, b);
		if ((*blob1 = malloc(*len1)) == NULL) {
			LogError("malloc of %u bytes failed", *len1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, *len1, b, *blob1);

		UnloadBlob_UINT32(&offset1, len2, b);
		if ((*blob2 = malloc(*len2)) == NULL) {
			free(*blob1);
			LogError("malloc of %u bytes failed", *len2);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, *len2, b, *blob2);

		break;
	}
	/* TPM BLOB: BLOB, optional AUTH, AUTH
	 * return:   UINT32 *, BYTE **, optional AUTH, AUTH */
	case TPM_ORD_ActivateIdentity:
	{
		UINT32 *len1 = va_arg(ap, UINT32 *);
		BYTE **blob1 = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!len1 || !blob1 || !auth2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1 && auth2) {
			offset1 = offset2 = len - (2 * TSS_TPM_RSP_BLOB_AUTH_LEN);
			UnloadBlob_Auth(&offset1, b, auth1);
			UnloadBlob_Auth(&offset1, b, auth2);
		} else if (auth2) {
			offset1 = offset2 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth2);
		} else
			offset2 = len;

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		offset2 -= TSS_TPM_TXBLOB_HDR_LEN;
		if ((*blob1 = malloc(offset2)) == NULL) {
			LogError("malloc of %zd bytes failed", (size_t)offset2);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		*len1 = offset2;
		UnloadBlob(&offset1, *len1, b, *blob1);

		break;
	}
	/* TPM BLOB: TPM_KEY, UINT32, BLOB, optional AUTH, AUTH
	 * return:   UINT32 *, BYTE **, UINT32 *, BYTE **, optional AUTH, AUTH */
	case TPM_ORD_MakeIdentity:
	{
		UINT32 *len1, *len2;
		BYTE **blob1, **blob2;
		TPM_AUTH *auth1, *auth2;

		len1 = va_arg(ap, UINT32 *);
		blob1 = va_arg(ap, BYTE **);
		len2 = va_arg(ap, UINT32 *);
		blob2 = va_arg(ap, BYTE **);
		auth1 = va_arg(ap, TPM_AUTH *);
		auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!len1 || !blob1 || !len2 || !blob2 || !auth2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset1 = offset2 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_TSS_KEY(&offset1, b, NULL);
		offset1 -= TSS_TPM_TXBLOB_HDR_LEN;

		if ((*blob1 = malloc(offset1)) == NULL) {
			LogError("malloc of %zd bytes failed", (size_t)offset1);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}
		*len1 = offset1;

		UnloadBlob(&offset2, offset1, b, *blob1);

		/* offset2 points to the stuff after the key */
		UnloadBlob_UINT32(&offset2, len2, b);

		if ((*blob2 = malloc(*len2)) == NULL) {
			free(*blob1);
			LogError("malloc of %u bytes failed", *len2);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset2, *len2, b, *blob2);

		if (auth1)
			UnloadBlob_Auth(&offset2, b, auth1);
		UnloadBlob_Auth(&offset2, b, auth2);

		break;
	}
	/* 1 TPM_VERSION, 2 UINT32s, 1 optional AUTH */
	case TPM_ORD_GetCapabilityOwner:
	{
		TPM_VERSION *ver1 = va_arg(ap, TPM_VERSION *);
		UINT32 *data1 = va_arg(ap, UINT32 *);
		UINT32 *data2 = va_arg(ap, UINT32 *);
		TPM_AUTH *auth = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!data1 || !data2 || !ver1) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_VERSION(&offset1, b, ver1);
		UnloadBlob_UINT32(&offset1, data1, b);
		UnloadBlob_UINT32(&offset1, data2, b);
		break;
	}
	/* TPM BLOB: 1 UINT32, 1 BLOB, 2 optional AUTHs
	 * return: UINT32 *, BYTE**, 2 optional AUTHs */
	case TPM_ORD_Sign:
	case TPM_ORD_GetTestResult:
	case TPM_ORD_CertifySelfTest:
	case TPM_ORD_Unseal:
	case TPM_ORD_GetRandom:
	case TPM_ORD_DAA_Join:
	case TPM_ORD_DAA_Sign:
	case TPM_ORD_ChangeAuth:
	case TPM_ORD_GetCapability:
	case TPM_ORD_LoadMaintenanceArchive:
	case TPM_ORD_ConvertMigrationBlob:
	case TPM_ORD_NV_ReadValue:
	case TPM_ORD_NV_ReadValueAuth:
	case TPM_ORD_Delegate_Manage:
	case TPM_ORD_Delegate_CreateKeyDelegation:
	case TPM_ORD_Delegate_CreateOwnerDelegation:
	case TPM_ORD_Delegate_UpdateVerification:
	case TPM_ORD_CMK_ConvertMigration:
	{
		UINT32 *data_len = va_arg(ap, UINT32 *);
		BYTE **data = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!data || !data_len) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1 && auth2) {
			offset1 = len - (2 * TSS_TPM_RSP_BLOB_AUTH_LEN);
			UnloadBlob_Auth(&offset1, b, auth1);
			UnloadBlob_Auth(&offset1, b, auth2);
		} else if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		} else if (auth2) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth2);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_UINT32(&offset1, data_len, b);
		if ((*data = malloc(*data_len)) == NULL) {
			LogError("malloc of %u bytes failed", *data_len);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, *data_len, b, *data);
		break;
	}
	/* TPM BLOB: 1 UINT32, 1 BLOB, 1 optional AUTH
	* return: UINT32 *, BYTE**, 1 optional AUTH*/
	case TPM_ORD_UnBind:
	{
		UINT32 *data_len = va_arg(ap, UINT32 *);
		BYTE **data = va_arg(ap, BYTE **);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!data || !data_len) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth1) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth1);
		} 

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_UINT32(&offset1, data_len, b);
		if ((*data = malloc(*data_len)) == NULL) {
			LogError("malloc of %u bytes failed", *data_len);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, *data_len, b, *data);
		break;
	}
	/* TPM BLOB: 1 BLOB, 1 optional AUTH
	 * return: UINT32 *, BYTE**, 1 optional AUTH*/
	case TPM_ORD_GetTicks:
	case TPM_ORD_Seal:
	case TPM_ORD_Sealx:
	case TPM_ORD_FieldUpgrade:
	case TPM_ORD_CreateWrapKey:
	case TPM_ORD_GetPubKey:
	case TPM_ORD_OwnerReadPubek:
	case TPM_ORD_OwnerReadInternalPub:
	case TPM_ORD_AuthorizeMigrationKey:
	case TPM_ORD_TakeOwnership:
	case TPM_ORD_CMK_CreateKey:
	{
		UINT32 *data_len = va_arg(ap, UINT32 *);
		BYTE **data = va_arg(ap, BYTE **);
		TPM_AUTH *auth = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!data || !data_len) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		/* remove the auth data from the back end of the data */
		if (auth) {
			offset1 = offset2 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth);
		} else
			offset2 = len;

		/* everything after the header is returned as the blob */
		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		offset2 -= offset1;
		if ((*data = malloc((size_t)offset2)) == NULL) {
			LogError("malloc of %zd bytes failed", (size_t)offset2);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		memcpy(*data, &b[offset1], offset2);
		*data_len = offset2;
		break;
	}
	/* TPM BLOB: BLOB, optional DIGEST */
	case TPM_ORD_CreateEndorsementKeyPair:
	case TPM_ORD_ReadPubek:
	{
		UINT32 *data_len = va_arg(ap, UINT32 *);
		BYTE **data = va_arg(ap, BYTE **);
		BYTE *digest1 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!data || !data_len) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (digest1) {
			offset1 = offset2 = len - TPM_DIGEST_SIZE;
			memcpy(digest1, &b[offset2], TPM_DIGEST_SIZE);
		} else
			offset2 = len;

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		offset2 -= offset1;
		if ((*data = malloc((size_t)offset2)) == NULL) {
			LogError("malloc of %zd bytes failed", (size_t)offset2);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, offset2, b, *data);
		*data_len = offset2;
		break;
	}
#ifdef TSS_BUILD_TSS12
	/* TPM BLOB: BLOB, DIGEST, DIGEST
	 * return: UINT32 *, BYTE**, DIGEST, DIGEST */
	case TPM_ORD_CreateRevocableEK:
	{
		UINT32 *data_len = va_arg(ap, UINT32 *);
		BYTE **data = va_arg(ap, BYTE **);
		BYTE *digest1 = va_arg(ap, BYTE *);
		BYTE *digest2 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!data || !data_len || !digest1 || !digest2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset2 = len - TPM_DIGEST_SIZE;
		memcpy(digest2, &b[offset2], TPM_DIGEST_SIZE);

		offset2 -= TPM_DIGEST_SIZE;
		memcpy(digest1, &b[offset2], TPM_DIGEST_SIZE);

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		offset2 -= offset1;
		if ((*data = malloc((size_t)offset2)) == NULL) {
			LogError("malloc of %zd bytes failed", (size_t)offset2);
			return TCSERR(TSS_E_OUTOFMEMORY);
		}

		UnloadBlob(&offset1, offset2, b, *data);
		*data_len = offset2;
		break;
	}
#endif
	/* 1 UINT32, 1 optional AUTH */
	case TPM_ORD_LoadKey:
	case TPM_ORD_LoadKey2:
	{
		UINT32 *handle;
		TPM_AUTH *auth;

		handle = va_arg(ap, UINT32 *);
		auth = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!handle) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		if (auth) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_UINT32(&offset1, handle, b);
		break;
	}
	/* 1 optional UINT32, 1 20 byte value */
	case TPM_ORD_DirRead:
	case TPM_ORD_OIAP:
	case TPM_ORD_LoadManuMaintPub:
	case TPM_ORD_ReadManuMaintPub:
	case TPM_ORD_Extend:
	case TPM_ORD_PcrRead:
	{
		UINT32 *handle = va_arg(ap, UINT32 *);
		BYTE *nonce = va_arg(ap, BYTE *);
		va_end(ap);

		if (!nonce) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		if (handle)
			UnloadBlob_UINT32(&offset1, handle, b);
		UnloadBlob(&offset1, TPM_NONCE_SIZE, b, nonce);
		break;
	}
	/* 1 UINT32, 2 20 byte values */
	case TPM_ORD_OSAP:
	case TPM_ORD_DSAP:
	{
		UINT32 *handle = va_arg(ap, UINT32 *);
		BYTE *nonce1 = va_arg(ap, BYTE *);
		BYTE *nonce2 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!handle || !nonce1 || !nonce2) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob_UINT32(&offset1, handle, b);
		UnloadBlob(&offset1, TPM_NONCE_SIZE, b, nonce1);
		UnloadBlob(&offset1, TPM_NONCE_SIZE, b, nonce2);
		break;
	}
#ifdef TSS_BUILD_CMK
	/* 1 20 byte value, 1 optional AUTH */
	case TPM_ORD_CMK_ApproveMA:
	case TPM_ORD_CMK_CreateTicket:
	{
		BYTE *hmac1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!hmac1) {
			LogError("Internal error for ordinal 0x%x", ordinal);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}

		offset1 = TSS_TPM_TXBLOB_HDR_LEN;
		UnloadBlob(&offset1, TPM_SHA1_160_HASH_LEN, b, hmac1);
		if (auth) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth);
		}
		break;
	}
#endif
	/* 1 optional AUTH */
	case TPM_ORD_DisablePubekRead:
	case TPM_ORD_DirWriteAuth:
	case TPM_ORD_ReleaseCounter:
	case TPM_ORD_ReleaseCounterOwner:
	case TPM_ORD_ChangeAuthOwner:
	case TPM_ORD_SetCapability:
	case TPM_ORD_SetOrdinalAuditStatus:
	case TPM_ORD_ResetLockValue:
	case TPM_ORD_SetRedirection:
	case TPM_ORD_DisableOwnerClear:
	case TPM_ORD_OwnerSetDisable:
	case TPM_ORD_SetTempDeactivated:
	case TPM_ORD_KillMaintenanceFeature:
	case TPM_ORD_NV_DefineSpace:
	case TPM_ORD_NV_WriteValue:
	case TPM_ORD_NV_WriteValueAuth:
	case TPM_ORD_OwnerClear:
	case TPM_ORD_Delegate_LoadOwnerDelegation:
	case TPM_ORD_CMK_SetRestrictions:
	case TPM_ORD_FlushSpecific:
	case TPM_ORD_KeyControlOwner:
	{
		TPM_AUTH *auth = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (auth) {
			offset1 = len - TSS_TPM_RSP_BLOB_AUTH_LEN;
			UnloadBlob_Auth(&offset1, b, auth);
		}
		break;
	}
	default:
		LogError("Unknown ordinal: 0x%x", ordinal);
		result = TCSERR(TSS_E_INTERNAL_ERROR);
		break;
	}

	return result;
}

/* XXX optimize these cases by always passing in lengths for blobs, no more "20 byte values" */
TSS_RESULT
tpm_rqu_build(TPM_COMMAND_CODE ordinal, UINT64 *outOffset, BYTE *out_blob, ...)
{
	TSS_RESULT result = TSS_SUCCESS;
	UINT64 blob_size;
	va_list ap;

	DBG_ASSERT(ordinal);
	DBG_ASSERT(outOffset);
	DBG_ASSERT(out_blob);

	va_start(ap, out_blob);

	switch (ordinal) {
#ifdef TSS_BUILD_DELEGATION
	/* 1 UINT16, 1 UINT32, 1 20 bytes value, 1 UINT32, 1 BLOB */
	case TPM_ORD_DSAP:
	{
		UINT16 val1 = va_arg(ap, int);
		UINT32 handle1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!digest1 || !in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT16(outOffset, val1, out_blob);
		LoadBlob_UINT32(outOffset, handle1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 1 BOOL, 1 UINT32, 1 BLOB, 1 20 byte value, 1 AUTH */
	case TPM_ORD_Delegate_CreateOwnerDelegation:
	{
		TSS_BOOL bool1 = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		BYTE *digest1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!in_len1 || !in_blob1 || !digest1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_BOOL(outOffset, bool1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 2 UINT32's, 1 BLOB, 1 20 byte value, 1 AUTH */
	case TPM_ORD_Delegate_CreateKeyDelegation:
	{
		UINT32 keyslot1 = va_arg(ap, UINT32);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		BYTE *digest1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keyslot1 || !in_len1 || !in_blob1 || !digest1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keyslot1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
#ifdef TSS_BUILD_TRANSPORT
	/* 3 UINT32's, 1 BLOB, 2 AUTHs */
	case TPM_ORD_ExecuteTransport:
	{
		UINT32 ord1 = va_arg(ap, UINT32);
		UINT32 *keyslot1 = va_arg(ap, UINT32 *);
		UINT32 *keyslot2 = va_arg(ap, UINT32 *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		if (keyslot1)
			LoadBlob_UINT32(outOffset, *keyslot1, out_blob);
		if (keyslot2)
			LoadBlob_UINT32(outOffset, *keyslot2, out_blob);
		//LoadBlob_UINT32(outOffset, in_len1, out_blob);
		if (in_blob1)
			LoadBlob(outOffset, in_len1, out_blob, in_blob1);

		if (auth1 && auth2) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ord1, out_blob);
		} else if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ord1, out_blob);
		} else if (auth2) {
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ord1, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ord1, out_blob);
		}

		break;
	}
#endif
	/* 1 UINT32, 1 UINT16, 1 BLOB, 1 UINT32, 1 BLOB, 1 options AUTH, 1 AUTH */
	case TPM_ORD_CreateMigrationBlob:
	{
		UINT32 keyslot1 = va_arg(ap, UINT32);
		UINT16 type1 = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!in_blob1 || !in_blob2 || !auth2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keyslot1, out_blob);
		LoadBlob_UINT16(outOffset, type1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT32, 1 UINT16, 1 20 byte value, 1 UINT16, 1 UINT32, 1 BLOB, 2 AUTHs */
	case TPM_ORD_ChangeAuth:
	{
		UINT32 keyslot1 = va_arg(ap, UINT32);
		UINT16 proto1 = va_arg(ap, int);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT16 entity1 = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !in_blob1 || !auth1 || !auth2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keyslot1, out_blob);
		LoadBlob_UINT16(outOffset, proto1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_UINT16(outOffset, entity1, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Auth(outOffset, out_blob, auth2);
		LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 2 DIGEST/ENCAUTH's, 1 UINT32, 1 BLOB, 1 optional AUTH, 1 AUTH */
	case TPM_ORD_MakeIdentity:
	{
		BYTE *dig1, *dig2, *blob1;
		UINT32 len1;
		TPM_AUTH *auth1, *auth2;

		dig1 = va_arg(ap, BYTE *);
		dig2 = va_arg(ap, BYTE *);
		len1 = va_arg(ap, UINT32);
		blob1 = va_arg(ap, BYTE *);
		auth1 = va_arg(ap, TPM_AUTH *);
		auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!dig1 || !dig2 || !blob1 || !auth2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, dig1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, dig2);
		LoadBlob(outOffset, len1, out_blob, blob1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#if (TSS_BUILD_NV || TSS_BUILD_DELEGATION)
	/* 3 UINT32's, 1 BLOB, 1 optional AUTH */
	case TPM_ORD_NV_WriteValue:
	case TPM_ORD_NV_WriteValueAuth:
	case TPM_ORD_Delegate_Manage:
	{
		UINT32 i = va_arg(ap, UINT32);
		UINT32 j = va_arg(ap, UINT32);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, i, out_blob);
		LoadBlob_UINT32(outOffset, j, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#endif
	/* 3 UINT32's, 1 optional AUTH */
	case TPM_ORD_NV_ReadValue:
	case TPM_ORD_NV_ReadValueAuth:
	case TPM_ORD_SetRedirection:
	{
		UINT32 i = va_arg(ap, UINT32);
		UINT32 j = va_arg(ap, UINT32);
		UINT32 k = va_arg(ap, UINT32);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, i, out_blob);
		LoadBlob_UINT32(outOffset, j, out_blob);
		LoadBlob_UINT32(outOffset, k, out_blob);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 20 byte value, 1 UINT32, 1 BLOB */
	case TPM_ORD_CreateEndorsementKeyPair:
	{
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!digest1 || !in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#ifdef TSS_BUILD_TSS12
	/* 1 20 byte value, 1 UINT32, 1 BLOB, 1 BOOL, 1 20 byte value */
	case TPM_ORD_CreateRevocableEK:
	{
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TSS_BOOL in_bool1 = va_arg(ap, int);
		BYTE *digest2 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!digest1 || !in_blob1 || !digest2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_BOOL(outOffset, in_bool1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest2);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 1 20 byte value */
	case TPM_ORD_RevokeTrust:
	{
		BYTE *digest1 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!digest1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
#ifdef TSS_BUILD_COUNTER
	/* 1 20 byte value, 1 UINT32, 1 BLOB, 1 AUTH */
	case TPM_ORD_CreateCounter:
	{
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !in_blob1 || !auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
#ifdef TSS_BUILD_DAA
	/* 1 UINT32, 1 BYTE, 1 UINT32, 1 BLOB, 1 UINT32, 1 BLOB, 1 AUTH */
	case TPM_ORD_DAA_Sign:
	case TPM_ORD_DAA_Join:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		BYTE stage1 = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !in_blob1 || !auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob_BOOL(outOffset, stage1, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
	/* 2 UINT32's, 1 BLOB, 1 UINT32, 1 BLOB, 1 optional AUTH */
	case TPM_ORD_ConvertMigrationBlob:
	case TPM_ORD_SetCapability:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !in_blob1 || !in_blob2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 2 UINT32's, 1 20 byte value, 2 optional AUTHs */
	case TPM_ORD_CertifyKey:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		UINT32 keySlot2 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !keySlot2 || !digest1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob_UINT32(outOffset, keySlot2, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		if (auth1 && auth2) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ordinal, out_blob);
		} else if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else if (auth2) {
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 2 UINT32's, 1 BLOB, 1 optional AUTH */
	case TPM_ORD_Delegate_LoadOwnerDelegation:
	case TPM_ORD_GetCapability:
	case TPM_ORD_UnBind:
	case TPM_ORD_Sign:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (in_len1 && !in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		if (in_len1)
			LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT32, 1 20 byte value, 1 UINT32, 1 optional BLOB, 1 UINT32, 1 BLOB, 1 AUTH */
	case TPM_ORD_Seal:
	case TPM_ORD_Sealx:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		blob_size = in_len1 + in_len2 + TPM_DIGEST_SIZE + sizeof(TPM_AUTH);
		if (blob_size > TSS_TPM_TXBLOB_SIZE) {
			result = TCSERR(TSS_E_BAD_PARAMETER);
			LogError("Oversized input when building ordinal 0x%x", ordinal);
			break;
		}
				
		if (!keySlot1 || !in_blob2 || !auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 2 UINT32's, 1 BLOB, 1 optional AUTH, 1 AUTH */
	case TPM_ORD_ActivateIdentity:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !in_blob1 || !auth2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT32, 1 20-byte blob, 1 BLOB, 1 optional AUTH */
	case TPM_ORD_Quote:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !digest1 || !in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);

		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#ifdef TSS_BUILD_TSS12
	/* 1 UINT32, 1 20-byte blob, 1 BLOB, 1 BOOL, 1 optional AUTH */
	case TPM_ORD_Quote2:
	{
		/* Input vars */
		UINT32 keySlot1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TSS_BOOL* addVersion = va_arg(ap,TSS_BOOL *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !digest1 || !in_blob1 || !addVersion) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);

		/* Load the addVersion Bool */
		LoadBlob_BOOL(outOffset,*addVersion,out_blob);

		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
	/* 1 UINT32, 2 20-byte blobs, 1 BLOB, 1 optional AUTH */
	case TPM_ORD_CreateWrapKey:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		BYTE *digest2 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !digest1 || !digest2 || !in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest2);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 2 BLOBs, 1 optional AUTH */
	case TPM_ORD_NV_DefineSpace:
	case TPM_ORD_LoadManuMaintPub:
	{
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!in_blob1 || !in_blob2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#ifdef TSS_BUILD_TICK
	/* 1 UINT32, 2 20-byte blobs, 1 optional AUTH */
	case TPM_ORD_TickStampBlob:
	{
		UINT32 keySlot1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		BYTE *digest2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!keySlot1 || !digest1 || !digest2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keySlot1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest2);

		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
	/* 1 BLOB */
	case TPM_ORD_ReadManuMaintPub:
	case TPM_ORD_ReadPubek:
	case TPM_ORD_PCR_Reset:
	case TPM_ORD_SetOperatorAuth:
	{
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 1 UINT32, 1 BLOB, 2 optional AUTHs */
	case TPM_ORD_LoadKey:
	case TPM_ORD_LoadKey2:
	case TPM_ORD_DirWriteAuth:
	case TPM_ORD_CertifySelfTest:
	case TPM_ORD_Unseal:
	case TPM_ORD_Extend:
	case TPM_ORD_StirRandom:
	case TPM_ORD_LoadMaintenanceArchive: /* XXX */
	case TPM_ORD_FieldUpgrade:
	case TPM_ORD_Delegate_UpdateVerification:
	case TPM_ORD_Delegate_VerifyDelegation:
	{
		UINT32 val1 = va_arg(ap, UINT32);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		TPM_AUTH *auth2 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (in_len1 && !in_blob1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, val1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		if (auth1 && auth2) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH2_COMMAND, *outOffset, ordinal, out_blob);
		} else if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else if (auth2) {
			LoadBlob_Auth(outOffset, out_blob, auth2);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT16, 1 BLOB, 1 AUTH */
	case TPM_ORD_AuthorizeMigrationKey:
	{
		UINT16 scheme1 = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!in_blob1 || !auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT16(outOffset, scheme1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 1 UINT16, 1 UINT32, 1 BLOB, 1 UINT32, 2 BLOBs, 1 AUTH */
	case TPM_ORD_TakeOwnership:
	{
		UINT16 scheme1 = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		UINT32 in_len3 = va_arg(ap, UINT32);
		BYTE *in_blob3 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!in_blob1 || !in_blob2 || !in_blob3 || !auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT16(outOffset, scheme1, out_blob);
		LoadBlob_UINT32(outOffset, in_len1, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		LoadBlob(outOffset, in_len3, out_blob, in_blob3);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#ifdef TSS_BUILD_AUDIT
	/* 1 UINT32, 1 BOOL, 1 20 byte value, 1 optional AUTH */
	case TPM_ORD_GetAuditDigestSigned:
	{
		UINT32 keyslot1 = va_arg(ap, UINT32);
		TSS_BOOL bool1 = va_arg(ap, int);
		BYTE *digest1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, keyslot1, out_blob);
		LoadBlob_BOOL(outOffset, bool1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);

		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#endif
	/* 1 UINT16, 1 UINT32, 1 20 byte value */
	case TPM_ORD_OSAP:
	{
		UINT16 type1 = va_arg(ap, int);
		UINT32 value1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		va_end(ap);

		if (!digest1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT16(outOffset, type1, out_blob);
		LoadBlob_UINT32(outOffset, value1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 1 UINT16, 1 20 byte value, 1 UINT16, 1 AUTH */
	case TPM_ORD_ChangeAuthOwner:
	{
		UINT16 type1 = va_arg(ap, int);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT16 type2 = va_arg(ap, int);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT16(outOffset, type1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_UINT16(outOffset, type2, out_blob);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#ifdef TSS_BUILD_AUDIT
	/* 1 UINT32, 1 BOOL, 1 AUTH */
	case TPM_ORD_SetOrdinalAuditStatus:
	{
		UINT32 ord1 = va_arg(ap, UINT32);
		TSS_BOOL bool1 = va_arg(ap, int);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!auth1) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, ord1, out_blob);
		LoadBlob_BOOL(outOffset, bool1, out_blob);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
	/* 1 BOOL, 1 optional AUTH */
	case TPM_ORD_OwnerSetDisable:
	case TPM_ORD_PhysicalSetDeactivated:
	case TPM_ORD_CreateMaintenanceArchive:
	case TPM_ORD_SetOwnerInstall:
	{
		TSS_BOOL bool1 = va_arg(ap, int);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_BOOL(outOffset, bool1, out_blob);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 optional AUTH */
	case TPM_ORD_OwnerClear:
	case TPM_ORD_DisablePubekRead:
	case TPM_ORD_GetCapabilityOwner:
	case TPM_ORD_ResetLockValue:
	case TPM_ORD_DisableOwnerClear:
	case TPM_ORD_SetTempDeactivated:
	case TPM_ORD_OIAP:
	case TPM_ORD_OwnerReadPubek:
	case TPM_ORD_SelfTestFull:
	case TPM_ORD_GetTicks:
	case TPM_ORD_GetTestResult:
	case TPM_ORD_KillMaintenanceFeature:
	case TPM_ORD_Delegate_ReadTable:
	case TPM_ORD_PhysicalEnable:
	case TPM_ORD_DisableForceClear:
	case TPM_ORD_ForceClear:
	{
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT32, 1 optional AUTH */
	case TPM_ORD_OwnerReadInternalPub:
	case TPM_ORD_GetPubKey:
	case TPM_ORD_ReleaseCounterOwner:
	case TPM_ORD_ReleaseCounter:
	case TPM_ORD_IncrementCounter:
	case TPM_ORD_PcrRead:
	case TPM_ORD_DirRead:
	case TPM_ORD_ReadCounter:
	case TPM_ORD_Terminate_Handle:
	case TPM_ORD_GetAuditDigest:
	case TPM_ORD_GetRandom:
	case TPM_ORD_CMK_SetRestrictions:
	{
		UINT32 i = va_arg(ap, UINT32);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, i, out_blob);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#ifdef TSS_BUILD_CMK
	/* 1 20 byte value, 1 optional AUTH */
	case TPM_ORD_CMK_ApproveMA:
	{
		BYTE *digest1 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#endif
	/* 1 UINT16 only */
	case TSC_ORD_PhysicalPresence:
	{
		UINT16 i = va_arg(ap, int);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT16(outOffset, i, out_blob);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#ifdef TSS_BUILD_CMK
	/* 1 UINT32, 1 20 byte value, 1 BLOB, 2 20 byte values, 1 optional AUTH */
	case TPM_ORD_CMK_CreateKey:
	{
		UINT32 key1 = va_arg(ap, UINT32);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		BYTE *digest2 = va_arg(ap, BYTE *);
		BYTE *digest3 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !in_blob1 || !digest2 || !digest3) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, key1, out_blob);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest2);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest3);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 BLOB, 1 20 byte value, 1 UINT32, 1 BLOB, 1 optional AUTH */
	case TPM_ORD_CMK_CreateTicket:
	{
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !in_blob1 || !in_blob2) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT32, 1 UINT16, 1 BLOB, 1 20 byte value, 4 x (1 UINT32, 1 BLOB), 1 optional AUTH */
	case TPM_ORD_CMK_CreateBlob:
	{
		UINT32 in_key1 = va_arg(ap, UINT32);
		UINT16 i = va_arg(ap, int);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		UINT32 in_len3 = va_arg(ap, UINT32);
		BYTE *in_blob3 = va_arg(ap, BYTE *);
		UINT32 in_len4 = va_arg(ap, UINT32);
		BYTE *in_blob4 = va_arg(ap, BYTE *);
		UINT32 in_len5 = va_arg(ap, UINT32);
		BYTE *in_blob5 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!digest1 || !in_blob1 || !in_blob2 || !in_blob3 || !in_blob4 || !in_blob5) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, in_key1, out_blob);
		LoadBlob_UINT16(outOffset, i, out_blob);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		LoadBlob_UINT32(outOffset, in_len3, out_blob);
		LoadBlob(outOffset, in_len3, out_blob, in_blob3);
		LoadBlob_UINT32(outOffset, in_len4, out_blob);
		LoadBlob(outOffset, in_len4, out_blob, in_blob4);
		LoadBlob_UINT32(outOffset, in_len5, out_blob);
		LoadBlob(outOffset, in_len5, out_blob, in_blob5);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
	/* 1 UINT32, 1 60 byte value, 1 20 byte value, 1 BLOB, 2 x (1 UINT32, 1 BLOB),
	 * 1 optional AUTH */
	case TPM_ORD_CMK_ConvertMigration:
	{
		UINT32 key1 = va_arg(ap, UINT32);
		BYTE *cmkauth1 = va_arg(ap, BYTE *);
		BYTE *digest1 = va_arg(ap, BYTE *);
		UINT32 in_len1 = va_arg(ap, UINT32);
		BYTE *in_blob1 = va_arg(ap, BYTE *);
		UINT32 in_len2 = va_arg(ap, UINT32);
		BYTE *in_blob2 = va_arg(ap, BYTE *);
		UINT32 in_len3 = va_arg(ap, UINT32);
		BYTE *in_blob3 = va_arg(ap, BYTE *);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
		va_end(ap);

		if (!cmkauth1 || !digest1 || !in_blob1 || !in_blob2 || !in_blob3) {
			result = TCSERR(TSS_E_INTERNAL_ERROR);
			LogError("Internal error for ordinal 0x%x", ordinal);
			break;
		}

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, key1, out_blob);
		LoadBlob(outOffset, 3 * TPM_SHA1_160_HASH_LEN, out_blob, cmkauth1);
		LoadBlob(outOffset, TPM_SHA1_160_HASH_LEN, out_blob, digest1);
		LoadBlob(outOffset, in_len1, out_blob, in_blob1);
		LoadBlob_UINT32(outOffset, in_len2, out_blob);
		LoadBlob(outOffset, in_len2, out_blob, in_blob2);
		LoadBlob_UINT32(outOffset, in_len3, out_blob);
		LoadBlob(outOffset, in_len3, out_blob, in_blob3);
		if (auth1) {
			LoadBlob_Auth(outOffset, out_blob, auth1);
			LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);
		} else {
			LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);
		}

		break;
	}
#endif
#ifdef TSS_BUILD_TSS12
	case TPM_ORD_FlushSpecific:
	{
		UINT32 val1 = va_arg(ap, UINT32);
		UINT32 val2 = va_arg(ap, UINT32);
		va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, val1, out_blob);
		LoadBlob_UINT32(outOffset, val2, out_blob);
		LoadBlob_Header(TPM_TAG_RQU_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
	/* 1 UINT32, 1 BLOB, 1 UINT32, 1 BOOL, 1 AUTH */
	case TPM_ORD_KeyControlOwner:
	{
		UINT32 i = va_arg(ap, UINT32);
		UINT32 len1 = va_arg(ap, UINT32);
		BYTE *blob1 = va_arg(ap, BYTE *);
		UINT32 j = va_arg(ap, UINT32);
		TSS_BOOL bool1 = va_arg(ap, int);
		TPM_AUTH *auth1 = va_arg(ap, TPM_AUTH *);
	        va_end(ap);

		*outOffset += TSS_TPM_TXBLOB_HDR_LEN;
		LoadBlob_UINT32(outOffset, i, out_blob);
		LoadBlob(outOffset, len1, out_blob, blob1);
		LoadBlob_UINT32(outOffset, j, out_blob);
		LoadBlob_BOOL(outOffset, bool1, out_blob);
		LoadBlob_Auth(outOffset, out_blob, auth1);
		LoadBlob_Header(TPM_TAG_RQU_AUTH1_COMMAND, *outOffset, ordinal, out_blob);

		break;
	}
#endif
	default:
		LogError("Unknown ordinal: 0x%x", ordinal);
		break;
	}

	return result;
}
