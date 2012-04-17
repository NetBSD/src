
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
Tspi_TPM_GetCapability(TSS_HTPM hTPM,			/* in */
		       TSS_FLAG capArea,		/* in */
		       UINT32 ulSubCapLength,		/* in */
		       BYTE * rgbSubCap,		/* in */
		       UINT32 * pulRespDataLength,	/* out */
		       BYTE ** prgbRespData)		/* out */
{
	TSS_HCONTEXT tspContext;
	TPM_CAPABILITY_AREA tcsCapArea;
	UINT32 tcsSubCap = 0;
	UINT32 tcsSubCapContainer;
	TSS_RESULT result;
	UINT32 nonVolFlags, volFlags, respLen;
	BYTE *respData;
	UINT64 offset;
	TSS_BOOL fOwnerAuth = FALSE, endianFlag = TRUE;

	if (pulRespDataLength == NULL || prgbRespData == NULL)
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((result = obj_tpm_get_tsp_context(hTPM, &tspContext)))
		return result;

	/* Verify the caps and subcaps */
	switch (capArea) {
	case TSS_TPMCAP_ORD:
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		tcsCapArea = TCPA_CAP_ORD;
		tcsSubCap = *(UINT32 *)rgbSubCap;
		break;
	case TSS_TPMCAP_FLAG:
		fOwnerAuth = TRUE;
		break;
	case TSS_TPMCAP_AUTH_ENCRYPT:
	case TSS_TPMCAP_ALG:
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		/* Test capArea again here in order to keep from having to duplicate the switch
		 * statement below */
		tcsCapArea = (capArea == TSS_TPMCAP_ALG ? TPM_CAP_ALG : TPM_CAP_AUTH_ENCRYPT);

		switch (*(UINT32 *)rgbSubCap) {
		case TSS_ALG_RSA:
			tcsSubCap = TPM_ALG_RSA;
			break;
		case TSS_ALG_AES128:
			tcsSubCap = TPM_ALG_AES128;
			break;
		case TSS_ALG_AES192:
			tcsSubCap = TPM_ALG_AES192;
			break;
		case TSS_ALG_AES256:
			tcsSubCap = TPM_ALG_AES256;
			break;
		case TSS_ALG_3DES:
			tcsSubCap = TPM_ALG_3DES;
			break;
		case TSS_ALG_DES:
			tcsSubCap = TPM_ALG_DES;
			break;
		case TSS_ALG_SHA:
			tcsSubCap = TPM_ALG_SHA;
			break;
		case TSS_ALG_HMAC:
			tcsSubCap = TPM_ALG_HMAC;
			break;
		case TSS_ALG_MGF1:
			tcsSubCap = TPM_ALG_MGF1;
			break;
		case TSS_ALG_XOR:
			tcsSubCap = TPM_ALG_XOR;
			break;
		default:
			tcsSubCap = *(UINT32 *)rgbSubCap;
			break;
		}
		break;
#ifdef TSS_BUILD_NV
	case TSS_TPMCAP_NV_LIST:
		tcsCapArea = TPM_CAP_NV_LIST;
		endianFlag = FALSE;
		break;
	case TSS_TPMCAP_NV_INDEX:
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		tcsCapArea = TPM_CAP_NV_INDEX;
		tcsSubCap = *(UINT32 *)rgbSubCap;
		break;
#endif
	case TSS_TPMCAP_PROPERTY:	/* Determines a physical property of the TPM. */
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		tcsCapArea = TCPA_CAP_PROPERTY;
		tcsSubCapContainer = *(UINT32 *)rgbSubCap;

		switch (tcsSubCapContainer) {
		case TSS_TPMCAP_PROP_PCR:
			tcsSubCap = TPM_CAP_PROP_PCR;
			break;
		case TSS_TPMCAP_PROP_DIR:
			tcsSubCap = TPM_CAP_PROP_DIR;
			break;
		/* case TSS_TPMCAP_PROP_SLOTS: */
		case TSS_TPMCAP_PROP_KEYS:
			tcsSubCap = TPM_CAP_PROP_SLOTS;
			break;
		case TSS_TPMCAP_PROP_MANUFACTURER:
			tcsSubCap = TPM_CAP_PROP_MANUFACTURER;
			endianFlag = FALSE;
			break;
		case TSS_TPMCAP_PROP_COUNTERS:
			tcsSubCap = TPM_CAP_PROP_COUNTERS;
			break;
		case TSS_TPMCAP_PROP_MAXCOUNTERS:
			tcsSubCap = TPM_CAP_PROP_MAX_COUNTERS;
			break;
		/*case TSS_TPMCAP_PROP_MINCOUNTERINCTIME: */
		case TSS_TPMCAP_PROP_MIN_COUNTER:
			tcsSubCap = TPM_CAP_PROP_MIN_COUNTER;
			break;
		case TSS_TPMCAP_PROP_ACTIVECOUNTER:
			tcsSubCap = TPM_CAP_PROP_ACTIVE_COUNTER;
			break;
		case TSS_TPMCAP_PROP_TRANSESSIONS:
			tcsSubCap = TPM_CAP_PROP_TRANSSESS;
			break;
		case TSS_TPMCAP_PROP_MAXTRANSESSIONS:
			tcsSubCap = TPM_CAP_PROP_MAX_TRANSSESS;
			break;
		case TSS_TPMCAP_PROP_SESSIONS:
			tcsSubCap = TPM_CAP_PROP_SESSIONS;
			break;
		case TSS_TPMCAP_PROP_MAXSESSIONS:
			tcsSubCap = TPM_CAP_PROP_MAX_SESSIONS;
			break;
		case TSS_TPMCAP_PROP_FAMILYROWS:
			tcsSubCap = TPM_CAP_PROP_FAMILYROWS;
			break;
		case TSS_TPMCAP_PROP_DELEGATEROWS:
			tcsSubCap = TPM_CAP_PROP_DELEGATE_ROW;
			break;
		case TSS_TPMCAP_PROP_OWNER:
			tcsSubCap = TPM_CAP_PROP_OWNER;
			break;
		case TSS_TPMCAP_PROP_MAXKEYS:
			tcsSubCap = TPM_CAP_PROP_MAX_KEYS;
			break;
		case TSS_TPMCAP_PROP_AUTHSESSIONS:
			tcsSubCap = TPM_CAP_PROP_AUTHSESS;
			break;
		case TSS_TPMCAP_PROP_MAXAUTHSESSIONS:
			tcsSubCap = TPM_CAP_PROP_MAX_AUTHSESS;
			break;
		case TSS_TPMCAP_PROP_CONTEXTS:
			tcsSubCap = TPM_CAP_PROP_CONTEXT;
			break;
		case TSS_TPMCAP_PROP_MAXCONTEXTS:
			tcsSubCap = TPM_CAP_PROP_MAX_CONTEXT;
			break;
		case TSS_TPMCAP_PROP_DAASESSIONS:
			tcsSubCap = TPM_CAP_PROP_SESSION_DAA;
			break;
		case TSS_TPMCAP_PROP_MAXDAASESSIONS:
			tcsSubCap = TPM_CAP_PROP_DAA_MAX;
			break;
		case TSS_TPMCAP_PROP_TISTIMEOUTS:
			tcsSubCap = TPM_CAP_PROP_TIS_TIMEOUT;
			break;
		case TSS_TPMCAP_PROP_STARTUPEFFECTS:
			tcsSubCap = TPM_CAP_PROP_STARTUP_EFFECT;
			endianFlag = FALSE;
			break;
		case TSS_TPMCAP_PROP_MAXCONTEXTCOUNTDIST:
			tcsSubCap = TPM_CAP_PROP_CONTEXT_DIST;
			break;
		case TSS_TPMCAP_PROP_CMKRESTRICTION:
			tcsSubCap = TPM_CAP_PROP_CMK_RESTRICTION;
			break;
		case TSS_TPMCAP_PROP_DURATION:
			tcsSubCap = TPM_CAP_PROP_DURATION;
			break;
		case TSS_TPMCAP_PROP_MAXNVAVAILABLE:
			tcsSubCap = TPM_CAP_PROP_NV_AVAILABLE;
			break;
		case TSS_TPMCAP_PROP_INPUTBUFFERSIZE:
			tcsSubCap = TPM_CAP_PROP_INPUT_BUFFER;
			break;
#if 0
		/* There isn't a way to query the TPM for these, the TPMWG is considering how to
		 * address some of them in the next version of the TPM - KEY Oct 15, 2007*/
		case TSS_TPMCAP_PROP_MAXNVWRITE:
			break;
		case TSS_TPMCAP_PROP_REVISION:
			break;
		case TSS_TPMCAP_PROP_LOCALITIES_AVAIL:
			break;
		case TSS_TPMCAP_PROP_PCRMAP:
			break;
#endif
		default:
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
		break;
	case TSS_TPMCAP_VERSION:	/* Queries the current TPM version. */
		tcsCapArea = TCPA_CAP_VERSION;
		endianFlag = FALSE;
		break;
	case TSS_TPMCAP_VERSION_VAL:	/* Queries the current TPM version for 1.2 TPM device. */
		tcsCapArea = TPM_CAP_VERSION_VAL;
		endianFlag = FALSE;
		break;
	case TSS_TPMCAP_MFR:
		tcsCapArea = TPM_CAP_MFR;
		endianFlag = FALSE;
		break;
	case TSS_TPMCAP_SYM_MODE:
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		tcsCapArea = TPM_CAP_SYM_MODE;
		tcsSubCap = *(UINT32 *)rgbSubCap;
		break;
	case TSS_TPMCAP_HANDLE:
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		tcsCapArea = TPM_CAP_HANDLE;
		tcsSubCap = *(UINT32 *)rgbSubCap;
		break;
	case TSS_TPMCAP_TRANS_ES:
		if ((ulSubCapLength != sizeof(UINT32)) || !rgbSubCap)
			return TSPERR(TSS_E_BAD_PARAMETER);

		tcsCapArea = TPM_CAP_TRANS_ES;
		switch (*(UINT32 *)rgbSubCap) {
		case TSS_ES_NONE:
			tcsSubCap = TPM_ES_NONE;
			break;
		case TSS_ES_RSAESPKCSV15:
			tcsSubCap = TPM_ES_RSAESPKCSv15;
			break;
		case TSS_ES_RSAESOAEP_SHA1_MGF1:
			tcsSubCap = TPM_ES_RSAESOAEP_SHA1_MGF1;
			break;
		case TSS_ES_SYM_CNT:
			tcsSubCap = TPM_ES_SYM_CNT;
			break;
		case TSS_ES_SYM_OFB:
			tcsSubCap = TPM_ES_SYM_OFB;
			break;
		case TSS_ES_SYM_CBC_PKCS5PAD:
			tcsSubCap = TPM_ES_SYM_CBC_PKCS5PAD;
			break;
		default:
			tcsSubCap = *(UINT32 *)rgbSubCap;
			break;
		}
		break;
	default:
		return TSPERR(TSS_E_BAD_PARAMETER);
		break;
	}

	if (fOwnerAuth) {
		/* do an owner authorized get capability call */
		if ((result = get_tpm_flags(tspContext, hTPM, &volFlags, &nonVolFlags)))
			return result;

		respLen = 2 * sizeof(UINT32);
		respData = calloc_tspi(tspContext, respLen);
		if (respData == NULL) {
			LogError("malloc of %u bytes failed.", respLen);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		offset = 0;
		Trspi_LoadBlob_UINT32(&offset, nonVolFlags, respData);
		Trspi_LoadBlob_UINT32(&offset, volFlags, respData);

		*pulRespDataLength = respLen;
		*prgbRespData = respData;

		return TSS_SUCCESS;
	}

	tcsSubCap = endian32(tcsSubCap);

	if ((result = TCS_API(tspContext)->GetTPMCapability(tspContext, tcsCapArea, ulSubCapLength,
							    (BYTE *)&tcsSubCap, pulRespDataLength,
							    prgbRespData)))
		return result;

	if (endianFlag) {
		if (*pulRespDataLength == sizeof(UINT32))
			*(UINT32 *)(*prgbRespData) = endian32(*(UINT32 *)(*prgbRespData));
		else if (*pulRespDataLength == sizeof(UINT16))
			*(UINT32 *)(*prgbRespData) = endian16(*(UINT32 *)(*prgbRespData));
	}

	if ((result = __tspi_add_mem_entry(tspContext, *prgbRespData))) {
		free(*prgbRespData);
		*prgbRespData = NULL;
		*pulRespDataLength = 0;
		return result;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_TPM_GetCapabilitySigned(TSS_HTPM hTPM,			/* in */
			     TSS_HTPM hKey,			/* in */
			     TSS_FLAG capArea,			/* in */
			     UINT32 ulSubCapLength,		/* in */
			     BYTE * rgbSubCap,			/* in */
			     TSS_VALIDATION * pValidationData,	/* in, out */
			     UINT32 * pulRespDataLength,	/* out */
			     BYTE ** prgbRespData)		/* out */
{
	/*
	 * Function was found to have a vulnerability, so implementation is not
	 * required by the TSS 1.1b spec.
	 */
	return TSPERR(TSS_E_NOTIMPL);
}

