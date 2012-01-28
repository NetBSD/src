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

#include "tpm_tspi.h"

TSS_UUID SRK_UUID = TSS_UUID_SRK;
extern TSS_HCONTEXT hContext;

const char *mapUnknown = "Unknown";

const char *usageSigning = "Signing";
const char *usageStorage = "Storage";
const char *usageIdentity = "Identity";
const char *usageAuthChange = "AuthChange";
const char *usageBind = "Bind";
const char *usageLegacy = "Legacy";

const int flagMax = 7;
const char *flagMap[] = {
	"!VOLATILE, !MIGRATABLE, !REDIRECTION",
	"!VOLATILE, !MIGRATABLE,  REDIRECTION",
	"!VOLATILE,  MIGRATABLE, !REDIRECTION",
	"!VOLATILE,  MIGRATABLE,  REDIRECTION",
	" VOLATILE, !MIGRATABLE, !REDIRECTION",
	" VOLATILE, !MIGRATABLE,  REDIRECTION",
	" VOLATILE,  MIGRATABLE, !REDIRECTION",
	" VOLATILE,  MIGRATABLE,  REDIRECTION",
};

const char *authUsageNever = "Never";
const char *authUsageAlways = "Always";

const char *algRsa = "RSA";
const char *algDes = "DES";
const char *alg3Des = "3DES";
const char *algSha = "SHA";
const char *algHmac = "HMAC";
const char *algAes = "AES";

const char *encNone = "None";
const char *encRsaPkcs15 = "RSAESPKCSv15";
const char *encRsaOaepSha1Mgf1 = "RSAESOAEP_SHA1_MGF1";

const char *sigNone = "None";
const char *sigRsaPkcs15Sha1 = "RSASSAPKCS1v15_SHA1";
const char *sigRsaPkcs15Der = "RSASSAPKCS1v15_DER";


const char *displayKeyUsageMap(UINT32 a_uiData)
{

	switch (a_uiData) {
	case TPM_KEY_SIGNING:
		return usageSigning;

	case TPM_KEY_STORAGE:
		return usageStorage;

	case TPM_KEY_IDENTITY:
		return usageIdentity;

	case TPM_KEY_AUTHCHANGE:
		return usageAuthChange;

	case TPM_KEY_BIND:
		return usageBind;

	case TPM_KEY_LEGACY:
		return usageLegacy;
	}

	return mapUnknown;
}

const char *displayKeyFlagsMap(UINT32 a_uiFlags)
{

	int iPos = a_uiFlags & flagMax;

	return flagMap[iPos];
}

const char *displayAuthUsageMap(UINT32 a_uiData)
{

	switch (a_uiData) {
	case TPM_AUTH_NEVER:
		return authUsageNever;

	case TPM_AUTH_ALWAYS:
		return authUsageAlways;
	}

	return mapUnknown;
}

const char *displayAlgorithmMap(UINT32 a_uiData)
{

	switch (a_uiData) {
	case TCPA_ALG_RSA:
		return algRsa;

	case TCPA_ALG_DES:
		return algDes;

	case TCPA_ALG_3DES:
		return alg3Des;

	case TCPA_ALG_SHA:
		return algSha;

	case TCPA_ALG_HMAC:
		return algHmac;

	case TCPA_ALG_AES:
		return algAes;
	}

	return mapUnknown;
}

const char *displayEncSchemeMap(UINT32 a_uiData)
{

	switch (a_uiData) {
	case TCPA_ES_NONE:
		return encNone;

	case TCPA_ES_RSAESPKCSv15:
		return encRsaPkcs15;

	case TCPA_ES_RSAESOAEP_SHA1_MGF1:
		return encRsaOaepSha1Mgf1;
	}

	return mapUnknown;
}

const char *displaySigSchemeMap(UINT32 a_uiData)
{

	switch (a_uiData) {
	case TCPA_SS_NONE:
		return sigNone;

	case TCPA_SS_RSASSAPKCS1v15_SHA1:
		return sigRsaPkcs15Sha1;

	case TCPA_SS_RSASSAPKCS1v15_DER:
		return sigRsaPkcs15Der;
	}

	return mapUnknown;
}

