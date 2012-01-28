/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005, 2006 International Business
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
#include "tpm_seal.h"
#include "tpm_unseal.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>

enum tspi_errors {
	ETSPICTXCREAT = 0,
	ETSPICTXCNCT,
	ETSPICTXCO,
	ETSPICTXLKBU,
	ETSPICTXLKBB,
	ETSPISETAD,
	ETSPIGETPO,
	ETSPIPOLSS,
	ETSPIDATU,
	ETSPIPOLATO,
};

TSS_HCONTEXT hContext = 0;
#define TSPI_FUNCTION_NAME_MAX 30
char tspi_error_strings[][TSPI_FUNCTION_NAME_MAX]= { 
				"Tspi_Context_Create",
				"Tspi_Context_Connect",
				"Tspi_Context_CreateObject",
				"Tspi_Context_LoadKeyByUUID",
				"Tspi_Context_LoadKeyByBlob",
				"Tspi_SetAttribData",
				"Tspi_GetPolicyObject",
				"Tspi_Policy_SetSecret",
				"Tspi_Data_Unseal",
				"Tspi_Policy_AssignToObject",
};

#define TSSKEY_DEFAULT_SIZE 768
#define EVPKEY_DEFAULT_SIZE 512

int tpm_errno;

int tpmUnsealFile( char* fname, unsigned char** tss_data, int* tss_size, 
		   BOOL srkWellKnown ) {

	int rc, rcLen=0, tssLen=0, evpLen=0;
	BYTE* rcPtr;
	char data[EVP_CIPHER_block_size(EVP_aes_256_cbc()) * 16];
	BYTE *tssKeyData = NULL;
	int tssKeyDataSize = 0;
	BYTE *evpKeyData = NULL;
	int evpKeyDataSize = 0;
	struct stat stats;
	TSS_HENCDATA hEncdata;
	TSS_HKEY hSrk, hKey;
	TSS_HPOLICY hPolicy;
	UINT32 symKeyLen;
	BYTE *symKey;
	BYTE wellKnown[TCPA_SHA1_160_HASH_LEN] = TSS_WELL_KNOWN_SECRET;
	char *srkSecret = NULL;
	int srkSecretLen;
	unsigned char* res_data = NULL;
	int res_size = 0;

	BIO *bdata = NULL, *b64 = NULL, *bmem = NULL;
	int bioRc;

	if ( tss_data == NULL || tss_size == NULL ) {
		rc = TPMSEAL_STD_ERROR;
		tpm_errno = EINVAL;
		goto out;
	}

	*tss_data = NULL;
	*tss_size = 0;

	/* Test for file existence */
	if ((rc = stat(fname, &stats))) {
		tpm_errno = errno;
		goto out;
	}	

	/* Create an input file BIO */
	if((bdata = BIO_new_file(fname, "r")) == NULL ) {
		tpm_errno = errno;
		rc = TPMSEAL_STD_ERROR;
		goto out;
	}

	/* Test file header for TSS */
	BIO_gets(bdata, data, sizeof(data));
	if (strncmp(data, TPMSEAL_HDR_STRING, 
			strlen(TPMSEAL_HDR_STRING)) != 0) {
		rc = TPMSEAL_FILE_ERROR;
		tpm_errno = ENOTSSHDR;
		goto out;
	}		

	/* Looking for TSS Key Header */
	BIO_gets(bdata, data, sizeof(data));
	if (strncmp(data, TPMSEAL_TSS_STRING, 
			strlen(TPMSEAL_TSS_STRING)) != 0) {
		rc = TPMSEAL_FILE_ERROR;
		tpm_errno = EWRONGTSSTAG;
		goto out;
	}

	/* Create a memory BIO to hold the base64 TSS key */
	if ((bmem = BIO_new(BIO_s_mem())) == NULL) {
		tpm_errno = EAGAIN;
		rc = TPMSEAL_STD_ERROR;
		goto out;
	}
	BIO_set_mem_eof_return(bmem, 0);

	/* Read the base64 TSS key into the memory BIO */
	while ((rcLen = BIO_gets(bdata, data, sizeof(data))) > 0) {
		/* Look for EVP Key Header (end of key) */
		if (strncmp(data, TPMSEAL_EVP_STRING,
				strlen(TPMSEAL_EVP_STRING)) == 0)
			break;

		if (BIO_write(bmem, data, rcLen) <= 0) {
			tpm_errno = EIO; 
			rc = TPMSEAL_STD_ERROR;
			goto out;
		}
	}
	if (strncmp(data, TPMSEAL_EVP_STRING, 
			strlen(TPMSEAL_EVP_STRING)) != 0 ) {
		tpm_errno = EWRONGEVPTAG;
		rc = TPMSEAL_FILE_ERROR;
		goto out;
	}

	/* Create a base64 BIO to decode the TSS key */
	if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
		tpm_errno = EAGAIN;
		rc = TPMSEAL_STD_ERROR;
		goto out;
	}

	/* Decode the TSS key */
	bmem = BIO_push( b64, bmem );
	while ((rcLen = BIO_read(bmem, data, sizeof(data))) > 0) {
		if ((tssLen + rcLen) > tssKeyDataSize) {
			tssKeyDataSize += TSSKEY_DEFAULT_SIZE;
			rcPtr = realloc( tssKeyData, tssKeyDataSize);
			if ( rcPtr == NULL ) {
				tpm_errno = ENOMEM;
				rc = TPMSEAL_STD_ERROR;
				goto out;
			}
			tssKeyData = rcPtr;
		}
		memcpy(tssKeyData + tssLen, data, rcLen);
		tssLen += rcLen;
	}
	bmem = BIO_pop(b64);
	BIO_free(b64);
	b64 = NULL;
	bioRc = BIO_reset(bmem);

	/* Check for EVP Key Type Header */
	BIO_gets(bdata, data, sizeof(data));
	if (strncmp(data, TPMSEAL_KEYTYPE_SYM, 
			strlen(TPMSEAL_KEYTYPE_SYM)) != 0 ) {
		rc = TPMSEAL_FILE_ERROR;
		tpm_errno = EWRONGKEYTYPE;
		goto out;
	}

	/* Make sure it's a supported cipher
	   (currently only AES 256 CBC) */
	if (strncmp(data + strlen(TPMSEAL_KEYTYPE_SYM),
			TPMSEAL_CIPHER_AES256CBC,
			strlen(TPMSEAL_CIPHER_AES256CBC)) != 0) {
		rc = TPMSEAL_FILE_ERROR;
		tpm_errno = EWRONGKEYTYPE;
		goto out;
	}

	/* Read the base64 Symmetric key into the memory BIO */
	while ((rcLen = BIO_gets(bdata, data, sizeof(data))) > 0) {
		/* Look for Encrypted Data Header (end of key) */
		if (strncmp(data, TPMSEAL_ENC_STRING,
				strlen(TPMSEAL_ENC_STRING)) == 0)
			break;

		if (BIO_write(bmem, data, rcLen) <= 0) {
			tpm_errno = EIO; 
			rc = TPMSEAL_STD_ERROR;
			goto out;
		}
	}
	if (strncmp(data, TPMSEAL_ENC_STRING, 
			strlen(TPMSEAL_ENC_STRING)) != 0 ) {
		tpm_errno = EWRONGDATTAG;
		rc = TPMSEAL_FILE_ERROR;
		goto out;
	}

	/* Create a base64 BIO to decode the Symmetric key */
	if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
		tpm_errno = EAGAIN;
		rc = TPMSEAL_STD_ERROR;
		goto out;
	}

	/* Decode the Symmetric key */
	bmem = BIO_push( b64, bmem );
	while ((rcLen = BIO_read(bmem, data, sizeof(data))) > 0) {
		if ((evpLen + rcLen) > evpKeyDataSize) {
			evpKeyDataSize += EVPKEY_DEFAULT_SIZE;
			rcPtr = realloc( evpKeyData, evpKeyDataSize);
			if ( rcPtr == NULL ) {
				tpm_errno = ENOMEM;
				rc = TPMSEAL_STD_ERROR;
				goto out;
			}
			evpKeyData = rcPtr;
		}
		memcpy(evpKeyData + evpLen, data, rcLen);
		evpLen += rcLen;
	}
	bmem = BIO_pop(b64);
	BIO_free(b64);
	b64 = NULL;
	bioRc = BIO_reset(bmem);

	/* Read the base64 encrypted data into the memory BIO */
	while ((rcLen = BIO_gets(bdata, data, sizeof(data))) > 0) {
		/* Look for TSS Footer (end of data) */
		if (strncmp(data, TPMSEAL_FTR_STRING,
				strlen(TPMSEAL_FTR_STRING)) == 0)
			break;

		if (BIO_write(bmem, data, rcLen) <= 0) {
			tpm_errno = EIO; 
			rc = TPMSEAL_STD_ERROR;
			goto out;
		}
	}
	if (strncmp(data, TPMSEAL_FTR_STRING, 
			strlen(TPMSEAL_FTR_STRING)) != 0 ) {
		tpm_errno = ENOTSSFTR;
		rc = TPMSEAL_FILE_ERROR;
		goto out;
	}

	/* Unseal */
	if ((rc=Tspi_Context_Create(&hContext)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXCREAT;
		goto out;
	}

	if (!srkWellKnown) {
		/* Prompt for SRK password */
		srkSecret = GETPASSWD(_("Enter SRK password: "), &srkSecretLen, FALSE);
		if (!srkSecret)
			goto out;
	}
	if ((rc=Tspi_Context_Connect(hContext, NULL)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXCNCT;
		goto tss_out;
	}
			
	if ((rc=Tspi_Context_CreateObject(hContext,
					TSS_OBJECT_TYPE_ENCDATA,
					TSS_ENCDATA_SEAL,
					&hEncdata)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXCO;
		goto tss_out;
	}
	        
	if ((rc=Tspi_SetAttribData(hEncdata,
				TSS_TSPATTRIB_ENCDATA_BLOB,
				TSS_TSPATTRIB_ENCDATABLOB_BLOB,
				evpLen, evpKeyData)) != TSS_SUCCESS) {
		tpm_errno = ETSPISETAD;
		goto tss_out;
	}

	if ((rc=Tspi_Context_CreateObject(hContext,
					TSS_OBJECT_TYPE_POLICY,
					TSS_POLICY_USAGE,
					&hPolicy)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXCO;
		goto tss_out;
	}

	if ((rc=Tspi_Policy_SetSecret(hPolicy, TSS_SECRET_MODE_PLAIN,
					strlen(TPMSEAL_SECRET),
					(BYTE *)TPMSEAL_SECRET)) != TSS_SUCCESS) {
		tpm_errno = ETSPIPOLSS;
		goto tss_out;
	}

	if ((rc=Tspi_Policy_AssignToObject(hPolicy, hEncdata)) != TSS_SUCCESS) {
		tpm_errno = ETSPIPOLATO;
		goto tss_out;
	}

	if ((rc=Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, 
					SRK_UUID, &hSrk)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXLKBU;
		goto tss_out;
	}

	/* Don't create a new policy for the SRK's secret, just use the context's
	 * default policy */
	if ((rc=Tspi_GetPolicyObject(hSrk, TSS_POLICY_USAGE, 
					&hPolicy)) != TSS_SUCCESS){
		tpm_errno = ETSPIGETPO;
		goto tss_out;
	}
	
	if (srkWellKnown)
		rc = Tspi_Policy_SetSecret(hPolicy, TSS_SECRET_MODE_SHA1,
				           sizeof(wellKnown),
				           (BYTE *) wellKnown);
	else
		rc = Tspi_Policy_SetSecret(hPolicy,TSS_SECRET_MODE_PLAIN,
					   srkSecretLen, 
					   (BYTE *) srkSecret);
					   
	if (rc != TSS_SUCCESS) {
		tpm_errno = ETSPIPOLSS;
		goto tss_out;
	}

	/* Failure point if trying to unseal data on a differnt TPM */
	if ((rc=Tspi_Context_LoadKeyByBlob(hContext, hSrk, tssLen, 
					tssKeyData, &hKey)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXLKBB;
		goto tss_out;
	}

	if ((rc=Tspi_Context_CreateObject(hContext,
					TSS_OBJECT_TYPE_POLICY,
					TSS_POLICY_USAGE,
					&hPolicy)) != TSS_SUCCESS) {
		tpm_errno = ETSPICTXCO;
		goto tss_out;
	}

	if ((rc=Tspi_Policy_SetSecret(hPolicy, TSS_SECRET_MODE_PLAIN,
					strlen(TPMSEAL_SECRET),
					(BYTE *)TPMSEAL_SECRET)) != TSS_SUCCESS) {
		tpm_errno = ETSPIPOLSS;
		goto tss_out;
	}

	if ((rc=Tspi_Policy_AssignToObject(hPolicy, hKey)) != TSS_SUCCESS) {
		tpm_errno = ETSPIPOLATO;
		goto tss_out;
	}

	if ((rc=Tspi_Data_Unseal(hEncdata, hKey, &symKeyLen,
					&symKey)) != TSS_SUCCESS) {
		tpm_errno = ETSPIDATU;
		goto tss_out;
	}

	/* Malloc a block of storage to hold the decrypted data
	   Using the size of the mem BIO is more than enough
	   (plus an extra cipher block size) */
	res_data = malloc(BIO_pending(bmem) + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
	if ( res_data == NULL ) {
		rc = TPMSEAL_STD_ERROR;
		tpm_errno = ENOMEM;
		goto tss_out;
	}

	/* Decode and decrypt the encrypted data */
	EVP_CIPHER_CTX ctx;
	EVP_DecryptInit(&ctx, EVP_aes_256_cbc(), symKey, (unsigned char *)TPMSEAL_IV);

	/* Create a base64 BIO to decode the encrypted data */
	if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
		tpm_errno = EAGAIN;
		rc = TPMSEAL_STD_ERROR;
		goto tss_out;
	}

	bmem = BIO_push( b64, bmem );
	while ((rcLen = BIO_read(bmem, data, sizeof(data))) > 0) {
		EVP_DecryptUpdate(&ctx, res_data+res_size,
					&rcLen, (unsigned char *)data, rcLen);
		res_size += rcLen;
	}
	EVP_DecryptFinal(&ctx, res_data+res_size, &rcLen);
	res_size += rcLen;
	bmem = BIO_pop(b64);
	BIO_free(b64);
	b64 = NULL;
	bioRc = BIO_reset(bmem);

tss_out:
	Tspi_Context_Close(hContext);
out:

	if (srkSecret)
		shredPasswd(srkSecret);
	
	if ( bdata )
		BIO_free(bdata);
	if ( b64 )
		BIO_free(b64);
	if ( bmem ) {
		bioRc = BIO_set_close(bmem, BIO_CLOSE);
		BIO_free(bmem);
	}

	if ( evpKeyData )
		free(evpKeyData);
	if ( tssKeyData )
		free(tssKeyData);

	if ( rc == 0 ) {
		*tss_data = res_data;
		*tss_size = res_size;
	} else
		free(res_data);

	return rc;
}

void tpmUnsealShred(unsigned char* data, int size) {

	if ( data != NULL ) {
		memset( data, 0, size);
		free(data);
	}

}

char tpm_error_buf[512];
char * tpmUnsealStrerror(int rc) {

	switch(rc) {
		case 0:
			return "Success";
		case TPMSEAL_STD_ERROR:
			return strerror(tpm_errno);
		case TPMSEAL_FILE_ERROR:
			switch(tpm_errno) {
				case ENOTSSHDR:
					return _("No TSS header present");
				case ENOTSSFTR:
					return _("No TSS footer present");
				case EWRONGTSSTAG:
					return _("Wrong TSS tag");
				case EWRONGEVPTAG:
					return _("Wrong EVP tag");
				case EWRONGDATTAG:
					return _("Wrong DATA tag");
				case EWRONGKEYTYPE:
					return _("Not a Symmetric EVP Key");
				case EBADSEEK:
					return _("Unable to move to desired file position");
			}
		default:
			snprintf(tpm_error_buf, sizeof(tpm_error_buf), 
				"%s: 0x%08x - layer=%s, code=%04x (%d), %s", 
				tspi_error_strings[tpm_errno],
				rc, Trspi_Error_Layer(rc), 
				Trspi_Error_Code(rc), 
				Trspi_Error_Code(rc), 
				Trspi_Error_String(rc)); 
			return tpm_error_buf;
	}
	return "";
}
