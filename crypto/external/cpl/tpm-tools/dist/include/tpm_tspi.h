/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

#ifndef __TPM_TSPI_H
#define __TPM_TSPI_H

#include <stdlib.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>
#include <tpm_utils.h>

extern TSS_UUID SRK_UUID;

#define NULL_HOBJECT 0
#define NULL_HKEY NULL_HOBJECT
#define NULL_HPCRS NULL_HOBJECT

//Display functions
const char *displayKeyUsageMap(UINT32 a_uiData);

const char *displayKeyFlagsMap(UINT32 a_uiFlags);

const char *displayAuthUsageMap(UINT32 a_uiData);

const char *displayAlgorithmMap(UINT32 a_uiData);

const char *displayEncSchemeMap(UINT32 a_uiData);

const char *displaySigSchemeMap(UINT32 a_uiData);

TSS_RESULT displayKey(TSS_HKEY a_hKey);

//Generic query functions
BOOL isTpmOwned(TSS_HCONTEXT hContext);

//TSPI logging functions
void tspiDebug(const char *a_szName, TSS_RESULT a_iResult);
void tspiError(const char *a_szName, TSS_RESULT a_iResult);
void tspiResult(const char *a_szName, TSS_RESULT a_tResult);

// Map a TSS_BOOL into a BOOL
BOOL mapTssBool(TSS_BOOL a_bValue);

//TSPI generic setup/teardown functions
TSS_RESULT contextCreate(TSS_HCONTEXT * a_hContext);
TSS_RESULT contextClose(TSS_HCONTEXT a_hContext);
TSS_RESULT contextConnect(TSS_HCONTEXT a_hContext);
TSS_RESULT contextCreateObject(TSS_HCONTEXT a_hContext,
			       TSS_FLAG a_fType,
			       TSS_FLAG a_fAttrs, TSS_HOBJECT * a_hObject);
TSS_RESULT contextCloseObject(TSS_HCONTEXT a_hContext,
			      TSS_HOBJECT a_hObject);
TSS_RESULT contextGetTpm(TSS_HCONTEXT a_hContext, TSS_HTPM * a_hTpm);
TSS_RESULT policyGet(TSS_HOBJECT a_hObject, TSS_HPOLICY * a_hPolicy);
TSS_RESULT policyAssign(TSS_HPOLICY a_hPolicy, TSS_HOBJECT a_hObject);
TSS_RESULT policySetSecret(TSS_HPOLICY a_hPolicy,
			   UINT32 a_uiSecretLen, BYTE * a_chSecret);

TSS_RESULT policyFlushSecret(TSS_HPOLICY a_hPolicy);

//Common TSPI functions
TSS_RESULT tpmGetPubEk(TSS_HTPM a_hTpm, TSS_BOOL a_fOwner,
                       TSS_VALIDATION * a_pValData, TSS_HKEY * a_phEPubKey);
TSS_RESULT tpmGetRandom(TSS_HTPM a_hTpm, UINT32 a_length, BYTE ** a_data);
TSS_RESULT tpmSetStatus(TSS_HTPM a_hTpm,
			TSS_FLAG a_fStatus, TSS_BOOL a_bValue);
TSS_RESULT tpmGetStatus(TSS_HTPM a_hTpm,
			TSS_FLAG a_fStatus, TSS_BOOL * a_bValue);
TSS_RESULT getCapability(TSS_HTPM a_hTpm,
			 TSS_FLAG a_fCapArea,
			 UINT32 a_uiSubCapLen,
			 BYTE * a_pSubCap,
			 UINT32 * a_uiResultLen, BYTE ** a_pResult);
TSS_RESULT getAttribData(TSS_HOBJECT a_hObject,
			 TSS_FLAG a_fAttr,
			 TSS_FLAG a_fSubAttr,
			 UINT32 * a_uiSize, BYTE ** a_pData);
TSS_RESULT getAttribUint32(TSS_HOBJECT a_hObject,
			   TSS_FLAG a_fAttr,
			   TSS_FLAG a_fSubAttr, UINT32 * a_uiData);

//TSPI key functions
TSS_RESULT keyLoadKey(TSS_HKEY a_hKey, TSS_HKEY a_hWrapKey);
TSS_RESULT keyLoadKeyByUUID(TSS_HCONTEXT a_hContext,
			    TSS_FLAG a_fStoreType,
			    TSS_UUID a_uKeyId, TSS_HKEY * a_hKey);
TSS_RESULT keyGetPubKey(TSS_HKEY a_hKey,
			UINT32 * a_uiKeyLen, BYTE ** a_pKey);
TSS_RESULT keyGetKeyByUUID(TSS_HCONTEXT a_hContext,
			   TSS_FLAG a_fStoreType,
			   TSS_UUID a_uKeyId, TSS_HKEY * a_hKey);

TSS_RESULT keyCreateKey(TSS_HKEY a_hKey, TSS_HKEY a_hWrapKey,
			TSS_HPCRS a_hPcrs);
TSS_RESULT dataSeal(TSS_HENCDATA a_hEncdata, TSS_HKEY a_hKey,
			UINT32 a_len, BYTE * a_data,
			TSS_HPCRS a_hPcrs);
TSS_RESULT tpmPcrRead(TSS_HTPM a_hTpm, UINT32 a_Idx,
			UINT32 *a_PcrSize, BYTE **a_PcrValue);
TSS_RESULT pcrcompositeSetPcrValue(TSS_HPCRS a_hPcrs, UINT32 a_Idx,
					UINT32 a_PcrSize, BYTE *a_PcrValue);
#ifdef TSS_LIB_IS_12
TSS_RESULT unloadVersionInfo(UINT64 *offset, BYTE *blob, TPM_CAP_VERSION_INFO *v);
TSS_RESULT pcrcompositeSetPcrLocality(TSS_HPCRS a_hPcrs, UINT32 localityValue);

TSS_RESULT NVDefineSpace(TSS_HNVSTORE hNVStore,
                         TSS_HPCRS hReadPcrComposite,
                         TSS_HPCRS hWritePcrComposite);

TSS_RESULT NVReleaseSpace(TSS_HNVSTORE hNVStore);

TSS_RESULT NVWriteValue(TSS_HNVSTORE hNVStore, UINT32 offset,
                        UINT32 ulDataLength, BYTE *rgbDataToWrite);

TSS_RESULT NVReadValue(TSS_HNVSTORE hNVStore, UINT32 offset,
                       UINT32 *ulDataLength, BYTE **rgbDataRead);

TSS_RESULT unloadNVDataPublic(UINT64 *offset, BYTE *blob, UINT32 bloblen,
                              TPM_NV_DATA_PUBLIC *v);
#endif

#endif