TSS_RESULT displayKey(TSS_HKEY a_hKey)
{

	TSS_RESULT result;
	UINT32 uiAttr, uiAttrSize;
	BYTE *pAttr;
	UINT32 uiAlg;

	result =
	    getAttribData(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			  TSS_TSPATTRIB_KEYINFO_VERSION, &uiAttrSize,
			  &pAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Version:   "));
	logHex(uiAttrSize, pAttr);

	result =
	    getAttribUint32(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			    TSS_TSPATTRIB_KEYINFO_USAGE, &uiAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Usage:     0x%04x (%s)\n"), uiAttr, displayKeyUsageMap(uiAttr));

	result =
	    getAttribUint32(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			    TSS_TSPATTRIB_KEYINFO_KEYFLAGS, &uiAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Flags:     0x%08x (%s)\n"), uiAttr, displayKeyFlagsMap(uiAttr));

	result =
	    getAttribUint32(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			    TSS_TSPATTRIB_KEYINFO_AUTHUSAGE, &uiAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  AuthUsage: 0x%02x (%s)\n"), uiAttr, displayAuthUsageMap(uiAttr));

	result =
	    getAttribUint32(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			    TSS_TSPATTRIB_KEYINFO_ALGORITHM, &uiAlg);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Algorithm:         0x%08x (%s)\n"), uiAlg, displayAlgorithmMap(uiAlg));

	result =
	    getAttribUint32(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			    TSS_TSPATTRIB_KEYINFO_ENCSCHEME, &uiAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Encryption Scheme: 0x%08x (%s)\n"), uiAttr, displayEncSchemeMap(uiAttr));

	result =
	    getAttribUint32(a_hKey, TSS_TSPATTRIB_KEY_INFO,
			    TSS_TSPATTRIB_KEYINFO_SIGSCHEME, &uiAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Signature Scheme:  0x%08x (%s)\n"), uiAttr, displaySigSchemeMap(uiAttr));

	if (uiAlg == TCPA_ALG_RSA) {
		result =
		    getAttribUint32(a_hKey, TSS_TSPATTRIB_RSAKEY_INFO,
				    TSS_TSPATTRIB_KEYINFO_RSA_KEYSIZE,
				    &uiAttr);
		if (result != TSS_SUCCESS)
			return result;
		logMsg(_("  Key Size:          %d bits\n"), uiAttr);
	}

	result =
	    getAttribData(a_hKey, TSS_TSPATTRIB_RSAKEY_INFO,
			  TSS_TSPATTRIB_KEYINFO_RSA_MODULUS, &uiAttrSize,
			  &pAttr);
	if (result != TSS_SUCCESS)
		return result;
	logMsg(_("  Public Key:"));
	logHex(uiAttrSize, pAttr);

	return result;
}

/*
 * Not always reliable as this depends on the TSS system.data being intact
 */
BOOL isTpmOwned(TSS_HCONTEXT hContext)
{

	TSS_HKEY hSrk;
	BOOL iRc = FALSE;

	if (keyGetKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, SRK_UUID, &hSrk)
	    != TSS_SUCCESS)
		goto out;

	iRc = TRUE;

      out:
	return iRc;
}

void tspiDebug(const char *a_szName, TSS_RESULT a_iResult)
{

	logDebug(_("%s success\n"), a_szName);
}

void tspiError(const char *a_szName, TSS_RESULT a_iResult)
{

	logError(_("%s failed: 0x%08x - layer=%s, code=%04x (%d), %s\n"),
		 a_szName, a_iResult, Trspi_Error_Layer(a_iResult),
		 Trspi_Error_Code(a_iResult),
		 Trspi_Error_Code(a_iResult),
		 Trspi_Error_String(a_iResult));
}

void tspiResult(const char *a_szName, TSS_RESULT a_tResult)
{

	if (a_tResult == TSS_SUCCESS)
		tspiDebug(a_szName, a_tResult);
	else
		tspiError(a_szName, a_tResult);
}

BOOL mapTssBool(TSS_BOOL a_bValue)
{
	BOOL bRc;

	bRc = a_bValue ? TRUE : FALSE;

	return bRc;
}

TSS_RESULT contextCreate(TSS_HCONTEXT * a_hContext)
{
	TSS_RESULT result = Tspi_Context_Create(a_hContext);
	tspiResult("Tspi_Context_Create", result);

	return result;
}

TSS_RESULT contextClose(TSS_HCONTEXT a_hContext)
{

	TSS_RESULT result = Tspi_Context_FreeMemory(a_hContext, NULL);
	tspiResult("Tspi_Context_FreeMemory", result);

	result = Tspi_Context_Close(a_hContext);
	tspiResult("Tspi_Context_Close", result);

	return result;
}

TSS_RESULT contextConnect(TSS_HCONTEXT a_hContext)
{

	TSS_RESULT result = Tspi_Context_Connect(a_hContext, NULL);
	tspiResult("Tspi_Context_Connect", result);

	return result;
}


TSS_RESULT
contextCreateObject(TSS_HCONTEXT a_hContext,
		    TSS_FLAG a_fType,
		    TSS_FLAG a_fAttrs, TSS_HOBJECT * a_hObject)
{
	TSS_RESULT result =
	    Tspi_Context_CreateObject(a_hContext, a_fType, a_fAttrs,
				      a_hObject);
	tspiResult("Tspi_Context_CreateObject", result);

	return result;
}

TSS_RESULT
contextCloseObject(TSS_HCONTEXT a_hContext, TSS_HOBJECT a_hObject)
{
	TSS_RESULT result =
	    Tspi_Context_CloseObject(a_hContext, a_hObject);
	tspiResult("Tspi_Context_CloseObject", result);

	return result;
}

TSS_RESULT contextGetTpm(TSS_HCONTEXT a_hContext, TSS_HTPM * a_hTpm)
{

	TSS_RESULT result = Tspi_Context_GetTpmObject(a_hContext, a_hTpm);
	tspiResult("Tspi_Context_GetTpmObject", result);

	return result;
}


TSS_RESULT policyGet(TSS_HOBJECT a_hObject, TSS_HPOLICY * a_hPolicy)
{
	TSS_RESULT result =
	    Tspi_GetPolicyObject(a_hObject, TSS_POLICY_USAGE, a_hPolicy);
	tspiResult("Tspi_GetPolicyObject", result);

	return result;
}

TSS_RESULT policyAssign(TSS_HPOLICY a_hPolicy, TSS_HOBJECT a_hObject)
{
	TSS_RESULT result =
	    Tspi_Policy_AssignToObject(a_hPolicy, a_hObject);
	tspiResult("Tspi_Policy_AssignToObject", result);

	return result;
}

TSS_RESULT
policySetSecret(TSS_HPOLICY a_hPolicy,
		UINT32 a_uiSecretLen, BYTE * a_chSecret)
{
	TSS_RESULT result;
	BYTE wellKnown[] = TSS_WELL_KNOWN_SECRET;

	//If secret is TSS_WELL_KNOWN_SECRET, change secret mode to TSS_SECRET_MODE_SHA1
	if (a_chSecret &&
	    a_uiSecretLen == sizeof(wellKnown) &&
	    !memcmp(a_chSecret, (BYTE *)wellKnown, sizeof(wellKnown)))
		result =
			Tspi_Policy_SetSecret(a_hPolicy, TSS_SECRET_MODE_SHA1,
					a_uiSecretLen, a_chSecret);
	else
		result =
			Tspi_Policy_SetSecret(a_hPolicy, TSS_SECRET_MODE_PLAIN,
					a_uiSecretLen, a_chSecret);
	tspiResult("Tspi_Policy_SetSecret", result);

	return result;
}

TSS_RESULT policyFlushSecret(TSS_HPOLICY a_hPolicy)
{
	TSS_RESULT result = Tspi_Policy_FlushSecret(a_hPolicy);
	tspiResult("Tspi_Policy_FlushSecret", result);

	return result;
}

TSS_RESULT
tpmGetPubEk(TSS_HTPM a_hTpm,
	    TSS_BOOL a_fOwner,
	    TSS_VALIDATION * a_pValData, TSS_HKEY * a_phEPubKey)
{

	TSS_RESULT result = Tspi_TPM_GetPubEndorsementKey(a_hTpm, a_fOwner,
							  a_pValData,
							  a_phEPubKey);
	tspiResult("Tspi_TPM_GetPubEndorsementKey", result);

	return result;
}

TSS_RESULT
tpmSetStatus(TSS_HTPM a_hTpm, TSS_FLAG a_fStatus, TSS_BOOL a_bValue)
{

	TSS_RESULT result =
	    Tspi_TPM_SetStatus(a_hTpm, a_fStatus, a_bValue);
	tspiResult("Tspi_TPM_SetStatus", result);

	return result;
}

TSS_RESULT
tpmGetStatus(TSS_HTPM a_hTpm, TSS_FLAG a_fStatus, TSS_BOOL * a_bValue)
{

	TSS_RESULT result =
	    Tspi_TPM_GetStatus(a_hTpm, a_fStatus, a_bValue);
	tspiResult("Tspi_TPM_GetStatus", result);

	return result;
}

TSS_RESULT tpmGetRandom(TSS_HTPM a_hTpm, UINT32 a_length, BYTE ** a_data)
{

	TSS_RESULT result = Tspi_TPM_GetRandom(a_hTpm, a_length, a_data);
	tspiResult("Tspi_TPM_GetRandom", result);

	return result;
}


TSS_RESULT keyLoadKey(TSS_HKEY a_hKey, TSS_HKEY a_hWrapKey)
{

	TSS_RESULT result = Tspi_Key_LoadKey(a_hKey, a_hWrapKey);
	tspiResult("Tspi_Key_LoadKey", result);

	return result;
}

TSS_RESULT
keyLoadKeyByUUID(TSS_HCONTEXT a_hContext,
		 TSS_FLAG a_fStoreType,
		 TSS_UUID a_uKeyId, TSS_HKEY * a_hKey)
{
	TSS_RESULT result =
	    Tspi_Context_LoadKeyByUUID(a_hContext, a_fStoreType, a_uKeyId,
				       a_hKey);
	tspiResult("Tspi_Context_LoadKeyByUUID", result);

	return result;
}

TSS_RESULT
keyGetPubKey(TSS_HKEY a_hKey, UINT32 * a_uiKeyLen, BYTE ** a_pKey)
{

	TSS_RESULT result = Tspi_Key_GetPubKey(a_hKey, a_uiKeyLen, a_pKey);
	tspiResult("Tspi_Key_GetPubKey", result);

	return result;
}

TSS_RESULT
keyGetKeyByUUID(TSS_HCONTEXT a_hContext,
		TSS_FLAG a_fStoreType,
		TSS_UUID a_uKeyId, TSS_HKEY * a_hKey)
{

	TSS_RESULT result =
	    Tspi_Context_GetKeyByUUID(a_hContext, a_fStoreType, a_uKeyId,
				      a_hKey);
	tspiResult("Tspi_Context_GetKeyByUUID", result);

	return result;
}

TSS_RESULT
getAttribData(TSS_HOBJECT a_hObject,
	      TSS_FLAG a_fAttr,
	      TSS_FLAG a_fSubAttr, UINT32 * a_uiSize, BYTE ** a_pData)
{

	TSS_RESULT result =
	    Tspi_GetAttribData(a_hObject, a_fAttr, a_fSubAttr, a_uiSize,
			       a_pData);
	tspiResult("Tspi_GetAttribData", result);

	return result;
}

TSS_RESULT
getAttribUint32(TSS_HOBJECT a_hObject,
		TSS_FLAG a_fAttr, TSS_FLAG a_fSubAttr, UINT32 * a_uiData)
{

	TSS_RESULT result =
	    Tspi_GetAttribUint32(a_hObject, a_fAttr, a_fSubAttr, a_uiData);
	tspiResult("Tspi_GetAttribUint32", result);

	return result;
}

TSS_RESULT
getCapability(TSS_HTPM a_hTpm,
	      TSS_FLAG a_fCapArea,
	      UINT32 a_uiSubCapLen,
	      BYTE * a_pSubCap, UINT32 * a_uiResultLen, BYTE ** a_pResult)
{
	TSS_RESULT result =
	    Tspi_TPM_GetCapability(a_hTpm, a_fCapArea, a_uiSubCapLen,
				   a_pSubCap, a_uiResultLen, a_pResult);
	tspiResult("Tspi_TPM_GetCapability", result);

	return result;
}

TSS_RESULT 
keyCreateKey(TSS_HKEY a_hKey, TSS_HKEY a_hWrapKey,
		TSS_HPCRS a_hPcrs)
{
	TSS_RESULT result = Tspi_Key_CreateKey(a_hKey, a_hWrapKey, a_hPcrs);
	tspiResult("Tspi_Key_CreateKey", result);

	return result;
}

TSS_RESULT dataSeal(TSS_HENCDATA a_hEncdata, TSS_HKEY a_hKey,
			UINT32 a_len, BYTE * a_data,
			TSS_HPCRS a_hPcrs)
{

	TSS_RESULT result =
		Tspi_Data_Seal(a_hEncdata, a_hKey, a_len, a_data, a_hPcrs);
	tspiResult("Tspi_Data_Seal", result);

	return result;
}

TSS_RESULT
tpmPcrRead(TSS_HTPM a_hTpm, UINT32 a_Idx,
		UINT32 *a_PcrSize, BYTE **a_PcrValue)
{
	TSS_RESULT result =
		Tspi_TPM_PcrRead(a_hTpm, a_Idx, a_PcrSize, a_PcrValue);
	tspiResult("Tspi_TPM_PcrRead", result);

	return result;
}

TSS_RESULT
pcrcompositeSetPcrValue(TSS_HPCRS a_hPcrs, UINT32 a_Idx,
			UINT32 a_PcrSize, BYTE *a_PcrValue)
{
	TSS_RESULT result =
		Tspi_PcrComposite_SetPcrValue(a_hPcrs, a_Idx, a_PcrSize, a_PcrValue);
	tspiResult("Tspi_PcrComposite_SetPcrValue", result);

	return result;
}

#ifdef TSS_LIB_IS_12
/*
 * These getPasswd functions will wrap calls to the other functions and check to see if the TSS
 * library's context tells us to remove the NULL terminating chars from the end of the password
 * when unicode is on.
 */
char *
getPasswd12(const char *a_pszPrompt, int* a_iLen, BOOL a_bConfirm)
{
	return _getPasswd12( a_pszPrompt, a_iLen, a_bConfirm, useUnicode);
}

char *_getPasswd12(const char *a_pszPrompt, int* a_iLen, BOOL a_bConfirm, BOOL a_bUseUnicode)
{
	UINT32 status;
	char *passwd;

	passwd = _getPasswd(a_pszPrompt, a_iLen, a_bConfirm, a_bUseUnicode);

	if (passwd && a_bUseUnicode) {
		/* If we're running against a 1.2 TSS, it will include the null terminating
		 * characters based on the TSS_TSPATTRIB_SECRET_HASH_MODE attribute of the
		 * context. If this is set to TSS_TSPATTRIB_HASH_MODE_NOT_NULL, we need to
		 * trim the two zeros off the end of the unicode string returned by
		 * Trspi_Native_To_UNICODE. */
		if (getAttribUint32(hContext, TSS_TSPATTRIB_SECRET_HASH_MODE,
				    TSS_TSPATTRIB_SECRET_HASH_MODE_POPUP, &status))
			goto out;

		if (status == TSS_TSPATTRIB_HASH_MODE_NOT_NULL)
			*a_iLen -= sizeof(TSS_UNICODE);
	}
out:
	return passwd;
}

TSS_RESULT
unloadVersionInfo(UINT64 *offset, BYTE *blob, TPM_CAP_VERSION_INFO *v)
{
	TSS_RESULT result = Trspi_UnloadBlob_CAP_VERSION_INFO(offset, blob, v);
	tspiResult("Trspi_UnloadBlob_CAP_VERSION_INFO", result);
	return result;
}

TSS_RESULT
pcrcompositeSetPcrLocality(TSS_HPCRS a_hPcrs, UINT32 localityValue)
{
	TSS_RESULT result =
		Tspi_PcrComposite_SetPcrLocality(a_hPcrs, localityValue);
	tspiResult("Tspi_PcrComposite_SetPcrLocality", result);

	return result;
}

TSS_RESULT
NVDefineSpace(TSS_HNVSTORE hNVStore, TSS_HPCRS hReadPcrComposite ,
              TSS_HPCRS hWritePcrComposite)
{
	TSS_RESULT result =
	        Tspi_NV_DefineSpace(hNVStore, hReadPcrComposite,
	                            hWritePcrComposite);

	tspiResult("Tspi_NV_DefineSpace", result);

	return result;
}

TSS_RESULT
NVReleaseSpace(TSS_HNVSTORE hNVStore)
{
	TSS_RESULT result =
	        Tspi_NV_ReleaseSpace(hNVStore);

	tspiResult("Tspi_NV_ReleaseSpace", result);

	return result;
}

TSS_RESULT
NVWriteValue(TSS_HNVSTORE hNVStore, UINT32 offset,
             UINT32 ulDataLength, BYTE *rgbDataToWrite)
{
	TSS_RESULT result =
	        Tspi_NV_WriteValue(hNVStore, offset,
	                           ulDataLength, rgbDataToWrite);

	tspiResult("Tspi_NV_WriteValue", result);

	return result;
}

TSS_RESULT
NVReadValue(TSS_HNVSTORE hNVStore, UINT32 offset,
            UINT32 *ulDataLength, BYTE **rgbDataRead)
{
	TSS_RESULT result =
	        Tspi_NV_ReadValue(hNVStore, offset,
	                          ulDataLength, rgbDataRead);

	tspiResult("Tspi_NV_ReadValue", result);

	return result;
}

TSS_RESULT
unloadNVDataPublic(UINT64 *offset, BYTE *blob, UINT32 blob_len, TPM_NV_DATA_PUBLIC *v)
{
	UINT64 off = *offset;
	TSS_RESULT result;
	result = Trspi_UnloadBlob_NV_DATA_PUBLIC(&off, blob, NULL);
	if (result == TSS_SUCCESS) {
		if (off > blob_len)
			return TSS_E_BAD_PARAMETER;
		result = Trspi_UnloadBlob_NV_DATA_PUBLIC(offset, blob, v);
	}
	tspiResult("Trspi_UnloadBlob_NV_DATA_PUBLIC", result);
	return result;
}


#endif
