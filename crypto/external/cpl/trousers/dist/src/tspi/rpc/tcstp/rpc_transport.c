
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
#include <assert.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "hosttable.h"
#include "tcsd_wrap.h"
#include "obj.h"
#include "rpc_tcstp_tsp.h"


TSS_RESULT
RPC_EstablishTransport_TP(struct host_table_entry *hte,
			   UINT32                  ulTransControlFlags,
			   TCS_KEY_HANDLE          hEncKey,
			   UINT32                  ulTransSessionInfoSize,
			   BYTE*                   rgbTransSessionInfo,
			   UINT32                  ulSecretSize,
			   BYTE*                   rgbSecret,
			   TPM_AUTH*               pEncKeyAuth,		/* in, out */
			   TPM_MODIFIER_INDICATOR* pbLocality,
			   TCS_HANDLE*             hTransSession,
			   UINT32*                 ulCurrentTicksSize,
			   BYTE**                  prgbCurrentTicks,
			   TPM_NONCE*              pTransNonce)
{
	TSS_RESULT result;
	UINT32 i;

	initData(&hte->comm, 8);
	hte->comm.hdr.u.ordinal = TCSD_ORD_ESTABLISHTRANSPORT;
	LogDebugFn("IN: TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &ulTransControlFlags, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 2, &hEncKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 3, &ulTransSessionInfoSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 4, rgbTransSessionInfo, ulTransSessionInfoSize,
		    &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 5, &ulSecretSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, 6, rgbSecret, ulSecretSize, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pEncKeyAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 7, pEncKeyAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (pEncKeyAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pEncKeyAuth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto done;
			}
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pbLocality, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, hTransSession, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, ulCurrentTicksSize, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}

		*prgbCurrentTicks = malloc(*ulCurrentTicksSize);
		if (*prgbCurrentTicks == NULL) {
			*ulCurrentTicksSize = 0;
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto done;
		}

		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *prgbCurrentTicks, *ulCurrentTicksSize,
			    &hte->comm)) {
			free(*prgbCurrentTicks);
			*prgbCurrentTicks = NULL;
			*ulCurrentTicksSize = 0;
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
		if (getData(TCSD_PACKET_TYPE_NONCE, i++, pTransNonce, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto done;
		}
	}
done:
	return result;
}


