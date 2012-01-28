
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
#include <syslog.h>
#include <string.h>
#include <netdb.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_utils.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcslog.h"
#include "tcsd_wrap.h"
#include "tcsd.h"
#include "tcs_utils.h"
#include "rpc_tcstp_tcs.h"


TSS_RESULT
tcs_wrap_EstablishTransport(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hEncKey, hTransSession;
	UINT32 ulTransControlFlags, ulTransSessionInfoSize, ulSecretSize, ulCurrentTicks, i;
	BYTE *rgbTransSessionInfo, *rgbSecret, *prgbCurrentTicks;
	TPM_MODIFIER_INDICATOR pbLocality;
	TPM_AUTH pEncKeyAuth, *pAuth;
	TPM_NONCE pTransNonce;
	TSS_RESULT result;

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &ulTransControlFlags, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 2, &hEncKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, 3, &ulTransSessionInfoSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbTransSessionInfo = malloc(ulTransSessionInfoSize);
	if (rgbTransSessionInfo == NULL) {
		LogError("malloc of %u bytes failed.", ulTransSessionInfoSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 4, rgbTransSessionInfo, ulTransSessionInfoSize,
		    &data->comm)) {
		free(rgbTransSessionInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (getData(TCSD_PACKET_TYPE_UINT32, 5, &ulSecretSize, 0, &data->comm)) {
		free(rgbTransSessionInfo);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	rgbSecret = malloc(ulSecretSize);
	if (rgbSecret == NULL) {
		free(rgbTransSessionInfo);
		LogError("malloc of %u bytes failed.", ulSecretSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, 6, rgbSecret, ulSecretSize, &data->comm)) {
		free(rgbTransSessionInfo);
		free(rgbSecret);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, 7, &pEncKeyAuth, 0, &data->comm))
		pAuth = NULL;
	else
		pAuth = &pEncKeyAuth;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_EstablishTransport_Internal(hContext, ulTransControlFlags, hEncKey,
						  ulTransSessionInfoSize, rgbTransSessionInfo,
						  ulSecretSize, rgbSecret, pAuth, &pbLocality,
						  &hTransSession, &ulCurrentTicks,
						  &prgbCurrentTicks, &pTransNonce);

	MUTEX_UNLOCK(tcsp_lock);

	free(rgbSecret);
	free(rgbTransSessionInfo);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 6);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth, 0, &data->comm)) {
				free(prgbCurrentTicks);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pbLocality, 0, &data->comm)) {
			free(prgbCurrentTicks);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &hTransSession, 0, &data->comm)) {
			free(prgbCurrentTicks);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &ulCurrentTicks, 0, &data->comm)) {
			free(prgbCurrentTicks);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, i++, prgbCurrentTicks, ulCurrentTicks,
			    &data->comm)) {
			free(prgbCurrentTicks);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(prgbCurrentTicks);
		if (setData(TCSD_PACKET_TYPE_NONCE, i++, &pTransNonce, 0, &data->comm))
			return TCSERR(TSS_E_INTERNAL_ERROR);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ExecuteTransport(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TPM_COMMAND_CODE unWrappedCommandOrdinal;
	TCS_HANDLE *rghHandles = NULL, handles[2];
	UINT32 ulWrappedCmdDataInSize, pulHandleListSize, ulWrappedCmdDataOutSize, i = 0;
	BYTE *rgbWrappedCmdDataIn, *rgbWrappedCmdDataOut;
	TPM_MODIFIER_INDICATOR pbLocality;
	TPM_AUTH pWrappedCmdAuth1, pWrappedCmdAuth2, pTransAuth, *pAuth1, *pAuth2, null_auth;
	UINT64 punCurrentTicks;
	TSS_RESULT result, pulWrappedCmdReturnCode;

	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &unWrappedCommandOrdinal, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &ulWrappedCmdDataInSize, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	rgbWrappedCmdDataIn = malloc(ulWrappedCmdDataInSize);
	if (rgbWrappedCmdDataIn == NULL) {
		LogError("malloc of %u bytes failed", ulWrappedCmdDataInSize);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_PBYTE, i++, rgbWrappedCmdDataIn, ulWrappedCmdDataInSize,
		    &data->comm)) {
		free(rgbWrappedCmdDataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_UINT32, i++, &pulHandleListSize, 0, &data->comm)) {
		free(rgbWrappedCmdDataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (pulHandleListSize > 2) {
		free(rgbWrappedCmdDataIn);
		return TCSERR(TSS_E_BAD_PARAMETER);
	}

	if (pulHandleListSize) {
		if (getData(TCSD_PACKET_TYPE_PBYTE, i++, handles,
			    pulHandleListSize * sizeof(UINT32), &data->comm)) {
			free(rgbWrappedCmdDataIn);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
	}
	rghHandles = handles;

	memset(&null_auth, 0, sizeof(TPM_AUTH));
	memset(&pWrappedCmdAuth1, 0, sizeof(TPM_AUTH));
	memset(&pWrappedCmdAuth2, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_AUTH, i++, &pWrappedCmdAuth1, 0, &data->comm)) {
		free(rgbWrappedCmdDataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, i++, &pWrappedCmdAuth2, 0, &data->comm)) {
		free(rgbWrappedCmdDataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}
	if (getData(TCSD_PACKET_TYPE_AUTH, i++, &pTransAuth, 0, &data->comm)) {
		free(rgbWrappedCmdDataIn);
		return TCSERR(TSS_E_INTERNAL_ERROR);
	}

	if (!memcmp(&pWrappedCmdAuth1, &null_auth, sizeof(TPM_AUTH)))
		pAuth1 = NULL;
	else
		pAuth1 = &pWrappedCmdAuth1;

	if (!memcmp(&pWrappedCmdAuth2, &null_auth, sizeof(TPM_AUTH)))
		pAuth2 = NULL;
	else
		pAuth2 = &pWrappedCmdAuth2;

	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ExecuteTransport_Internal(hContext, unWrappedCommandOrdinal,
						ulWrappedCmdDataInSize, rgbWrappedCmdDataIn,
						&pulHandleListSize, &rghHandles, pAuth1, pAuth2,
						&pTransAuth, &punCurrentTicks, &pbLocality,
						&pulWrappedCmdReturnCode, &ulWrappedCmdDataOutSize,
						&rgbWrappedCmdDataOut);

	MUTEX_UNLOCK(tcsp_lock);

	free(rgbWrappedCmdDataIn);

	if (result == TSS_SUCCESS) {
		i = 0;
		initData(&data->comm, 10);
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pulHandleListSize, 0, &data->comm)) {
			free(rgbWrappedCmdDataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (pulHandleListSize) {
			if (setData(TCSD_PACKET_TYPE_PBYTE, i++, rghHandles,
				    pulHandleListSize * sizeof(UINT32), &data->comm)) {
				free(rgbWrappedCmdDataOut);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (pAuth1) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth1, 0, &data->comm)) {
				free(rgbWrappedCmdDataOut);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		} else {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, &null_auth, 0, &data->comm)) {
				free(rgbWrappedCmdDataOut);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (pAuth2) {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, pAuth2, 0, &data->comm)) {
				free(rgbWrappedCmdDataOut);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		} else {
			if (setData(TCSD_PACKET_TYPE_AUTH, i++, &null_auth, 0, &data->comm)) {
				free(rgbWrappedCmdDataOut);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_AUTH, i++, &pTransAuth, 0, &data->comm)) {
			free(rgbWrappedCmdDataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT64, i++, &punCurrentTicks, 0, &data->comm)) {
			free(rgbWrappedCmdDataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pbLocality, 0, &data->comm)) {
			free(rgbWrappedCmdDataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &pulWrappedCmdReturnCode, 0,
			    &data->comm)) {
			free(rgbWrappedCmdDataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, i++, &ulWrappedCmdDataOutSize, 0,
			    &data->comm)) {
			free(rgbWrappedCmdDataOut);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (ulWrappedCmdDataOutSize) {
			if (setData(TCSD_PACKET_TYPE_PBYTE, i++, rgbWrappedCmdDataOut,
				    ulWrappedCmdDataOutSize, &data->comm)) {
				free(rgbWrappedCmdDataOut);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		free(rgbWrappedCmdDataOut);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}

TSS_RESULT
tcs_wrap_ReleaseTransportSigned(struct tcsd_thread_data *data)
{
	TCS_CONTEXT_HANDLE hContext;
	TCS_KEY_HANDLE hSignatureKey;
	TPM_NONCE AntiReplayNonce;
	UINT32 pulCurrentTicks, pulSignatureSize;
	BYTE *prgbCurrentTicks, *prgbSignature;
	TPM_MODIFIER_INDICATOR pbLocality;
	TPM_AUTH pKeyAuth, pTransAuth, *pAuth, null_auth;
	TSS_RESULT result;

	memset(&null_auth, 0, sizeof(TPM_AUTH));
	memset(&pKeyAuth, 0, sizeof(TPM_AUTH));

	if (getData(TCSD_PACKET_TYPE_UINT32, 0, &hContext, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	LogDebugFn("thread %ld context %x", THREAD_ID, hContext);

	if (getData(TCSD_PACKET_TYPE_UINT32, 1, &hSignatureKey, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_NONCE, 2, &AntiReplayNonce, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);
	if (getData(TCSD_PACKET_TYPE_AUTH, 3, &pKeyAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);

	if (!memcmp(&null_auth, &pKeyAuth, sizeof(TPM_AUTH)))
		pAuth = NULL;
	else
		pAuth = &pKeyAuth;

	if (getData(TCSD_PACKET_TYPE_AUTH, 4, &pTransAuth, 0, &data->comm))
		return TCSERR(TSS_E_INTERNAL_ERROR);


	MUTEX_LOCK(tcsp_lock);

	result = TCSP_ReleaseTransportSigned_Internal(hContext, hSignatureKey, &AntiReplayNonce,
						      pAuth, &pTransAuth, &pbLocality,
						      &pulCurrentTicks, &prgbCurrentTicks,
						      &pulSignatureSize, &prgbSignature);

	MUTEX_UNLOCK(tcsp_lock);

	if (result == TSS_SUCCESS) {
		initData(&data->comm, 7);
		if (pAuth) {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, pAuth, 0, &data->comm)) {
				free(prgbCurrentTicks);
				free(prgbSignature);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		} else {
			if (setData(TCSD_PACKET_TYPE_AUTH, 0, &null_auth, 0, &data->comm)) {
				free(prgbCurrentTicks);
				free(prgbSignature);
				return TCSERR(TSS_E_INTERNAL_ERROR);
			}
		}
		if (setData(TCSD_PACKET_TYPE_AUTH, 1, &pTransAuth, 0, &data->comm)) {
			free(prgbCurrentTicks);
			free(prgbSignature);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 2, &pbLocality, 0, &data->comm)) {
			free(prgbCurrentTicks);
			free(prgbSignature);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_UINT32, 3, &pulCurrentTicks, 0, &data->comm)) {
			free(prgbCurrentTicks);
			free(prgbSignature);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 4, prgbCurrentTicks, pulCurrentTicks,
			    &data->comm)) {
			free(prgbCurrentTicks);
			free(prgbSignature);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(prgbCurrentTicks);
		if (setData(TCSD_PACKET_TYPE_UINT32, 5, &pulSignatureSize, 0, &data->comm)) {
			free(prgbSignature);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		if (setData(TCSD_PACKET_TYPE_PBYTE, 6, prgbSignature, pulSignatureSize,
			    &data->comm)) {
			free(prgbSignature);
			return TCSERR(TSS_E_INTERNAL_ERROR);
		}
		free(prgbSignature);
	} else
		initData(&data->comm, 0);

	data->comm.hdr.u.result = result;
	return TSS_SUCCESS;
}