TSS_RESULT
RPC_ExecuteTransport_TP(struct host_table_entry *hte,
			 TPM_COMMAND_CODE        unWrappedCommandOrdinal,
			 UINT32                  ulWrappedCmdParamInSize,
			 BYTE*                   rgbWrappedCmdParamIn,
			 UINT32*                 pulHandleListSize,	/* in, out */
			 TCS_HANDLE**            rghHandles,		/* in, out */
			 TPM_AUTH*               pWrappedCmdAuth1,	/* in, out */
			 TPM_AUTH*               pWrappedCmdAuth2,	/* in, out */
			 TPM_AUTH*               pTransAuth,		/* in, out */
			 UINT64*                 punCurrentTicks,
			 TPM_MODIFIER_INDICATOR* pbLocality,
			 TPM_RESULT*             pulWrappedCmdReturnCode,
			 UINT32*                 ulWrappedCmdParamOutSize,
			 BYTE**                  rgbWrappedCmdParamOut)
{
	TSS_RESULT result;
	TPM_AUTH null_auth;
	UINT32 i = 0;

	memset(&null_auth, 0, sizeof(TPM_AUTH));

	initData(&hte->comm, 9);
	hte->comm.hdr.u.ordinal = TCSD_ORD_EXECUTETRANSPORT;
	LogDebugFn("IN: TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &unWrappedCommandOrdinal, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, &ulWrappedCmdParamInSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_PBYTE, i++, rgbWrappedCmdParamIn, ulWrappedCmdParamInSize,
		    &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, i++, pulHandleListSize, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (*pulHandleListSize) {
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, *rghHandles,
			    *pulHandleListSize * sizeof(UINT32), &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (pWrappedCmdAuth1) {
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, pWrappedCmdAuth1, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, &null_auth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (pWrappedCmdAuth2) {
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, pWrappedCmdAuth2, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, &null_auth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (setData(TCSD_PACKET_TYPE_AUTH, i++, pTransAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		i = 0;
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pulHandleListSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);

		if (*pulHandleListSize) {
			*rghHandles = malloc(*pulHandleListSize * sizeof(UINT32));
			if (*rghHandles == NULL) {
				*pulHandleListSize = 0;
				return TSPERR(TSS_E_INTERNAL_ERROR);
			}
			if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *rghHandles,
				    *pulHandleListSize * sizeof(UINT32), &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
		}
		if (pWrappedCmdAuth1) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pWrappedCmdAuth1, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
		} else {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, &null_auth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
		}
		if (pWrappedCmdAuth2) {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, pWrappedCmdAuth2, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
		} else {
			if (getData(TCSD_PACKET_TYPE_AUTH, i++, &null_auth, 0, &hte->comm)) {
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
		}
		if (getData(TCSD_PACKET_TYPE_AUTH, i++, pTransAuth, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto error;
		}
		if (getData(TCSD_PACKET_TYPE_UINT64, i++, punCurrentTicks, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto error;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pbLocality, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto error;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, pulWrappedCmdReturnCode, 0, &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto error;
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, i++, ulWrappedCmdParamOutSize, 0,
			    &hte->comm)) {
			result = TSPERR(TSS_E_INTERNAL_ERROR);
			goto error;
		}
		if (*ulWrappedCmdParamOutSize) {
			*rgbWrappedCmdParamOut = malloc(*ulWrappedCmdParamOutSize);
			if (*rgbWrappedCmdParamOut == NULL) {
				*ulWrappedCmdParamOutSize = 0;
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
			if (getData(TCSD_PACKET_TYPE_PBYTE, i++, *rgbWrappedCmdParamOut,
				    *ulWrappedCmdParamOutSize, &hte->comm)) {
				free(*rgbWrappedCmdParamOut);
				result = TSPERR(TSS_E_INTERNAL_ERROR);
				goto error;
			}
		} else
			*rgbWrappedCmdParamOut = NULL;
	}

	return result;
error:
	if (*pulHandleListSize) {
		free(*rghHandles);
		*rghHandles = NULL;
	}
	return result;
}

TSS_RESULT
RPC_ReleaseTransportSigned_TP(struct host_table_entry *hte,
			       TCS_KEY_HANDLE          hSignatureKey,
			       TPM_NONCE*              AntiReplayNonce,
			       TPM_AUTH*               pKeyAuth,		/* in, out */
			       TPM_AUTH*               pTransAuth,		/* in, out */
			       TPM_MODIFIER_INDICATOR* pbLocality,
			       UINT32*                 pulCurrentTicksSize,
			       BYTE**                  prgbCurrentTicks,
			       UINT32*                 pulSignatureSize,
			       BYTE**                  prgbSignature)
{
	TSS_RESULT result;
	TPM_AUTH null_auth;

	memset(&null_auth, 0, sizeof(TPM_AUTH));

	initData(&hte->comm, 5);
	hte->comm.hdr.u.ordinal = TCSD_ORD_RELEASETRANSPORTSIGNED;
	LogDebugFn("IN: TCS Context: 0x%x", hte->tcsContext);

	if (setData(TCSD_PACKET_TYPE_UINT32, 0, &hte->tcsContext, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_UINT32, 1, &hSignatureKey, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (setData(TCSD_PACKET_TYPE_NONCE, 2, AntiReplayNonce, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);
	if (pKeyAuth) {
		if (setData(TCSD_PACKET_TYPE_AUTH, 3, pKeyAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	} else {
		if (setData(TCSD_PACKET_TYPE_AUTH, 3, &null_auth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (setData(TCSD_PACKET_TYPE_AUTH, 4, pTransAuth, 0, &hte->comm))
		return TSPERR(TSS_E_INTERNAL_ERROR);

	result = sendTCSDPacket(hte);

	if (result == TSS_SUCCESS)
		result = hte->comm.hdr.u.result;

	if (result == TSS_SUCCESS) {
		if (pKeyAuth) {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, pKeyAuth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		} else {
			if (getData(TCSD_PACKET_TYPE_AUTH, 0, &null_auth, 0, &hte->comm))
				return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_AUTH, 1, pTransAuth, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 2, pbLocality, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);
		if (getData(TCSD_PACKET_TYPE_UINT32, 3, pulCurrentTicksSize, 0, &hte->comm))
			return TSPERR(TSS_E_INTERNAL_ERROR);

		*prgbCurrentTicks = malloc(*pulCurrentTicksSize);
		if (*prgbCurrentTicks == NULL) {
			*pulCurrentTicksSize = 0;
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 4, *prgbCurrentTicks, *pulCurrentTicksSize,
			    &hte->comm)) {
			free(*prgbCurrentTicks);
			*prgbCurrentTicks = NULL;
			*pulCurrentTicksSize = 0;
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_UINT32, 5, pulSignatureSize, 0, &hte->comm)) {
			free(*prgbCurrentTicks);
			*prgbCurrentTicks = NULL;
			*pulCurrentTicksSize = 0;
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}

		*prgbSignature = malloc(*pulSignatureSize);
		if (*prgbSignature == NULL) {
			free(*prgbCurrentTicks);
			*prgbCurrentTicks = NULL;
			*pulCurrentTicksSize = 0;
			*pulSignatureSize = 0;
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
		if (getData(TCSD_PACKET_TYPE_PBYTE, 6, *prgbSignature, *pulSignatureSize,
			    &hte->comm)) {
			free(*prgbCurrentTicks);
			*prgbCurrentTicks = NULL;
			*pulCurrentTicksSize = 0;
			free(*prgbSignature);
			*prgbSignature = NULL;
			*pulSignatureSize = 0;
			return TSPERR(TSS_E_INTERNAL_ERROR);
		}
	}

	return result;
}
