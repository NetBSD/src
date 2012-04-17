
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <langinfo.h>
#include <iconv.h>
#include <wchar.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "capabilities.h"
#include "tsplog.h"
#include "obj.h"


void
Trspi_UnloadBlob_NONCE(UINT64 *offset, BYTE *blob, TPM_NONCE *n)
{
	if (!n) {
		(*offset) += TPM_SHA1_160_HASH_LEN;
		return;
	}

	Trspi_UnloadBlob(offset, TPM_SHA1_160_HASH_LEN, blob, n->nonce);
}

void
Trspi_LoadBlob_NONCE(UINT64 *offset, BYTE *blob, TPM_NONCE *n)
{
	Trspi_LoadBlob(offset, TPM_SHA1_160_HASH_LEN, blob, n->nonce);
}

void
Trspi_LoadBlob_DIGEST(UINT64 *offset, BYTE *blob, TPM_DIGEST *digest)
{
	Trspi_LoadBlob(offset, TPM_SHA1_160_HASH_LEN, blob, digest->digest);
}

void
Trspi_UnloadBlob_DIGEST(UINT64 *offset, BYTE *blob, TPM_DIGEST *digest)
{
	if (!digest) {
		(*offset) += TPM_SHA1_160_HASH_LEN;
		return;
	}

	Trspi_UnloadBlob(offset, TPM_SHA1_160_HASH_LEN, blob, digest->digest);
}

void
Trspi_LoadBlob_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_PUBKEY *pubKey)
{
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &pubKey->algorithmParms);
	Trspi_LoadBlob_STORE_PUBKEY(offset, blob, &pubKey->pubKey);
}

TSS_RESULT
Trspi_UnloadBlob_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_PUBKEY *pubKey)
{
	TSS_RESULT result;

	if (!pubKey) {
		(void)Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);
		(void)Trspi_UnloadBlob_STORE_PUBKEY(offset, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = Trspi_UnloadBlob_KEY_PARMS(offset, blob, &pubKey->algorithmParms)))
		return result;
	if ((result = Trspi_UnloadBlob_STORE_PUBKEY(offset, blob, &pubKey->pubKey))) {
		free(pubKey->pubKey.key);
		free(pubKey->algorithmParms.parms);
		pubKey->pubKey.key = NULL;
		pubKey->pubKey.keyLength = 0;
		pubKey->algorithmParms.parms = NULL;
		pubKey->algorithmParms.parmSize = 0;
		return result;
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob(UINT64 *offset, size_t size, BYTE *to, BYTE *from)
{
	if (size == 0)
		return;
	if (to)
		memcpy(&to[(*offset)], from, size);
	(*offset) += size;
}

void
Trspi_UnloadBlob(UINT64 *offset, size_t size, BYTE *from, BYTE *to)
{
	if (size <= 0)
		return;
	if (to)
		memcpy(to, &from[*offset], size);
	(*offset) += size;
}

void
Trspi_LoadBlob_BYTE(UINT64 *offset, BYTE data, BYTE *blob)
{
	if (blob)
		blob[*offset] = data;
	(*offset)++;
}

void
Trspi_UnloadBlob_BYTE(UINT64 *offset, BYTE *dataOut, BYTE *blob)
{
	if (dataOut)
		*dataOut = blob[*offset];
	(*offset)++;
}

void
Trspi_LoadBlob_BOOL(UINT64 *offset, TSS_BOOL data, BYTE *blob)
{
	if (blob)
		blob[*offset] = (BYTE) data;
	(*offset)++;
}

void
Trspi_UnloadBlob_BOOL(UINT64 *offset, TSS_BOOL *dataOut, BYTE *blob)
{
	if (dataOut)
		*dataOut = blob[*offset];
	(*offset)++;
}

void
Trspi_LoadBlob_UINT64(UINT64 *offset, UINT64 in, BYTE *blob)
{
	if (blob)
		UINT64ToArray(in, &blob[*offset]);
	(*offset) += sizeof(UINT64);
}

void
Trspi_LoadBlob_UINT32(UINT64 *offset, UINT32 in, BYTE *blob)
{
	if (blob)
		UINT32ToArray(in, &blob[*offset]);
	(*offset) += sizeof(UINT32);
}

void
Trspi_LoadBlob_UINT16(UINT64 *offset, UINT16 in, BYTE *blob)
{
	if (blob)
		UINT16ToArray(in, &blob[*offset]);
	(*offset) += sizeof(UINT16);
}

void
Trspi_UnloadBlob_UINT64(UINT64 *offset, UINT64 *out, BYTE *blob)
{
	if (out)
		*out = Decode_UINT64(&blob[*offset]);
	(*offset) += sizeof(UINT64);
}

void
Trspi_UnloadBlob_UINT32(UINT64 *offset, UINT32 *out, BYTE *blob)
{
	if (out)
		*out = Decode_UINT32(&blob[*offset]);
	(*offset) += sizeof(UINT32);
}

void
Trspi_UnloadBlob_UINT16(UINT64 *offset, UINT16 *out, BYTE *blob)
{
	if (out)
		*out = Decode_UINT16(&blob[*offset]);
	(*offset) += sizeof(UINT16);
}

void
Trspi_LoadBlob_RSA_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_RSA_KEY_PARMS *parms)
{
	Trspi_LoadBlob_UINT32(offset, parms->keyLength, blob);
	Trspi_LoadBlob_UINT32(offset, parms->numPrimes, blob);
	Trspi_LoadBlob_UINT32(offset, parms->exponentSize, blob);

	if (parms->exponentSize > 0)
		Trspi_LoadBlob(offset, parms->exponentSize, blob, parms->exponent);
}

void
Trspi_UnloadBlob_TSS_VERSION(UINT64 *offset, BYTE *blob, TSS_VERSION *out)
{
	if (!out) {
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_BYTE(offset, &out->bMajor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->bMinor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->bRevMajor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->bRevMinor, blob);
}

void
Trspi_LoadBlob_TSS_VERSION(UINT64 *offset, BYTE *blob, TSS_VERSION version)
{
	Trspi_LoadBlob_BYTE(offset, version.bMajor, blob);
	Trspi_LoadBlob_BYTE(offset, version.bMinor, blob);
	Trspi_LoadBlob_BYTE(offset, version.bRevMajor, blob);
	Trspi_LoadBlob_BYTE(offset, version.bRevMinor, blob);
}

void
Trspi_UnloadBlob_TCPA_VERSION(UINT64 *offset, BYTE *blob, TCPA_VERSION *out)
{
	if (!out) {
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_BYTE(offset, &out->major, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->minor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->revMajor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->revMinor, blob);
}

void
Trspi_LoadBlob_TCPA_VERSION(UINT64 *offset, BYTE *blob, TCPA_VERSION version)
{
	Trspi_LoadBlob_BYTE(offset, version.major, blob);
	Trspi_LoadBlob_BYTE(offset, version.minor, blob);
	Trspi_LoadBlob_BYTE(offset, version.revMajor, blob);
	Trspi_LoadBlob_BYTE(offset, version.revMinor, blob);
}

TSS_RESULT
Trspi_UnloadBlob_PCR_INFO(UINT64 *offset, BYTE *blob, TCPA_PCR_INFO *pcr)
{
	TSS_RESULT result;

	if (!pcr) {
		(void)Trspi_UnloadBlob_PCR_SELECTION(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = Trspi_UnloadBlob_PCR_SELECTION(offset, blob, &pcr->pcrSelection)))
		return result;
	Trspi_UnloadBlob_DIGEST(offset, blob, &pcr->digestAtRelease);
	Trspi_UnloadBlob_DIGEST(offset, blob, &pcr->digestAtCreation);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_PCR_INFO(UINT64 *offset, BYTE *blob, TCPA_PCR_INFO *pcr)
{
	Trspi_LoadBlob_PCR_SELECTION(offset, blob, &pcr->pcrSelection);
	Trspi_LoadBlob_DIGEST(offset, blob, &pcr->digestAtRelease);
	Trspi_LoadBlob_DIGEST(offset, blob, &pcr->digestAtCreation);
}

TSS_RESULT
Trspi_UnloadBlob_PCR_INFO_LONG(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_LONG *pcr)
{
	TSS_RESULT result;

	if (!pcr) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_PCR_SELECTION(offset, blob, NULL);
		Trspi_UnloadBlob_PCR_SELECTION(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &pcr->tag, blob);
	Trspi_UnloadBlob_BYTE(offset, &pcr->localityAtCreation, blob);
	Trspi_UnloadBlob_BYTE(offset, &pcr->localityAtRelease, blob);
	if ((result = Trspi_UnloadBlob_PCR_SELECTION(offset, blob, &pcr->creationPCRSelection)))
		return result;
	if ((result = Trspi_UnloadBlob_PCR_SELECTION(offset, blob, &pcr->releasePCRSelection)))
		return result;
	Trspi_UnloadBlob_DIGEST(offset, blob, &pcr->digestAtCreation);
	Trspi_UnloadBlob_DIGEST(offset, blob, &pcr->digestAtRelease);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_PCR_INFO_LONG(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_LONG *pcr)
{
	Trspi_LoadBlob_UINT16(offset, pcr->tag, blob);
	Trspi_LoadBlob_BYTE(offset, pcr->localityAtCreation, blob);
	Trspi_LoadBlob_BYTE(offset, pcr->localityAtRelease, blob);
	Trspi_LoadBlob_PCR_SELECTION(offset, blob, &pcr->creationPCRSelection);
	Trspi_LoadBlob_PCR_SELECTION(offset, blob, &pcr->releasePCRSelection);
	Trspi_LoadBlob_DIGEST(offset, blob, &pcr->digestAtCreation);
	Trspi_LoadBlob_DIGEST(offset, blob, &pcr->digestAtRelease);
}

TSS_RESULT
Trspi_UnloadBlob_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_SHORT *pcr)
{
	TSS_RESULT result;

	if (!pcr) {
		Trspi_UnloadBlob_PCR_SELECTION(offset, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = Trspi_UnloadBlob_PCR_SELECTION(offset, blob, &pcr->pcrSelection)))
		return result;
	Trspi_UnloadBlob_BYTE(offset, &pcr->localityAtRelease, blob);
	Trspi_UnloadBlob_DIGEST(offset, blob, &pcr->digestAtRelease);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_SHORT *pcr)
{
	Trspi_LoadBlob_PCR_SELECTION(offset, blob, &pcr->pcrSelection);
	Trspi_LoadBlob_BYTE(offset, pcr->localityAtRelease, blob);
	Trspi_LoadBlob_DIGEST(offset, blob, &pcr->digestAtRelease);
}

TSS_RESULT
Trspi_UnloadBlob_PCR_SELECTION(UINT64 *offset, BYTE *blob, TCPA_PCR_SELECTION *pcr)
{
	if (!pcr) {
		UINT16 sizeOfSelect;

		Trspi_UnloadBlob_UINT16(offset, &sizeOfSelect, blob);
		Trspi_UnloadBlob(offset, sizeOfSelect, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &pcr->sizeOfSelect, blob);

	if (pcr->sizeOfSelect > 0) {
		pcr->pcrSelect = calloc(1, pcr->sizeOfSelect);
		if (pcr->pcrSelect == NULL) {
			LogError("malloc of %u bytes failed.", pcr->sizeOfSelect);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, pcr->sizeOfSelect, blob, pcr->pcrSelect);
	} else {
		pcr->pcrSelect = NULL;
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_PCR_SELECTION(UINT64 *offset, BYTE *blob, TCPA_PCR_SELECTION *pcr)
{
	UINT16 i;

	Trspi_LoadBlob_UINT16(offset, pcr->sizeOfSelect, blob);
	for (i = 0; i < pcr->sizeOfSelect; i++)
		Trspi_LoadBlob_BYTE(offset, pcr->pcrSelect[i], blob);
}

void
Trspi_LoadBlob_KEY12(UINT64 *offset, BYTE *blob, TPM_KEY12 *key)
{
	Trspi_LoadBlob_UINT16(offset, key->tag, blob);
	Trspi_LoadBlob_UINT16(offset, key->fill, blob);
	Trspi_LoadBlob_UINT16(offset, key->keyUsage, blob);
	Trspi_LoadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	Trspi_LoadBlob_BYTE(offset, key->authDataUsage, blob);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &key->algorithmParms);
	Trspi_LoadBlob_UINT32(offset, key->PCRInfoSize, blob);
	Trspi_LoadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	Trspi_LoadBlob_STORE_PUBKEY(offset, blob, &key->pubKey);
	Trspi_LoadBlob_UINT32(offset, key->encSize, blob);
	Trspi_LoadBlob(offset, key->encSize, blob, key->encData);
}

void
Trspi_LoadBlob_KEY(UINT64 *offset, BYTE *blob, TCPA_KEY *key)
{
	Trspi_LoadBlob_TCPA_VERSION(offset, blob, key->ver);
	Trspi_LoadBlob_UINT16(offset, key->keyUsage, blob);
	Trspi_LoadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	Trspi_LoadBlob_BYTE(offset, key->authDataUsage, blob);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &key->algorithmParms);
	Trspi_LoadBlob_UINT32(offset, key->PCRInfoSize, blob);
	Trspi_LoadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	Trspi_LoadBlob_STORE_PUBKEY(offset, blob, &key->pubKey);
	Trspi_LoadBlob_UINT32(offset, key->encSize, blob);
	Trspi_LoadBlob(offset, key->encSize, blob, key->encData);
}

void
Trspi_LoadBlob_KEY_FLAGS(UINT64 *offset, BYTE *blob, TCPA_KEY_FLAGS *flags)
{
	Trspi_LoadBlob_UINT32(offset, *flags, blob);
}

void
Trspi_UnloadBlob_KEY_FLAGS(UINT64 *offset, BYTE *blob, TCPA_KEY_FLAGS *flags)
{
	Trspi_UnloadBlob_UINT32(offset, flags, blob);
}

void
Trspi_LoadBlob_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_KEY_PARMS *keyInfo)
{
	Trspi_LoadBlob_UINT32(offset, keyInfo->algorithmID, blob);
	Trspi_LoadBlob_UINT16(offset, keyInfo->encScheme, blob);
	Trspi_LoadBlob_UINT16(offset, keyInfo->sigScheme, blob);
	Trspi_LoadBlob_UINT32(offset, keyInfo->parmSize, blob);

	if (keyInfo->parmSize > 0)
		Trspi_LoadBlob(offset, keyInfo->parmSize, blob, keyInfo->parms);
}

void
Trspi_LoadBlob_STORE_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_STORE_PUBKEY *store)
{
	Trspi_LoadBlob_UINT32(offset, store->keyLength, blob);
	Trspi_LoadBlob(offset, store->keyLength, blob, store->key);
}

void
Trspi_LoadBlob_UUID(UINT64 *offset, BYTE *blob, TSS_UUID uuid)
{
	Trspi_LoadBlob_UINT32(offset, uuid.ulTimeLow, blob);
	Trspi_LoadBlob_UINT16(offset, uuid.usTimeMid, blob);
	Trspi_LoadBlob_UINT16(offset, uuid.usTimeHigh, blob);
	Trspi_LoadBlob_BYTE(offset, uuid.bClockSeqHigh, blob);
	Trspi_LoadBlob_BYTE(offset, uuid.bClockSeqLow, blob);
	Trspi_LoadBlob(offset, 6, blob, uuid.rgbNode);
}

void
Trspi_UnloadBlob_UUID(UINT64 *offset, BYTE *blob, TSS_UUID *uuid)
{
	if (!uuid) {
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob(offset, 6, blob, NULL);

		return;
	}

	memset(uuid, 0, sizeof(TSS_UUID));
	Trspi_UnloadBlob_UINT32(offset, &uuid->ulTimeLow, blob);
	Trspi_UnloadBlob_UINT16(offset, &uuid->usTimeMid, blob);
	Trspi_UnloadBlob_UINT16(offset, &uuid->usTimeHigh, blob);
	Trspi_UnloadBlob_BYTE(offset, &uuid->bClockSeqHigh, blob);
	Trspi_UnloadBlob_BYTE(offset, &uuid->bClockSeqLow, blob);
	Trspi_UnloadBlob(offset, 6, blob, uuid->rgbNode);
}

TSS_RESULT
Trspi_UnloadBlob_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_KEY_PARMS *keyParms)
{
	if (!keyParms) {
		UINT32 parmSize;

		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, &parmSize, blob);

		(*offset) += parmSize;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &keyParms->algorithmID, blob);
	Trspi_UnloadBlob_UINT16(offset, &keyParms->encScheme, blob);
	Trspi_UnloadBlob_UINT16(offset, &keyParms->sigScheme, blob);
	Trspi_UnloadBlob_UINT32(offset, &keyParms->parmSize, blob);

	if (keyParms->parmSize > 0) {
		keyParms->parms = malloc(keyParms->parmSize);
		if (keyParms->parms == NULL) {
			LogError("malloc of %u bytes failed.", keyParms->parmSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, keyParms->parmSize, blob, keyParms->parms);
	} else {
		keyParms->parms = NULL;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_KEY12(UINT64 *offset, BYTE *blob, TPM_KEY12 *key)
{
	TSS_RESULT result;

	if (!key) {
		UINT32 PCRInfoSize, encSize;

		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_KEY_FLAGS(offset, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &PCRInfoSize, blob);
		Trspi_UnloadBlob(offset, PCRInfoSize, blob, NULL);
		Trspi_UnloadBlob_STORE_PUBKEY(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &encSize, blob);
		Trspi_UnloadBlob(offset, encSize, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &key->tag, blob);
	Trspi_UnloadBlob_UINT16(offset, &key->fill, blob);
	Trspi_UnloadBlob_UINT16(offset, &key->keyUsage, blob);
	Trspi_UnloadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	Trspi_UnloadBlob_BYTE(offset, &key->authDataUsage, blob);
	if ((result = Trspi_UnloadBlob_KEY_PARMS(offset, (BYTE *) blob, &key->algorithmParms)))
		return result;
	Trspi_UnloadBlob_UINT32(offset, &key->PCRInfoSize, blob);

	if (key->PCRInfoSize > 0) {
		key->PCRInfo = malloc(key->PCRInfoSize);
		if (key->PCRInfo == NULL) {
			LogError("malloc of %d bytes failed.", key->PCRInfoSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	} else {
		key->PCRInfo = NULL;
	}

	if ((result = Trspi_UnloadBlob_STORE_PUBKEY(offset, blob, &key->pubKey)))
		return result;
	Trspi_UnloadBlob_UINT32(offset, &key->encSize, blob);

	if (key->encSize > 0) {
		key->encData = malloc(key->encSize);
		if (key->encData == NULL) {
			LogError("malloc of %d bytes failed.", key->encSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->encSize, blob, key->encData);
	} else {
		key->encData = NULL;
	}

	return result;
}

TSS_RESULT
Trspi_UnloadBlob_KEY(UINT64 *offset, BYTE *blob, TCPA_KEY *key)
{
	TSS_RESULT result;

	if (!key) {
		UINT32 PCRInfoSize, encSize;

		Trspi_UnloadBlob_TCPA_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_KEY_FLAGS(offset, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &PCRInfoSize, blob);
		Trspi_UnloadBlob(offset, PCRInfoSize, blob, NULL);
		Trspi_UnloadBlob_STORE_PUBKEY(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &encSize, blob);
		Trspi_UnloadBlob(offset, encSize, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_TCPA_VERSION(offset, blob, &key->ver);
	Trspi_UnloadBlob_UINT16(offset, &key->keyUsage, blob);
	Trspi_UnloadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	Trspi_UnloadBlob_BYTE(offset, &key->authDataUsage, blob);
	if ((result = Trspi_UnloadBlob_KEY_PARMS(offset, (BYTE *) blob, &key->algorithmParms)))
		return result;
	Trspi_UnloadBlob_UINT32(offset, &key->PCRInfoSize, blob);

	if (key->PCRInfoSize > 0) {
		key->PCRInfo = malloc(key->PCRInfoSize);
		if (key->PCRInfo == NULL) {
			LogError("malloc of %d bytes failed.", key->PCRInfoSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	} else {
		key->PCRInfo = NULL;
	}

	if ((result = Trspi_UnloadBlob_STORE_PUBKEY(offset, blob, &key->pubKey)))
		return result;
	Trspi_UnloadBlob_UINT32(offset, &key->encSize, blob);

	if (key->encSize > 0) {
		key->encData = malloc(key->encSize);
		if (key->encData == NULL) {
			LogError("malloc of %d bytes failed.", key->encSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->encSize, blob, key->encData);
	} else {
		key->encData = NULL;
	}

	return result;
}

TSS_RESULT
Trspi_UnloadBlob_STORE_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_STORE_PUBKEY *store)
{
	if (!store) {
		UINT32 keyLength;

		Trspi_UnloadBlob_UINT32(offset, &keyLength, blob);
		Trspi_UnloadBlob(offset, keyLength, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &store->keyLength, blob);

	if (store->keyLength > 0) {
		store->key = malloc(store->keyLength);
		if (store->key == NULL) {
			LogError("malloc of %d bytes failed.", store->keyLength);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, store->keyLength, blob, store->key);
	} else {
		store->key = NULL;
	}

	return TSS_SUCCESS;
}

void
Trspi_UnloadBlob_VERSION(UINT64 *offset, BYTE *blob, TCPA_VERSION *out)
{
	if (!out) {
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_BYTE(offset, &out->major, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->minor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->revMajor, blob);
	Trspi_UnloadBlob_BYTE(offset, &out->revMinor, blob);
}

TSS_RESULT
Trspi_UnloadBlob_KM_KEYINFO(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO *info)
{
	if (!info) {
		UINT32 ulVendorDataLength;

		Trspi_UnloadBlob_TSS_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UUID(offset, blob, NULL);
		Trspi_UnloadBlob_UUID(offset, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BOOL(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, &ulVendorDataLength, blob);

		(*offset) += ulVendorDataLength;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_TSS_VERSION(offset, blob, &info->versionInfo);
	Trspi_UnloadBlob_UUID(offset, blob, &info->keyUUID);
	Trspi_UnloadBlob_UUID(offset, blob, &info->parentKeyUUID);
	Trspi_UnloadBlob_BYTE(offset, &info->bAuthDataUsage, blob);
	Trspi_UnloadBlob_BOOL(offset, &info->fIsLoaded, blob);
	Trspi_UnloadBlob_UINT32(offset, &info->ulVendorDataLength, blob);
	if (info->ulVendorDataLength > 0){
		/* allocate space for vendor data */
		info->rgbVendorData = malloc(info->ulVendorDataLength);
		if (info->rgbVendorData == NULL) {
			LogError("malloc of %u bytes failed.", info->ulVendorDataLength);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, info->ulVendorDataLength, blob, info->rgbVendorData);
	} else
		info->rgbVendorData = NULL;

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_KM_KEYINFO2(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO2 *info)
{
	if (!info) {
		UINT32 ulVendorDataLength;

		Trspi_UnloadBlob_TSS_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UUID(offset, blob, NULL);
		Trspi_UnloadBlob_UUID(offset, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_BOOL(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, &ulVendorDataLength, blob);

		(*offset) += ulVendorDataLength;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_TSS_VERSION(offset, blob, &info->versionInfo);
	Trspi_UnloadBlob_UUID(offset, blob, &info->keyUUID);
	Trspi_UnloadBlob_UUID(offset, blob, &info->parentKeyUUID);
	Trspi_UnloadBlob_BYTE(offset, &info->bAuthDataUsage, blob);
	/* Takes data regarding the new 2 fields of TSS_KM_KEYINFO2 */
	Trspi_UnloadBlob_UINT32(offset, &info->persistentStorageType, blob);
	Trspi_UnloadBlob_UINT32(offset, &info->persistentStorageTypeParent, blob);
	Trspi_UnloadBlob_BOOL(offset, &info->fIsLoaded, blob);
	Trspi_UnloadBlob_UINT32(offset, &info->ulVendorDataLength, blob);
	if (info->ulVendorDataLength > 0) {
		/* allocate space for vendor data */
		info->rgbVendorData = malloc(info->ulVendorDataLength);
		if (info->rgbVendorData == NULL) {
			LogError("malloc of %u bytes failed.", info->ulVendorDataLength);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, info->ulVendorDataLength, blob, info->rgbVendorData);
	} else
		info->rgbVendorData = NULL;

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_PCR_EVENT(UINT64 *offset, BYTE *blob, TSS_PCR_EVENT *event)
{
	Trspi_LoadBlob_TCPA_VERSION(offset, blob, *(TCPA_VERSION *)(&event->versionInfo));
	Trspi_LoadBlob_UINT32(offset, event->ulPcrIndex, blob);
	Trspi_LoadBlob_UINT32(offset, event->eventType, blob);

	Trspi_LoadBlob_UINT32(offset, event->ulPcrValueLength, blob);
	if (event->ulPcrValueLength > 0)
		Trspi_LoadBlob(offset, event->ulPcrValueLength, blob, event->rgbPcrValue);

	Trspi_LoadBlob_UINT32(offset, event->ulEventLength, blob);
	if (event->ulEventLength > 0)
		Trspi_LoadBlob(offset, event->ulEventLength, blob, event->rgbEvent);

}

TSS_RESULT
Trspi_UnloadBlob_PCR_EVENT(UINT64 *offset, BYTE *blob, TSS_PCR_EVENT *event)
{
	if (!event) {
		UINT32 ulPcrValueLength, ulEventLength;

		Trspi_UnloadBlob_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		Trspi_UnloadBlob_UINT32(offset, &ulPcrValueLength, blob);
		(*offset) += ulPcrValueLength;

		Trspi_UnloadBlob_UINT32(offset, &ulEventLength, blob);
		(*offset) += ulEventLength;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_VERSION(offset, blob, (TCPA_VERSION *)&(event->versionInfo));
	Trspi_UnloadBlob_UINT32(offset, &event->ulPcrIndex, blob);
	Trspi_UnloadBlob_UINT32(offset, &event->eventType, blob);

	Trspi_UnloadBlob_UINT32(offset, &event->ulPcrValueLength, blob);
	if (event->ulPcrValueLength > 0) {
		event->rgbPcrValue = malloc(event->ulPcrValueLength);
		if (event->rgbPcrValue == NULL) {
			LogError("malloc of %u bytes failed.", event->ulPcrValueLength);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, event->ulPcrValueLength, blob, event->rgbPcrValue);
	} else {
		event->rgbPcrValue = NULL;
	}

	Trspi_UnloadBlob_UINT32(offset, &event->ulEventLength, blob);
	if (event->ulEventLength > 0) {
		event->rgbEvent = malloc(event->ulEventLength);
		if (event->rgbEvent == NULL) {
			LogError("malloc of %d bytes failed.", event->ulEventLength);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, event->ulEventLength, blob, event->rgbEvent);
	} else {
		event->rgbEvent = NULL;
	}

	return TSS_SUCCESS;
}

/* loads a blob with the info needed to hash when creating the private key area
 * of a TPM_KEY(12) from an external source
 */
void
Trspi_LoadBlob_PRIVKEY_DIGEST12(UINT64 *offset, BYTE *blob, TPM_KEY12 *key)
{
	Trspi_LoadBlob_UINT16(offset, key->tag, blob);
	Trspi_LoadBlob_UINT16(offset, key->fill, blob);
	Trspi_LoadBlob_UINT16(offset, key->keyUsage, blob);
	Trspi_LoadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	Trspi_LoadBlob_BYTE(offset, key->authDataUsage, blob);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &key->algorithmParms);

	Trspi_LoadBlob_UINT32(offset, key->PCRInfoSize, blob);
	/* exclude pcrInfo when PCRInfoSize is 0 as spec'd in TPM 1.1b spec p.71 */
	if (key->PCRInfoSize != 0)
		Trspi_LoadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);

	Trspi_LoadBlob_STORE_PUBKEY(offset, blob, &key->pubKey);
	/* exclude encSize, encData as spec'd in TPM 1.1b spec p.71 */
}

void
Trspi_LoadBlob_PRIVKEY_DIGEST(UINT64 *offset, BYTE *blob, TCPA_KEY *key)
{
	Trspi_LoadBlob_TCPA_VERSION(offset, blob, key->ver);
	Trspi_LoadBlob_UINT16(offset, key->keyUsage, blob);
	Trspi_LoadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	Trspi_LoadBlob_BYTE(offset, key->authDataUsage, blob);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &key->algorithmParms);

	Trspi_LoadBlob_UINT32(offset, key->PCRInfoSize, blob);
	/* exclude pcrInfo when PCRInfoSize is 0 as spec'd in TPM 1.1b spec p.71 */
	if (key->PCRInfoSize != 0)
		Trspi_LoadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);

	Trspi_LoadBlob_STORE_PUBKEY(offset, blob, &key->pubKey);
	/* exclude encSize, encData as spec'd in TPM 1.1b spec p.71 */
}

void
Trspi_LoadBlob_SYMMETRIC_KEY(UINT64 *offset, BYTE *blob, TCPA_SYMMETRIC_KEY *key)
{
	Trspi_LoadBlob_UINT32(offset, key->algId, blob);
	Trspi_LoadBlob_UINT16(offset, key->encScheme, blob);
	Trspi_LoadBlob_UINT16(offset, key->size, blob);

	if (key->size > 0)
		Trspi_LoadBlob(offset, key->size, blob, key->data);
}

TSS_RESULT
Trspi_UnloadBlob_SYMMETRIC_KEY(UINT64 *offset, BYTE *blob, TCPA_SYMMETRIC_KEY *key)
{
	if (!key) {
		UINT16 size;

		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, &size, blob);
		(*offset) += size;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &key->algId, blob);
	Trspi_UnloadBlob_UINT16(offset, &key->encScheme, blob);
	Trspi_UnloadBlob_UINT16(offset, &key->size, blob);

	if (key->size > 0) {
		key->data = malloc(key->size);
		if (key->data == NULL) {
			key->size = 0;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->size, blob, key->data);
	} else {
		key->data = NULL;
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_IDENTITY_REQ(UINT64 *offset, BYTE *blob, TCPA_IDENTITY_REQ *req)
{
	Trspi_LoadBlob_UINT32(offset, req->asymSize, blob);
	Trspi_LoadBlob_UINT32(offset, req->symSize, blob);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &req->asymAlgorithm);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &req->symAlgorithm);
	Trspi_LoadBlob(offset, req->asymSize, blob, req->asymBlob);
	Trspi_LoadBlob(offset, req->symSize, blob, req->symBlob);
}

void
Trspi_LoadBlob_CHANGEAUTH_VALIDATE(UINT64 *offset, BYTE *blob, TPM_CHANGEAUTH_VALIDATE *caValidate)
{
	Trspi_LoadBlob(offset, TCPA_SHA1_160_HASH_LEN, blob, caValidate->newAuthSecret.authdata);
	Trspi_LoadBlob(offset, TCPA_SHA1_160_HASH_LEN, blob, caValidate->n1.nonce);
}

TSS_RESULT
Trspi_UnloadBlob_IDENTITY_REQ(UINT64 *offset, BYTE *blob, TCPA_IDENTITY_REQ *req)
{
	TSS_RESULT result;

	if (!req) {
		UINT32 asymSize, symSize;

		Trspi_UnloadBlob_UINT32(offset, &asymSize, blob);
		Trspi_UnloadBlob_UINT32(offset, &symSize, blob);
		(void)Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);
		(void)Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);

		(*offset) += asymSize;
		(*offset) += symSize;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &req->asymSize, blob);
	Trspi_UnloadBlob_UINT32(offset, &req->symSize, blob);
	if ((result = Trspi_UnloadBlob_KEY_PARMS(offset, blob, &req->asymAlgorithm)))
		return result;
	if ((Trspi_UnloadBlob_KEY_PARMS(offset, blob, &req->symAlgorithm))) {
		free(req->asymAlgorithm.parms);
		req->asymAlgorithm.parmSize = 0;
		return result;
	}

	if (req->asymSize > 0) {
		req->asymBlob = malloc(req->asymSize);
		if (req->asymBlob == NULL) {
			req->asymSize = 0;
			req->asymAlgorithm.parmSize = 0;
			free(req->asymAlgorithm.parms);
			req->symAlgorithm.parmSize = 0;
			free(req->symAlgorithm.parms);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, req->asymSize, blob, req->asymBlob);
	} else {
		req->asymBlob = NULL;
	}

	if (req->symSize > 0) {
		req->symBlob = malloc(req->symSize);
		if (req->symBlob == NULL) {
			req->symSize = 0;
			req->asymSize = 0;
			free(req->asymBlob);
			req->asymBlob = NULL;
			req->asymAlgorithm.parmSize = 0;
			free(req->asymAlgorithm.parms);
			req->symAlgorithm.parmSize = 0;
			free(req->symAlgorithm.parms);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, req->symSize, blob, req->symBlob);
	} else {
		req->symBlob = NULL;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_IDENTITY_PROOF(UINT64 *offset, BYTE *blob, TCPA_IDENTITY_PROOF *proof)
{
	TSS_RESULT result;

	if (!proof) {
		UINT32 labelSize, identityBindingSize, endorsementSize, platformSize;
		UINT32 conformanceSize;

		Trspi_UnloadBlob_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &labelSize, blob);
		Trspi_UnloadBlob_UINT32(offset, &identityBindingSize, blob);
		Trspi_UnloadBlob_UINT32(offset, &endorsementSize, blob);
		Trspi_UnloadBlob_UINT32(offset, &platformSize, blob);
		Trspi_UnloadBlob_UINT32(offset, &conformanceSize, blob);

		(void)Trspi_UnloadBlob_PUBKEY(offset, blob, NULL);

		(*offset) += labelSize;
		(*offset) += identityBindingSize;
		(*offset) += endorsementSize;
		(*offset) += platformSize;
		(*offset) += conformanceSize;

		return TSS_SUCCESS;
	}

	/* helps when an error occurs */
	memset(proof, 0, sizeof(TCPA_IDENTITY_PROOF));

	Trspi_UnloadBlob_VERSION(offset, blob, (TCPA_VERSION *)&proof->ver);
	Trspi_UnloadBlob_UINT32(offset, &proof->labelSize, blob);
	Trspi_UnloadBlob_UINT32(offset, &proof->identityBindingSize, blob);
	Trspi_UnloadBlob_UINT32(offset, &proof->endorsementSize, blob);
	Trspi_UnloadBlob_UINT32(offset, &proof->platformSize, blob);
	Trspi_UnloadBlob_UINT32(offset, &proof->conformanceSize, blob);

	if ((result = Trspi_UnloadBlob_PUBKEY(offset, blob,
					      &proof->identityKey))) {
		proof->labelSize = 0;
		proof->identityBindingSize = 0;
		proof->endorsementSize = 0;
		proof->platformSize = 0;
		proof->conformanceSize = 0;
		return result;
	}

	if (proof->labelSize > 0) {
		proof->labelArea = malloc(proof->labelSize);
		if (proof->labelArea == NULL) {
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		Trspi_UnloadBlob(offset, proof->labelSize, blob, proof->labelArea);
	} else {
		proof->labelArea = NULL;
	}

	if (proof->identityBindingSize > 0) {
		proof->identityBinding = malloc(proof->identityBindingSize);
		if (proof->identityBinding == NULL) {
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		Trspi_UnloadBlob(offset, proof->identityBindingSize, blob,
				 proof->identityBinding);
	} else {
		proof->identityBinding = NULL;
	}

	if (proof->endorsementSize > 0) {
		proof->endorsementCredential = malloc(proof->endorsementSize);
		if (proof->endorsementCredential == NULL) {
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		Trspi_UnloadBlob(offset, proof->endorsementSize, blob,
				 proof->endorsementCredential);
	} else {
		proof->endorsementCredential = NULL;
	}

	if (proof->platformSize > 0) {
		proof->platformCredential = malloc(proof->platformSize);
		if (proof->platformCredential == NULL) {
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		Trspi_UnloadBlob(offset, proof->platformSize, blob,
				 proof->platformCredential);
	} else {
		proof->platformCredential = NULL;
	}

	if (proof->conformanceSize > 0) {
		proof->conformanceCredential = malloc(proof->conformanceSize);
		if (proof->conformanceCredential == NULL) {
			result = TSPERR(TSS_E_OUTOFMEMORY);
			goto error;
		}
		Trspi_UnloadBlob(offset, proof->conformanceSize, blob,
				 proof->conformanceCredential);
	} else {
		proof->conformanceCredential = NULL;
	}

	return TSS_SUCCESS;
error:
	proof->labelSize = 0;
	proof->identityBindingSize = 0;
	proof->endorsementSize = 0;
	proof->platformSize = 0;
	proof->conformanceSize = 0;
	free(proof->labelArea);
	proof->labelArea = NULL;
	free(proof->identityBinding);
	proof->identityBinding = NULL;
	free(proof->endorsementCredential);
	proof->endorsementCredential = NULL;
	free(proof->conformanceCredential);
	proof->conformanceCredential = NULL;
	/* free identityKey */
	free(proof->identityKey.pubKey.key);
	free(proof->identityKey.algorithmParms.parms);
	proof->identityKey.pubKey.key = NULL;
	proof->identityKey.pubKey.keyLength = 0;
	proof->identityKey.algorithmParms.parms = NULL;
	proof->identityKey.algorithmParms.parmSize = 0;

	return result;
}

void
Trspi_LoadBlob_SYM_CA_ATTESTATION(UINT64 *offset, BYTE *blob, TCPA_SYM_CA_ATTESTATION *sym)
{
	Trspi_LoadBlob_UINT32(offset, sym->credSize, blob);
	Trspi_LoadBlob_KEY_PARMS(offset, blob, &sym->algorithm);
	Trspi_LoadBlob(offset, sym->credSize, blob, sym->credential);
}

TSS_RESULT
Trspi_UnloadBlob_SYM_CA_ATTESTATION(UINT64 *offset, BYTE *blob, TCPA_SYM_CA_ATTESTATION *sym)
{
	TSS_RESULT result;

	if (!sym) {
		UINT32 credSize;

		Trspi_UnloadBlob_UINT32(offset, &credSize, blob);
		(void)Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);

		(*offset) += credSize;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &sym->credSize, blob);
	if ((result = Trspi_UnloadBlob_KEY_PARMS(offset, blob, &sym->algorithm))) {
		sym->credSize = 0;
		return result;
	}

	if (sym->credSize > 0) {
		if ((sym->credential = malloc(sym->credSize)) == NULL) {
			free(sym->algorithm.parms);
			sym->algorithm.parmSize = 0;
			sym->credSize = 0;
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, sym->credSize, blob, sym->credential);
	} else {
		sym->credential = NULL;
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_ASYM_CA_CONTENTS(UINT64 *offset, BYTE *blob, TCPA_ASYM_CA_CONTENTS *asym)
{
	Trspi_LoadBlob_SYMMETRIC_KEY(offset, blob, &asym->sessionKey);
	Trspi_LoadBlob(offset, TCPA_SHA1_160_HASH_LEN, blob,
		       (BYTE *)&asym->idDigest);
}

TSS_RESULT
Trspi_UnloadBlob_ASYM_CA_CONTENTS(UINT64 *offset, BYTE *blob, TCPA_ASYM_CA_CONTENTS *asym)
{
	TSS_RESULT result;

	if (!asym) {
		(void)Trspi_UnloadBlob_SYMMETRIC_KEY(offset, blob, NULL);
		Trspi_UnloadBlob(offset, TCPA_SHA1_160_HASH_LEN, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = Trspi_UnloadBlob_SYMMETRIC_KEY(offset, blob, &asym->sessionKey)))
		return result;

	Trspi_UnloadBlob(offset, TCPA_SHA1_160_HASH_LEN, blob, (BYTE *)&asym->idDigest);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_BOUND_DATA(UINT64 *offset, TCPA_BOUND_DATA bd, UINT32 payloadLength, BYTE *blob)
{
	Trspi_LoadBlob_TCPA_VERSION(offset, blob, bd.ver);
	Trspi_LoadBlob(offset, 1, blob, &bd.payload);
	Trspi_LoadBlob(offset, payloadLength, blob, bd.payloadData);
}

/* function to mimic strerror with TSS error codes */
char *
Trspi_Error_String(TSS_RESULT r)
{
	/* Check the return code to see if it is common to all layers.
	 * If so, return it.
	 */
	switch (TSS_ERROR_CODE(r)) {
		case TSS_SUCCESS:			return "Success";
		default:
			break;
	}

	/* The return code is either unknown, or specific to a layer */
	if (TSS_ERROR_LAYER(r) == TSS_LAYER_TPM) {
		switch (TSS_ERROR_CODE(r)) {
			case TPM_E_AUTHFAIL:			return "Authentication failed";
			case TPM_E_BAD_PARAMETER:		return "Bad Parameter";
			case TPM_E_BADINDEX:			return "Bad memory index";
			case TPM_E_AUDITFAILURE:		return "Audit failure";
			case TPM_E_CLEAR_DISABLED:		return "Clear has been disabled";
			case TPM_E_DEACTIVATED:			return "TPM is deactivated";
			case TPM_E_DISABLED:			return "TPM is disabled";
			case TPM_E_FAIL:			return "Operation failed";
			case TPM_E_BAD_ORDINAL:			return "Ordinal was unknown or inconsistent";
			case TPM_E_INSTALL_DISABLED:		return "Owner install disabled";
			case TPM_E_INVALID_KEYHANDLE:		return "Invalid keyhandle";
			case TPM_E_KEYNOTFOUND:			return "Key not found";
			case TPM_E_INAPPROPRIATE_ENC:		return "Bad encryption scheme";
			case TPM_E_MIGRATEFAIL:			return "Migration authorization failed";
			case TPM_E_INVALID_PCR_INFO:		return "PCR information uninterpretable";
			case TPM_E_NOSPACE:			return "No space to load key";
			case TPM_E_NOSRK:			return "No SRK";
			case TPM_E_NOTSEALED_BLOB:		return "Encrypted blob invalid";
			case TPM_E_OWNER_SET:			return "Owner already set";
			case TPM_E_RESOURCES:			return "Insufficient TPM resources";
			case TPM_E_SHORTRANDOM:			return "Random string too short";
			case TPM_E_SIZE:			return "TPM out of space";
			case TPM_E_WRONGPCRVAL:			return "Wrong PCR value";
			case TPM_E_BAD_PARAM_SIZE:		return "Bad input size";
			case TPM_E_SHA_THREAD:			return "No existing SHA-1 thread";
			case TPM_E_SHA_ERROR:			return "SHA-1 error";
			case TPM_E_FAILEDSELFTEST:		return "Self-test failed, TPM shutdown";
			case TPM_E_AUTH2FAIL:			return "Second authorization session failed";
			case TPM_E_BADTAG:			return "Invalid tag";
			case TPM_E_IOERROR:			return "I/O error";
			case TPM_E_ENCRYPT_ERROR:		return "Encryption error";
			case TPM_E_DECRYPT_ERROR:		return "Decryption error";
			case TPM_E_INVALID_AUTHHANDLE:		return "Invalid authorization handle";
			case TPM_E_NO_ENDORSEMENT:		return "No EK";
			case TPM_E_INVALID_KEYUSAGE:		return "Invalid key usage";
			case TPM_E_WRONG_ENTITYTYPE:		return "Invalid entity type";
			case TPM_E_INVALID_POSTINIT:		return "Invalid POST init sequence";
			case TPM_E_INAPPROPRIATE_SIG:		return "Invalid signature format";
			case TPM_E_BAD_KEY_PROPERTY:		return "Unsupported key parameters";
			case TPM_E_BAD_MIGRATION:		return "Invalid migration properties";
			case TPM_E_BAD_SCHEME:			return "Invalid signature or encryption scheme";
			case TPM_E_BAD_DATASIZE:		return "Invalid data size";
			case TPM_E_BAD_MODE:			return "Bad mode parameter";
			case TPM_E_BAD_PRESENCE:		return "Bad physical presence value";
			case TPM_E_BAD_VERSION:			return "Invalid version";
			case TPM_E_NO_WRAP_TRANSPORT:		return "TPM does not allow for wrapped transport sessions";
			case TPM_E_AUDITFAIL_UNSUCCESSFUL:	return "TPM audit construction failed and the underlying command was returning a failure code also";
			case TPM_E_AUDITFAIL_SUCCESSFUL:	return "TPM audit construction failed and the underlying command was returning success";
			case TPM_E_NOTRESETABLE:		return "Attempt to reset a PCR register that does not have the resettable attribute";
			case TPM_E_NOTLOCAL:			return "Attempt to reset a PCR register that requires locality and locality modifier not part of command transport";
			case TPM_E_BAD_TYPE:			return "Make identity blob not properly typed";
			case TPM_E_INVALID_RESOURCE:		return "When saving context identified resource type does not match actual resource";
			case TPM_E_NOTFIPS:			return "TPM is attempting to execute a command only available when in FIPS mode";
			case TPM_E_INVALID_FAMILY:		return "Command is attempting to use an invalid family ID";
			case TPM_E_NO_NV_PERMISSION:		return "Permission to manipulate the NV storage is not available";
			case TPM_E_REQUIRES_SIGN:		return "Operation requires a signed command";
			case TPM_E_KEY_NOTSUPPORTED:		return "Wrong operation to load an NV key";
			case TPM_E_AUTH_CONFLICT:		return "NV_LoadKey blob requires both owner and blob authorization";
			case TPM_E_AREA_LOCKED:			return "NV area is locked and not writable";
			case TPM_E_BAD_LOCALITY:		return "Locality is incorrect for attempted operation";
			case TPM_E_READ_ONLY:			return "NV area is read only and can't be written to";
			case TPM_E_PER_NOWRITE:			return "There is no protection on write to NV area";
			case TPM_E_FAMILYCOUNT:			return "Family count value does not match";
			case TPM_E_WRITE_LOCKED:		return "NV area has already been written to";
			case TPM_E_BAD_ATTRIBUTES:		return "NV area attributes conflict";
			case TPM_E_INVALID_STRUCTURE:		return "Structure tag and version are invalid or inconsistent";
			case TPM_E_KEY_OWNER_CONTROL:		return "Key is under control of TPM Owner and can only be evicted by TPM Owner";
			case TPM_E_BAD_COUNTER:			return "Counter handle is incorrect";
			case TPM_E_NOT_FULLWRITE:		return "Write is not a complete write of area";
			case TPM_E_CONTEXT_GAP:			return "Gap between saved context counts is too large";
			case TPM_E_MAXNVWRITES:			return "Maximum number of NV writes without an owner has been exceeded";
			case TPM_E_NOOPERATOR:			return "No operator AuthData value is set";
			case TPM_E_RESOURCEMISSING:		return "Resource pointed to by context is not loaded";
			case TPM_E_DELEGATE_LOCK:		return "Delegate administration is locked";
			case TPM_E_DELEGATE_FAMILY:		return "Attempt to manage a family other then delegated family";
			case TPM_E_DELEGATE_ADMIN:		return "Delegation table management not enabled";
			case TPM_E_TRANSPORT_NOTEXCLUSIVE:	return "A command was executed outside of an exclusive transport session";
			case TPM_E_OWNER_CONTROL:		return "Attempt to context save an owner evict-controlled key";
			case TPM_E_DAA_RESOURCES:		return "DAA command has no resources available to execute command";
			case TPM_E_DAA_INPUT_DATA0:		return "Consistency check on DAA parameter inputData0 has failed";
			case TPM_E_DAA_INPUT_DATA1:		return "Consistency check on DAA parameter inputData1 has failed";
			case TPM_E_DAA_ISSUER_SETTINGS:		return "Consistency check on DAA_issuerSettings has failed";
			case TPM_E_DAA_TPM_SETTINGS:		return "Consistency check on DAA_tpmSpecific has failed";
			case TPM_E_DAA_STAGE:			return "Atomic process indicated by submitted DAA command is not expected process";
			case TPM_E_DAA_ISSUER_VALIDITY:		return "Issuer's validity check has detected an inconsistency";
			case TPM_E_DAA_WRONG_W:			return "Consistency check on w has failed";
			case TPM_E_BAD_HANDLE:			return "Handle is incorrect";
			case TPM_E_BAD_DELEGATE:		return "Delegation is not correct";
			case TPM_E_BADCONTEXT:			return "Context blob is invalid";
			case TPM_E_TOOMANYCONTEXTS:		return "Too many contexts held by TPM";
			case TPM_E_MA_TICKET_SIGNATURE:		return "Migration authority signature validation failure";
			case TPM_E_MA_DESTINATION:		return "Migration destination not authenticated";
			case TPM_E_MA_SOURCE:			return "Migration source incorrect";
			case TPM_E_MA_AUTHORITY:		return "Incorrect migration authority";
			case TPM_E_PERMANENTEK:			return "Attempt to revoke EK but EK is not revocable";
			case TPM_E_BAD_SIGNATURE:		return "Bad signature of CMK ticket";
			case TPM_E_NOCONTEXTSPACE:		return "No room in context list for additional contexts";
			case TPM_E_RETRY:			return "TPM busy: Retry command at a later time";
			case TPM_E_NEEDS_SELFTEST:		return "SelfTestFull has not been run";
			case TPM_E_DOING_SELFTEST:		return "TPM is currently executing a full selftest";
			case TPM_E_DEFEND_LOCK_RUNNING:		return "TPM is defending against dictionary attacks and is in some time-out period";
			case TPM_E_DISABLED_CMD:		return "The TPM target command has been disabled";
			default:				return "Unknown error";
		}
	} else if (TSS_ERROR_LAYER(r) == TSS_LAYER_TDDL) {
		switch (TSS_ERROR_CODE(r)) {
			case TSS_E_FAIL:			return "General failure";
			case TSS_E_BAD_PARAMETER:		return "Bad parameter";
			case TSS_E_INTERNAL_ERROR:		return "Internal software error";
			case TSS_E_NOTIMPL:			return "Not implemented";
			case TSS_E_PS_KEY_NOTFOUND:		return "Key not found in persistent storage";
			case TSS_E_KEY_ALREADY_REGISTERED:	return "UUID already registered";
			case TSS_E_CANCELED:			return "The action was cancelled by request";
			case TSS_E_TIMEOUT:			return "The operation has timed out";
			case TSS_E_OUTOFMEMORY:			return "Out of memory";
			case TSS_E_TPM_UNEXPECTED:		return "Unexpected TPM output";
			case TSS_E_COMM_FAILURE:		return "Communication failure";
			case TSS_E_TPM_UNSUPPORTED_FEATURE:	return "Unsupported feature";
			case TDDL_E_COMPONENT_NOT_FOUND:	return "Connection to TPM device failed";
			case TDDL_E_ALREADY_OPENED:		return "Device already opened";
			case TDDL_E_BADTAG:			return "Invalid or unsupported capability";
			case TDDL_E_INSUFFICIENT_BUFFER:	return "Receive buffer too small";
			case TDDL_E_COMMAND_COMPLETED:		return "Command has already completed";
			case TDDL_E_COMMAND_ABORTED:		return "TPM aborted processing of command";
			case TDDL_E_ALREADY_CLOSED:		return "Device driver already closed";
			case TDDL_E_IOERROR:			return "I/O error";
			default:				return "Unknown";
		}
	} else if (TSS_ERROR_LAYER(r) == TSS_LAYER_TCS) {
		switch (TSS_ERROR_CODE(r)) {
			case TSS_E_FAIL:			return "General failure";
			case TSS_E_BAD_PARAMETER:		return "Bad parameter";
			case TSS_E_INTERNAL_ERROR:		return "Internal software error";
			case TSS_E_NOTIMPL:			return "Not implemented";
			case TSS_E_PS_KEY_NOTFOUND:		return "Key not found in persistent storage";
			case TSS_E_KEY_ALREADY_REGISTERED:	return "UUID already registered";
			case TSS_E_CANCELED:			return "The action was cancelled by request";
			case TSS_E_TIMEOUT:			return "The operation has timed out";
			case TSS_E_OUTOFMEMORY:			return "Out of memory";
			case TSS_E_TPM_UNEXPECTED:		return "Unexpected TPM output";
			case TSS_E_COMM_FAILURE:		return "Communication failure";
			case TSS_E_TPM_UNSUPPORTED_FEATURE:	return "Unsupported feature";
			case TCS_E_KEY_MISMATCH:		return "UUID does not match key handle";
			case TCS_E_KM_LOADFAILED:		return "Key load failed: parent key requires authorization";
			case TCS_E_KEY_CONTEXT_RELOAD:		return "Reload of key context failed";
			case TCS_E_BAD_INDEX:			return "Bad memory index";
			case TCS_E_INVALID_CONTEXTHANDLE:	return "Invalid context handle";
			case TCS_E_INVALID_KEYHANDLE:		return "Invalid key handle";
			case TCS_E_INVALID_AUTHHANDLE:		return "Invalid authorization session handle";
			case TCS_E_INVALID_AUTHSESSION:		return "Authorization session has been closed by TPM";
			case TCS_E_INVALID_KEY:			return "Invalid key";
			default:				return "Unknown";
		}
	} else {
		switch (TSS_ERROR_CODE(r)) {
			case TSS_E_FAIL:			return "General failure";
			case TSS_E_BAD_PARAMETER:		return "Bad parameter";
			case TSS_E_INTERNAL_ERROR:		return "Internal software error";
			case TSS_E_NOTIMPL:			return "Not implemented";
			case TSS_E_PS_KEY_NOTFOUND:		return "Key not found in persistent storage";
			case TSS_E_KEY_ALREADY_REGISTERED:	return "UUID already registered";
			case TSS_E_CANCELED:			return "The action was cancelled by request";
			case TSS_E_TIMEOUT:			return "The operation has timed out";
			case TSS_E_OUTOFMEMORY:			return "Out of memory";
			case TSS_E_TPM_UNEXPECTED:		return "Unexpected TPM output";
			case TSS_E_COMM_FAILURE:		return "Communication failure";
			case TSS_E_TPM_UNSUPPORTED_FEATURE:	return "Unsupported feature";
			case TSS_E_INVALID_OBJECT_TYPE:		return "Object type not valid for this operation";
			case TSS_E_INVALID_OBJECT_INITFLAG:	return "Wrong flag information for object creation";
			case TSS_E_INVALID_HANDLE:		return "Invalid handle";
			case TSS_E_NO_CONNECTION:		return "Core service connection doesn't exist";
			case TSS_E_CONNECTION_FAILED:		return "Core service connection failed";
			case TSS_E_CONNECTION_BROKEN:		return "Communication with core services failed";
			case TSS_E_HASH_INVALID_ALG:		return "Invalid hash algorithm";
			case TSS_E_HASH_INVALID_LENGTH:		return "Hash length is inconsistent with algorithm";
			case TSS_E_HASH_NO_DATA:		return "Hash object has no internal hash value";
			case TSS_E_SILENT_CONTEXT:		return "A silent context requires user input";
			case TSS_E_INVALID_ATTRIB_FLAG:		return "Flag value for attrib-functions inconsistent";
			case TSS_E_INVALID_ATTRIB_SUBFLAG:	return "Sub-flag value for attrib-functions inconsistent";
			case TSS_E_INVALID_ATTRIB_DATA:		return "Data for attrib-functions invalid";
			case TSS_E_NO_PCRS_SET:			return "No PCR registers are selected or set";
			case TSS_E_KEY_NOT_LOADED:		return "The addressed key is not currently loaded";
			case TSS_E_KEY_NOT_SET:			return "No key informatio is currently available";
			case TSS_E_VALIDATION_FAILED:		return "Internal validation of data failed";
			case TSS_E_TSP_AUTHREQUIRED:		return "Authorization is required";
			case TSS_E_TSP_AUTH2REQUIRED:		return "Multiple authorizations are required";
			case TSS_E_TSP_AUTHFAIL:		return "Authorization failed";
			case TSS_E_TSP_AUTH2FAIL:		return "Multiple authorization failed";
			case TSS_E_KEY_NO_MIGRATION_POLICY:	return "Addressed key has no migration policy";
			case TSS_E_POLICY_NO_SECRET:		return "No secret information available for the address policy";
			case TSS_E_INVALID_OBJ_ACCESS:		return "Accessed object is in an inconsistent state";
			case TSS_E_INVALID_ENCSCHEME:		return "Invalid encryption scheme";
			case TSS_E_INVALID_SIGSCHEME:		return "Invalid signature scheme";
			case TSS_E_ENC_INVALID_LENGTH:		return "Invalid length for encrypted data object";
			case TSS_E_ENC_NO_DATA:			return "Encrypted data object contains no data";
			case TSS_E_ENC_INVALID_TYPE:		return "Invalid type for encrypted data object";
			case TSS_E_INVALID_KEYUSAGE:		return "Invalid usage of key";
			case TSS_E_VERIFICATION_FAILED:		return "Internal validation of data failed";
			case TSS_E_HASH_NO_IDENTIFIER:		return "Hash algorithm identifier not set";
			case TSS_E_NV_AREA_EXIST:		return "NVRAM area already exists";
			case TSS_E_NV_AREA_NOT_EXIST:		return "NVRAM area does not exist";
			default:				return "Unknown";
		}
	}
}

char *
Trspi_Error_Layer(TSS_RESULT r)
{
	switch (TSS_ERROR_LAYER(r)) {
		case TSS_LAYER_TPM:	return "tpm";
		case TSS_LAYER_TDDL:	return "tddl";
		case TSS_LAYER_TCS:	return "tcs";
		case TSS_LAYER_TSP:	return "tsp";
		default:		return "unknown";
	}
}

TSS_RESULT
Trspi_Error_Code(TSS_RESULT r)
{
	return TSS_ERROR_CODE(r);
}

static int
hacky_strlen(char *codeset, BYTE *string)
{
	BYTE *ptr = string;
	int len = 0;

	if (strcmp("UTF-16", codeset) == 0) {
		while (!(ptr[0] == '\0' && ptr[1] == '\0')) {
			len += 2;
			ptr += 2;
		}
	} else if (strcmp("UTF-32", codeset) == 0) {
		while (!(ptr[0] == '\0' && ptr[1] == '\0' &&
			 ptr[2] == '\0' && ptr[3] == '\0')) {
			len += 4;
			ptr += 4;
		}
	} else {
		/* default to 8bit chars */
		while (*ptr++ != '\0') {
			len++;
		}
	}

	return len;
}

static inline int
char_width(char *codeset)
{
	if (strcmp("UTF-16", codeset) == 0) {
		return 2;
	} else if (strcmp("UTF-32", codeset) == 0) {
		return 4;
	}

	return 1;
}

#define MAX_BUF_SIZE	4096

BYTE *
Trspi_Native_To_UNICODE(BYTE *string, unsigned *size)
{
	char *ret, *outbuf, tmpbuf[MAX_BUF_SIZE] = { 0, };
	BSD_CONST char *ptr;
	unsigned len = 0, tmplen;
	iconv_t cd = 0;
	size_t rc, outbytesleft, inbytesleft;

	if (string == NULL)
		goto alloc_string;

	if ((cd = iconv_open("UTF-16LE", nl_langinfo(CODESET))) == (iconv_t)-1) {
		LogDebug("iconv_open: %s", strerror(errno));
		return NULL;
	}

	if ((tmplen = hacky_strlen(nl_langinfo(CODESET), string)) == 0) {
		LogDebug("hacky_strlen returned 0");
		goto alloc_string;
	}

	do {
		len++;
		outbytesleft = len;
		inbytesleft = tmplen;
		outbuf = tmpbuf;
		ptr = (char *)string;
		errno = 0;

		rc = iconv(cd, (BSD_CONST char **)&ptr, &inbytesleft, &outbuf, &outbytesleft);
	} while (rc == (size_t)-1 && errno == E2BIG);

	if (len > MAX_BUF_SIZE) {
		LogDebug("string too long.");
		iconv_close(cd);
		return NULL;
	}

alloc_string:
	/* add terminating bytes of the correct width */
	len += char_width("UTF-16");
	if ((ret = calloc(1, len)) == NULL) {
		LogDebug("malloc of %u bytes failed.", len);
		iconv_close(cd);
		return NULL;
	}

	memcpy(ret, &tmpbuf, len);
	if (size)
		*size = len;

	if (cd)
		iconv_close(cd);

	return (BYTE *)ret;

}

BYTE *
Trspi_UNICODE_To_Native(BYTE *string, unsigned *size)
{
	char *ret, *outbuf, tmpbuf[MAX_BUF_SIZE] = { 0, };
	BSD_CONST char *ptr;
	unsigned len = 0, tmplen;
	iconv_t cd;
	size_t rc, outbytesleft, inbytesleft;

	if (string == NULL) {
		if (size)
			*size = 0;
		return NULL;
	}

	if ((cd = iconv_open(nl_langinfo(CODESET), "UTF-16LE")) == (iconv_t)-1) {
		LogDebug("iconv_open: %s", strerror(errno));
		return NULL;
	}

	if ((tmplen = hacky_strlen("UTF-16", string)) == 0) {
		LogDebug("hacky_strlen returned 0");
		return 0;
	}

	do {
		len++;
		outbytesleft = len;
		inbytesleft = tmplen;
		outbuf = tmpbuf;
		ptr = (char *)string;
		errno = 0;

		rc = iconv(cd, (BSD_CONST char **)&ptr, &inbytesleft, &outbuf, &outbytesleft);
	} while (rc == (size_t)-1 && errno == E2BIG);

	/* add terminating bytes of the correct width */
	len += char_width(nl_langinfo(CODESET));
	if (len > MAX_BUF_SIZE) {
		LogDebug("string too long.");
		iconv_close(cd);
		return NULL;
	}

	if ((ret = calloc(1, len)) == NULL) {
		LogDebug("malloc of %d bytes failed.", len);
		iconv_close(cd);
		return NULL;
	}

	memcpy(ret, &tmpbuf, len);
	if (size)
		*size = len;
	iconv_close(cd);

	return (BYTE *)ret;
}

/* Functions to support incremental hashing */
TSS_RESULT
Trspi_Hash_UINT16(Trspi_HashCtx *c, UINT16 i)
{
	BYTE bytes[sizeof(UINT16)];

	UINT16ToArray(i, bytes);
	return Trspi_HashUpdate(c, sizeof(UINT16), bytes);
}

TSS_RESULT
Trspi_Hash_UINT32(Trspi_HashCtx *c, UINT32 i)
{
	BYTE bytes[sizeof(UINT32)];

	UINT32ToArray(i, bytes);
	return Trspi_HashUpdate(c, sizeof(UINT32), bytes);
}

TSS_RESULT
Trspi_Hash_UINT64(Trspi_HashCtx *c, UINT64 i)
{
	BYTE bytes[sizeof(UINT64)];

	UINT64ToArray(i, bytes);
	return Trspi_HashUpdate(c, sizeof(UINT64), bytes);
}

TSS_RESULT
Trspi_Hash_BYTE(Trspi_HashCtx *c, BYTE data)
{
	return Trspi_HashUpdate(c, sizeof(BYTE), &data);
}

TSS_RESULT
Trspi_Hash_BOOL(Trspi_HashCtx *c, TSS_BOOL data)
{
	return Trspi_HashUpdate(c, (UINT32)sizeof(TSS_BOOL), (BYTE *)&data);
}

TSS_RESULT
Trspi_Hash_VERSION(Trspi_HashCtx *c, TSS_VERSION *version)
{
	TSS_RESULT result;

	result = Trspi_Hash_BYTE(c, version->bMajor);
	result |= Trspi_Hash_BYTE(c, version->bMinor);
	result |= Trspi_Hash_BYTE(c, version->bRevMajor);
	result |= Trspi_Hash_BYTE(c, version->bRevMinor);

	return result;
}

TSS_RESULT
Trspi_Hash_DAA_PK(Trspi_HashCtx *c, TSS_DAA_PK *pk)
{
	UINT32 i;
	TSS_RESULT result;

	result = Trspi_Hash_VERSION(c, &pk->versionInfo);

	result |= Trspi_Hash_UINT32(c, pk->modulusLength);
	result |= Trspi_HashUpdate(c, pk->modulusLength, pk->modulus);

	result |= Trspi_Hash_UINT32(c, pk->capitalSLength);
	result |= Trspi_HashUpdate(c, pk->capitalSLength, pk->capitalS);

	result |= Trspi_Hash_UINT32(c, pk->capitalZLength);
	result |= Trspi_HashUpdate(c, pk->capitalZLength, pk->capitalZ);

	result |= Trspi_Hash_UINT32(c, pk->capitalR0Length);
	result |= Trspi_HashUpdate(c, pk->capitalR0Length, pk->capitalR0);

	result |= Trspi_Hash_UINT32(c, pk->capitalR1Length);
	result |= Trspi_HashUpdate(c, pk->capitalR1Length, pk->capitalR1);

	result |= Trspi_Hash_UINT32(c, pk->gammaLength);
	result |= Trspi_HashUpdate(c, pk->gammaLength, pk->gamma);

	result |= Trspi_Hash_UINT32(c, pk->capitalGammaLength);
	result |= Trspi_HashUpdate(c, pk->capitalGammaLength, pk->capitalGamma);

	result |= Trspi_Hash_UINT32(c, pk->rhoLength);
	result |= Trspi_HashUpdate(c, pk->rhoLength, pk->rho);

	for (i = 0; i < pk->capitalYLength; i++)
		result |= Trspi_HashUpdate(c, pk->capitalYLength2, pk->capitalY[i]);

	result |= Trspi_Hash_UINT32(c, pk->capitalYPlatformLength);

	result |= Trspi_Hash_UINT32(c, pk->issuerBaseNameLength);
	result |= Trspi_HashUpdate(c, pk->issuerBaseNameLength, pk->issuerBaseName);

	return result;
}

TSS_RESULT
Trspi_Hash_RSA_KEY_PARMS(Trspi_HashCtx *c, TCPA_RSA_KEY_PARMS *parms)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, parms->keyLength);
	result |= Trspi_Hash_UINT32(c, parms->numPrimes);
	result |= Trspi_Hash_UINT32(c, parms->exponentSize);

	if (parms->exponentSize > 0)
		result |= Trspi_HashUpdate(c, parms->exponentSize, parms->exponent);

	return result;
}

TSS_RESULT
Trspi_Hash_STORE_PUBKEY(Trspi_HashCtx *c, TCPA_STORE_PUBKEY *store)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, store->keyLength);
	result |= Trspi_HashUpdate(c, store->keyLength, store->key);

	return result;
}

TSS_RESULT
Trspi_Hash_KEY_PARMS(Trspi_HashCtx *c, TCPA_KEY_PARMS *keyInfo)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, keyInfo->algorithmID);
	result |= Trspi_Hash_UINT16(c, keyInfo->encScheme);
	result |= Trspi_Hash_UINT16(c, keyInfo->sigScheme);
	result |= Trspi_Hash_UINT32(c, keyInfo->parmSize);

	if (keyInfo->parmSize > 0)
		result |= Trspi_HashUpdate(c, keyInfo->parmSize, keyInfo->parms);

	return result;
}

TSS_RESULT
Trspi_Hash_PUBKEY(Trspi_HashCtx *c, TCPA_PUBKEY *pubKey)
{
	TSS_RESULT result;

	result = Trspi_Hash_KEY_PARMS(c, &pubKey->algorithmParms);
	result |= Trspi_Hash_STORE_PUBKEY(c, &pubKey->pubKey);

	return result;
}

TSS_RESULT
Trspi_Hash_STORED_DATA(Trspi_HashCtx *c, TCPA_STORED_DATA *data)
{
	TSS_RESULT result;

	result = Trspi_Hash_VERSION(c, (TSS_VERSION *)&data->ver);
	result |= Trspi_Hash_UINT32(c, data->sealInfoSize);
	result |= Trspi_HashUpdate(c, data->sealInfoSize, data->sealInfo);
	result |= Trspi_Hash_UINT32(c, data->encDataSize);
	result |= Trspi_HashUpdate(c, data->encDataSize, data->encData);

	return result;
}

TSS_RESULT
Trspi_Hash_PCR_SELECTION(Trspi_HashCtx *c, TCPA_PCR_SELECTION *pcr)
{
	TSS_RESULT result;
	UINT16 i;

	result = Trspi_Hash_UINT16(c, pcr->sizeOfSelect);

	for (i = 0; i < pcr->sizeOfSelect; i++)
		result |= Trspi_Hash_BYTE(c, pcr->pcrSelect[i]);

	return result;
}

TSS_RESULT
Trspi_Hash_KEY_FLAGS(Trspi_HashCtx *c, TCPA_KEY_FLAGS *flags)
{
	return Trspi_Hash_UINT32(c, *flags);
}

TSS_RESULT
Trspi_Hash_KEY12(Trspi_HashCtx *c, TPM_KEY12 *key)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, key->tag);
	result |= Trspi_Hash_UINT16(c, key->fill);
	result |= Trspi_Hash_UINT16(c, key->keyUsage);
	result |= Trspi_Hash_KEY_FLAGS(c, &key->keyFlags);
	result |= Trspi_Hash_BYTE(c, key->authDataUsage);
	result |= Trspi_Hash_KEY_PARMS(c, &key->algorithmParms);
	result |= Trspi_Hash_UINT32(c, key->PCRInfoSize);
	result |= Trspi_HashUpdate(c, key->PCRInfoSize, key->PCRInfo);
	result |= Trspi_Hash_STORE_PUBKEY(c, &key->pubKey);
	result |= Trspi_Hash_UINT32(c, key->encSize);
	result |= Trspi_HashUpdate(c, key->encSize, key->encData);

	return result;
}

TSS_RESULT
Trspi_Hash_KEY(Trspi_HashCtx *c, TCPA_KEY *key)
{
	TSS_RESULT result;

	result = Trspi_Hash_VERSION(c, (TSS_VERSION *)&key->ver);
	result |= Trspi_Hash_UINT16(c, key->keyUsage);
	result |= Trspi_Hash_KEY_FLAGS(c, &key->keyFlags);
	result |= Trspi_Hash_BYTE(c, key->authDataUsage);
	result |= Trspi_Hash_KEY_PARMS(c, &key->algorithmParms);
	result |= Trspi_Hash_UINT32(c, key->PCRInfoSize);
	result |= Trspi_HashUpdate(c, key->PCRInfoSize, key->PCRInfo);
	result |= Trspi_Hash_STORE_PUBKEY(c, &key->pubKey);
	result |= Trspi_Hash_UINT32(c, key->encSize);
	result |= Trspi_HashUpdate(c, key->encSize, key->encData);

	return result;
}

TSS_RESULT
Trspi_Hash_UUID(Trspi_HashCtx *c, TSS_UUID uuid)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, uuid.ulTimeLow);
	result |= Trspi_Hash_UINT16(c, uuid.usTimeMid);
	result |= Trspi_Hash_UINT16(c, uuid.usTimeHigh);
	result |= Trspi_Hash_BYTE(c, uuid.bClockSeqHigh);
	result |= Trspi_Hash_BYTE(c, uuid.bClockSeqLow);
	result |= Trspi_HashUpdate(c, sizeof(uuid.rgbNode), uuid.rgbNode);

	return result;
}

TSS_RESULT
Trspi_Hash_PCR_EVENT(Trspi_HashCtx *c, TSS_PCR_EVENT *event)
{
	TSS_RESULT result;

	result = Trspi_Hash_VERSION(c, &event->versionInfo);
	result |= Trspi_Hash_UINT32(c, event->ulPcrIndex);
	result |= Trspi_Hash_UINT32(c, event->eventType);

	Trspi_Hash_UINT32(c, event->ulPcrValueLength);
	if (event->ulPcrValueLength > 0)
		result |= Trspi_HashUpdate(c, event->ulPcrValueLength, event->rgbPcrValue);

	result |= Trspi_Hash_UINT32(c, event->ulEventLength);
	if (event->ulEventLength > 0)
		result |= Trspi_HashUpdate(c, event->ulEventLength, event->rgbEvent);


	return result;
}

TSS_RESULT
Trspi_Hash_PRIVKEY_DIGEST12(Trspi_HashCtx *c, TPM_KEY12 *key)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, key->tag);
	result |= Trspi_Hash_UINT16(c, key->fill);
	result |= Trspi_Hash_UINT16(c, key->keyUsage);
	result |= Trspi_Hash_KEY_FLAGS(c, &key->keyFlags);
	result |= Trspi_Hash_BYTE(c, key->authDataUsage);
	result |= Trspi_Hash_KEY_PARMS(c, &key->algorithmParms);

	result |= Trspi_Hash_UINT32(c, key->PCRInfoSize);
	/* exclude pcrInfo when PCRInfoSize is 0 as spec'd in TPM 1.1b spec p.71 */
	if (key->PCRInfoSize != 0)
		result |= Trspi_HashUpdate(c, key->PCRInfoSize, key->PCRInfo);

	Trspi_Hash_STORE_PUBKEY(c, &key->pubKey);
	/* exclude encSize, encData as spec'd in TPM 1.1b spec p.71 */

	return result;
}

TSS_RESULT
Trspi_Hash_PRIVKEY_DIGEST(Trspi_HashCtx *c, TCPA_KEY *key)
{
	TSS_RESULT result;

	result = Trspi_Hash_VERSION(c, (TSS_VERSION *)&key->ver);
	result |= Trspi_Hash_UINT16(c, key->keyUsage);
	result |= Trspi_Hash_KEY_FLAGS(c, &key->keyFlags);
	result |= Trspi_Hash_BYTE(c, key->authDataUsage);
	result |= Trspi_Hash_KEY_PARMS(c, &key->algorithmParms);

	result |= Trspi_Hash_UINT32(c, key->PCRInfoSize);
	/* exclude pcrInfo when PCRInfoSize is 0 as spec'd in TPM 1.1b spec p.71 */
	if (key->PCRInfoSize != 0)
		result |= Trspi_HashUpdate(c, key->PCRInfoSize, key->PCRInfo);

	Trspi_Hash_STORE_PUBKEY(c, &key->pubKey);
	/* exclude encSize, encData as spec'd in TPM 1.1b spec p.71 */

	return result;
}

TSS_RESULT
Trspi_Hash_SYMMETRIC_KEY(Trspi_HashCtx *c, TCPA_SYMMETRIC_KEY *key)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, key->algId);
	result |= Trspi_Hash_UINT16(c, key->encScheme);
	result |= Trspi_Hash_UINT16(c, key->size);

	if (key->size > 0)
		result |= Trspi_HashUpdate(c, key->size, key->data);

	return result;
}

TSS_RESULT
Trspi_Hash_IDENTITY_REQ(Trspi_HashCtx *c, TCPA_IDENTITY_REQ *req)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, req->asymSize);
	result |= Trspi_Hash_UINT32(c, req->symSize);
	result |= Trspi_Hash_KEY_PARMS(c, &req->asymAlgorithm);
	result |= Trspi_Hash_KEY_PARMS(c, &req->symAlgorithm);
	result |= Trspi_HashUpdate(c, req->asymSize, req->asymBlob);
	result |= Trspi_HashUpdate(c, req->symSize, req->symBlob);

	return result;
}

TSS_RESULT
Trspi_Hash_CHANGEAUTH_VALIDATE(Trspi_HashCtx *c, TPM_CHANGEAUTH_VALIDATE *caValidate)
{
	TSS_RESULT result;

	result = Trspi_HashUpdate(c, TCPA_SHA1_160_HASH_LEN, caValidate->newAuthSecret.authdata);
	result |= Trspi_HashUpdate(c, TCPA_SHA1_160_HASH_LEN, caValidate->n1.nonce);

	return result;
}

TSS_RESULT
Trspi_Hash_SYM_CA_ATTESTATION(Trspi_HashCtx *c, TCPA_SYM_CA_ATTESTATION *sym)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, sym->credSize);
	result |= Trspi_Hash_KEY_PARMS(c, &sym->algorithm);
	result |= Trspi_HashUpdate(c, sym->credSize, sym->credential);

	return result;
}

TSS_RESULT
Trspi_Hash_ASYM_CA_CONTENTS(Trspi_HashCtx *c, TCPA_ASYM_CA_CONTENTS *asym)
{
	TSS_RESULT result;

	result = Trspi_Hash_SYMMETRIC_KEY(c, &asym->sessionKey);
	result |= Trspi_HashUpdate(c, TCPA_SHA1_160_HASH_LEN, (BYTE *)&asym->idDigest);

	return result;
}

TSS_RESULT
Trspi_Hash_BOUND_DATA(Trspi_HashCtx *c, TCPA_BOUND_DATA *bd, UINT32 payloadLength)
{
	TSS_RESULT result;

	result = Trspi_Hash_VERSION(c, (TSS_VERSION *)&bd->ver);
	result |= Trspi_Hash_BYTE(c, bd->payload);
	result |= Trspi_HashUpdate(c, payloadLength, bd->payloadData);

	return result;
}

TSS_RESULT
Trspi_Hash_TRANSPORT_AUTH(Trspi_HashCtx *c, TPM_TRANSPORT_AUTH *a)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, a->tag);
	result |= Trspi_HashUpdate(c, TPM_SHA1_160_HASH_LEN, a->authData.authdata);

	return result;
}

TSS_RESULT
Trspi_Hash_TRANSPORT_LOG_IN(Trspi_HashCtx *c, TPM_TRANSPORT_LOG_IN *l)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, l->tag);
	result |= Trspi_Hash_DIGEST(c, l->parameters.digest);
	result |= Trspi_Hash_DIGEST(c, l->pubKeyHash.digest);

	return result;
}

TSS_RESULT
Trspi_Hash_TRANSPORT_LOG_OUT(Trspi_HashCtx *c, TPM_TRANSPORT_LOG_OUT *l)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, l->tag);
	result |= Trspi_Hash_CURRENT_TICKS(c, &l->currentTicks);
	result |= Trspi_Hash_DIGEST(c, l->parameters.digest);
	result |= Trspi_Hash_UINT32(c, l->locality);

	return result;
}

TSS_RESULT
Trspi_Hash_CURRENT_TICKS(Trspi_HashCtx *c, TPM_CURRENT_TICKS *t)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, t->tag);
	result |= Trspi_Hash_UINT64(c, t->currentTicks);
	result |= Trspi_Hash_UINT16(c, t->tickRate);
	result |= Trspi_Hash_NONCE(c, t->tickNonce.nonce);

	return result;
}

TSS_RESULT
Trspi_Hash_SIGN_INFO(Trspi_HashCtx *c, TPM_SIGN_INFO *s)
{
	TSS_RESULT result;

	result = Trspi_Hash_UINT16(c, s->tag);
	result |= Trspi_HashUpdate(c, 4, s->fixed);
	result |= Trspi_Hash_NONCE(c, s->replay.nonce);
	result |= Trspi_Hash_UINT32(c, s->dataLen);
	result |= Trspi_HashUpdate(c, s->dataLen, s->data);

	return result;
}

void
Trspi_UnloadBlob_COUNTER_VALUE(UINT64 *offset, BYTE *blob, TPM_COUNTER_VALUE *ctr)
{
	if (!ctr) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		/* '4' is hard-coded in the spec */
		Trspi_UnloadBlob(offset, 4, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_UINT16(offset, &ctr->tag, blob);
	/* '4' is hard-coded in the spec */
	Trspi_UnloadBlob(offset, 4, blob, (BYTE *)&ctr->label);
	Trspi_UnloadBlob_UINT32(offset, &ctr->counter, blob);
}

void
Trspi_LoadBlob_COUNTER_VALUE(UINT64 *offset, BYTE *blob, TPM_COUNTER_VALUE *ctr)
{
	Trspi_LoadBlob_UINT16(offset, ctr->tag, blob);
	Trspi_LoadBlob(offset, 4, blob, (BYTE *)&ctr->label);
	Trspi_LoadBlob_UINT32(offset, ctr->counter, blob);
}

void
Trspi_UnloadBlob_CURRENT_TICKS(UINT64 *offset, BYTE *blob, TPM_CURRENT_TICKS *ticks)
{
	if (!ticks) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT64(offset, NULL, blob);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob(offset, sizeof(TPM_NONCE), blob, NULL);

		return;
	}

	Trspi_UnloadBlob_UINT16(offset, &ticks->tag, blob);
	Trspi_UnloadBlob_UINT64(offset, &ticks->currentTicks, blob);
	Trspi_UnloadBlob_UINT16(offset, &ticks->tickRate, blob);
	Trspi_UnloadBlob(offset, sizeof(TPM_NONCE), blob, (BYTE *)&ticks->tickNonce);
}

void
Trspi_UnloadBlob_TRANSPORT_PUBLIC(UINT64 *offset, BYTE *blob, TPM_TRANSPORT_PUBLIC *t)
{
	Trspi_UnloadBlob_UINT16(offset, &t->tag, blob);
	Trspi_UnloadBlob_UINT32(offset, &t->transAttributes, blob);
	Trspi_UnloadBlob_UINT32(offset, &t->algId, blob);
	Trspi_UnloadBlob_UINT16(offset, &t->encScheme, blob);
}

void
Trspi_LoadBlob_TRANSPORT_PUBLIC(UINT64 *offset, BYTE *blob, TPM_TRANSPORT_PUBLIC *t)
{
	Trspi_LoadBlob_UINT16(offset, t->tag, blob);
	Trspi_LoadBlob_UINT32(offset, t->transAttributes, blob);
	Trspi_LoadBlob_UINT32(offset, t->algId, blob);
	Trspi_LoadBlob_UINT16(offset, t->encScheme, blob);
}

void
Trspi_LoadBlob_TRANSPORT_AUTH(UINT64 *offset, BYTE *blob, TPM_TRANSPORT_AUTH *t)
{
	Trspi_LoadBlob_UINT16(offset, t->tag, blob);
	Trspi_LoadBlob(offset, TPM_SHA1_160_HASH_LEN, blob, t->authData.authdata);
}

void
Trspi_LoadBlob_SIGN_INFO(UINT64 *offset, BYTE *blob, TPM_SIGN_INFO *s)
{
	Trspi_LoadBlob_UINT16(offset, s->tag, blob);
	Trspi_LoadBlob(offset, 4, blob, s->fixed);
	Trspi_LoadBlob(offset, TPM_SHA1_160_HASH_LEN, blob, s->replay.nonce);
	Trspi_LoadBlob_UINT32(offset, s->dataLen, blob);
	Trspi_LoadBlob(offset, s->dataLen, blob, s->data);
}

TSS_RESULT
Trspi_UnloadBlob_CERTIFY_INFO(UINT64 *offset, BYTE *blob, TPM_CERTIFY_INFO *c)
{
	TSS_RESULT result;

	if (!c) {
		UINT32 pcrInfoSize;

		Trspi_UnloadBlob_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_KEY_PARMS(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);
		Trspi_UnloadBlob_NONCE(offset, blob, NULL);
		Trspi_UnloadBlob_BOOL(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, &pcrInfoSize, blob);

		(*offset) += pcrInfoSize;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_VERSION(offset, blob, &c->version);
	Trspi_UnloadBlob_UINT16(offset, &c->keyUsage, blob);
	Trspi_UnloadBlob_UINT32(offset, &c->keyFlags, blob);
	Trspi_UnloadBlob_BYTE(offset, &c->authDataUsage, blob);
	if ((result = Trspi_UnloadBlob_KEY_PARMS(offset, blob, &c->algorithmParms)))
		return result;
	Trspi_UnloadBlob_DIGEST(offset, blob, &c->pubkeyDigest);
	Trspi_UnloadBlob_NONCE(offset, blob, &c->data);
	Trspi_UnloadBlob_BOOL(offset, (TSS_BOOL *)&c->parentPCRStatus, blob);
	Trspi_UnloadBlob_UINT32(offset, &c->PCRInfoSize, blob);
        if (c->PCRInfoSize != 0) {
                c->PCRInfo = malloc(sizeof(TPM_PCR_INFO));
                if (c->PCRInfo == NULL) {
                        LogError("malloc of %lu bytes failed.", sizeof(TPM_PCR_INFO));
                        return TSPERR(TSS_E_OUTOFMEMORY);
                }
        } else {
                c->PCRInfo = NULL;
        }
        Trspi_UnloadBlob_PCR_INFO(offset, blob, (TPM_PCR_INFO *)c->PCRInfo);

	return TSS_SUCCESS;
}

void
Trspi_UnloadBlob_TPM_FAMILY_LABEL(UINT64 *offset, BYTE *blob, TPM_FAMILY_LABEL *label)
{
	if (!label) {
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_BYTE(offset, &label->label, blob);
}

void
Trspi_LoadBlob_TPM_FAMILY_LABEL(UINT64 *offset, BYTE *blob, TPM_FAMILY_LABEL *label)
{
	Trspi_LoadBlob_BYTE(offset, label->label, blob);
}

void
Trspi_UnloadBlob_TPM_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TPM_FAMILY_TABLE_ENTRY *entry)
{
	if (!entry) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_TPM_FAMILY_LABEL(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_UINT16(offset, &entry->tag, blob);
	Trspi_UnloadBlob_TPM_FAMILY_LABEL(offset, blob, &entry->label);
	Trspi_UnloadBlob_UINT32(offset, &entry->familyID, blob);
	Trspi_UnloadBlob_UINT32(offset, &entry->verificationCount, blob);
	Trspi_UnloadBlob_UINT32(offset, &entry->flags, blob);
}

void
Trspi_LoadBlob_TPM_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TPM_FAMILY_TABLE_ENTRY *entry)
{
	Trspi_LoadBlob_UINT16(offset, entry->tag, blob);
	Trspi_LoadBlob_TPM_FAMILY_LABEL(offset, blob, &entry->label);
	Trspi_LoadBlob_UINT32(offset, entry->familyID, blob);
	Trspi_LoadBlob_UINT32(offset, entry->verificationCount, blob);
	Trspi_LoadBlob_UINT32(offset, entry->flags, blob);
}

void
Trspi_UnloadBlob_TPM_DELEGATE_LABEL(UINT64 *offset, BYTE *blob, TPM_DELEGATE_LABEL *label)
{
	if (!label) {
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_BYTE(offset, &label->label, blob);
}

void
Trspi_LoadBlob_TPM_DELEGATE_LABEL(UINT64 *offset, BYTE *blob, TPM_DELEGATE_LABEL *label)
{
	Trspi_LoadBlob_BYTE(offset, label->label, blob);
}

void
Trspi_UnloadBlob_TPM_DELEGATIONS(UINT64 *offset, BYTE *blob, TPM_DELEGATIONS *delegations)
{
	if (!delegations) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_UINT16(offset, &delegations->tag, blob);
	Trspi_UnloadBlob_UINT32(offset, &delegations->delegateType, blob);
	Trspi_UnloadBlob_UINT32(offset, &delegations->per1, blob);
	Trspi_UnloadBlob_UINT32(offset, &delegations->per2, blob);
}

void
Trspi_LoadBlob_TPM_DELEGATIONS(UINT64 *offset, BYTE *blob, TPM_DELEGATIONS *delegations)
{
	Trspi_LoadBlob_UINT16(offset, delegations->tag, blob);
	Trspi_LoadBlob_UINT32(offset, delegations->delegateType, blob);
	Trspi_LoadBlob_UINT32(offset, delegations->per1, blob);
	Trspi_LoadBlob_UINT32(offset, delegations->per2, blob);
}

TSS_RESULT
Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(UINT64 *offset, BYTE *blob, TPM_DELEGATE_PUBLIC *pub)
{
	TSS_RESULT result;

	if (!pub) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_TPM_DELEGATE_LABEL(offset, blob, NULL);
		(void)Trspi_UnloadBlob_PCR_INFO_SHORT(offset, blob, NULL);
		Trspi_UnloadBlob_TPM_DELEGATIONS(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &pub->tag, blob);
	Trspi_UnloadBlob_TPM_DELEGATE_LABEL(offset, blob, &pub->label);
	if ((result = Trspi_UnloadBlob_PCR_INFO_SHORT(offset, blob, &pub->pcrInfo)))
		return result;
	Trspi_UnloadBlob_TPM_DELEGATIONS(offset, blob, &pub->permissions);
	Trspi_UnloadBlob_UINT32(offset, &pub->familyID, blob);
	Trspi_UnloadBlob_UINT32(offset, &pub->verificationCount, blob);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_TPM_DELEGATE_PUBLIC(UINT64 *offset, BYTE *blob, TPM_DELEGATE_PUBLIC *pub)
{
	Trspi_LoadBlob_UINT16(offset, pub->tag, blob);
	Trspi_LoadBlob_TPM_DELEGATE_LABEL(offset, blob, &pub->label);
	Trspi_LoadBlob_PCR_INFO_SHORT(offset, blob, &pub->pcrInfo);
	Trspi_LoadBlob_TPM_DELEGATIONS(offset, blob, &pub->permissions);
	Trspi_LoadBlob_UINT32(offset, pub->familyID, blob);
	Trspi_LoadBlob_UINT32(offset, pub->verificationCount, blob);
}

TSS_RESULT
Trspi_UnloadBlob_TPM_DELEGATE_OWNER_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_OWNER_BLOB *owner)
{
	TSS_RESULT result;

	if (!owner) {
		UINT32 additionalSize, sensitiveSize;

		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		(void)Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &additionalSize, blob);
		(void)Trspi_UnloadBlob(offset, additionalSize, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &sensitiveSize, blob);
		(void)Trspi_UnloadBlob(offset, sensitiveSize, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &owner->tag, blob);
	if ((result = Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(offset, blob, &owner->pub)))
		return result;
	Trspi_UnloadBlob_DIGEST(offset, blob, &owner->integrityDigest);
	Trspi_UnloadBlob_UINT32(offset, &owner->additionalSize, blob);
	if (owner->additionalSize > 0) {
		owner->additionalArea = malloc(owner->additionalSize);
		if (owner->additionalArea == NULL) {
			LogError("malloc of %u bytes failed.", owner->additionalSize);
			free(owner->pub.pcrInfo.pcrSelection.pcrSelect);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, owner->additionalSize, blob, owner->additionalArea);
	}
	Trspi_UnloadBlob_UINT32(offset, &owner->sensitiveSize, blob);
	if (owner->sensitiveSize > 0) {
		owner->sensitiveArea = malloc(owner->sensitiveSize);
		if (owner->sensitiveArea == NULL) {
			LogError("malloc of %u bytes failed.", owner->sensitiveSize);
			free(owner->pub.pcrInfo.pcrSelection.pcrSelect);
			free(owner->additionalArea);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, owner->sensitiveSize, blob, owner->sensitiveArea);
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_TPM_DELEGATE_OWNER_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_OWNER_BLOB *owner)
{
	Trspi_LoadBlob_UINT16(offset, owner->tag, blob);
	Trspi_LoadBlob_TPM_DELEGATE_PUBLIC(offset, blob, &owner->pub);
	Trspi_LoadBlob_DIGEST(offset, blob, &owner->integrityDigest);
	Trspi_LoadBlob_UINT32(offset, owner->additionalSize, blob);
	Trspi_LoadBlob(offset, owner->additionalSize, blob, owner->additionalArea);
	Trspi_LoadBlob_UINT32(offset, owner->sensitiveSize, blob);
	Trspi_LoadBlob(offset, owner->sensitiveSize, blob, owner->sensitiveArea);
}

TSS_RESULT
Trspi_UnloadBlob_TPM_DELEGATE_KEY_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_KEY_BLOB *key)
{
	TSS_RESULT result;

	if (!key) {
		UINT32 additionalSize, sensitiveSize;

		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		(void)Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &additionalSize, blob);
		(void)Trspi_UnloadBlob(offset, additionalSize, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &sensitiveSize, blob);
		(void)Trspi_UnloadBlob(offset, sensitiveSize, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &key->tag, blob);
	if ((result = Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(offset, blob, &key->pub)))
		return result;
	Trspi_UnloadBlob_DIGEST(offset, blob, &key->integrityDigest);
	Trspi_UnloadBlob_DIGEST(offset, blob, &key->pubKeyDigest);
	Trspi_UnloadBlob_UINT32(offset, &key->additionalSize, blob);
	if (key->additionalSize > 0) {
		key->additionalArea = malloc(key->additionalSize);
		if (key->additionalArea == NULL) {
			LogError("malloc of %u bytes failed.", key->additionalSize);
			free(key->pub.pcrInfo.pcrSelection.pcrSelect);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->additionalSize, blob, key->additionalArea);
	}
	Trspi_UnloadBlob_UINT32(offset, &key->sensitiveSize, blob);
	if (key->sensitiveSize > 0) {
		key->sensitiveArea = malloc(key->sensitiveSize);
		if (key->sensitiveArea == NULL) {
			LogError("malloc of %u bytes failed.", key->sensitiveSize);
			free(key->pub.pcrInfo.pcrSelection.pcrSelect);
			free(key->additionalArea);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, key->sensitiveSize, blob, key->sensitiveArea);
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_TPM_DELEGATE_KEY_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_KEY_BLOB *key)
{
	Trspi_LoadBlob_UINT16(offset, key->tag, blob);
	Trspi_LoadBlob_TPM_DELEGATE_PUBLIC(offset, blob, &key->pub);
	Trspi_LoadBlob_DIGEST(offset, blob, &key->integrityDigest);
	Trspi_LoadBlob_DIGEST(offset, blob, &key->pubKeyDigest);
	Trspi_LoadBlob_UINT32(offset, key->additionalSize, blob);
	Trspi_LoadBlob(offset, key->additionalSize, blob, key->additionalArea);
	Trspi_LoadBlob_UINT32(offset, key->sensitiveSize, blob);
	Trspi_LoadBlob(offset, key->sensitiveSize, blob, key->sensitiveArea);
}

void
Trspi_UnloadBlob_TSS_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TSS_FAMILY_TABLE_ENTRY *entry)
{
	if (!entry) {
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_BOOL(offset, NULL, blob);
		Trspi_UnloadBlob_BOOL(offset, NULL, blob);

		return;
	}

	Trspi_UnloadBlob_UINT32(offset, &entry->familyID, blob);
	Trspi_UnloadBlob_BYTE(offset, &entry->label, blob);
	Trspi_UnloadBlob_UINT32(offset, &entry->verificationCount, blob);
	Trspi_UnloadBlob_BOOL(offset, &entry->enabled, blob);
	Trspi_UnloadBlob_BOOL(offset, &entry->locked, blob);
}

void
Trspi_LoadBlob_TSS_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TSS_FAMILY_TABLE_ENTRY *entry)
{
	Trspi_LoadBlob_UINT32(offset, entry->familyID, blob);
	Trspi_LoadBlob_BYTE(offset, entry->label, blob);
	Trspi_LoadBlob_UINT32(offset, entry->verificationCount, blob);
	Trspi_LoadBlob_BOOL(offset, entry->enabled, blob);
	Trspi_LoadBlob_BOOL(offset, entry->locked, blob);
}

TSS_RESULT
Trspi_UnloadBlob_TSS_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TSS_PCR_INFO_SHORT *pcr)
{
	if (!pcr) {
		UINT32 sizeOfSelect, sizeOfDigestAtRelease;

		Trspi_UnloadBlob_UINT32(offset, &sizeOfSelect, blob);
		(void)Trspi_UnloadBlob(offset, sizeOfSelect, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, &sizeOfDigestAtRelease, blob);
		(void)Trspi_UnloadBlob(offset, sizeOfDigestAtRelease, blob, NULL);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &pcr->sizeOfSelect, blob);
	if (pcr->sizeOfSelect > 0) {
		pcr->selection = malloc(pcr->sizeOfSelect);
		if (pcr->selection == NULL) {
			LogError("malloc of %u bytes failed.", pcr->sizeOfSelect);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, pcr->sizeOfSelect, blob, pcr->selection);
	} else {
		pcr->selection = NULL;
	}
	Trspi_UnloadBlob_BYTE(offset, &pcr->localityAtRelease, blob);
	Trspi_UnloadBlob_UINT32(offset, &pcr->sizeOfDigestAtRelease, blob);
	if (pcr->sizeOfDigestAtRelease > 0) {
		pcr->digestAtRelease = malloc(pcr->sizeOfDigestAtRelease);
		if (pcr->digestAtRelease == NULL) {
			LogError("malloc of %u bytes failed.", pcr->sizeOfDigestAtRelease);
			free(pcr->selection);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}
		Trspi_UnloadBlob(offset, pcr->sizeOfDigestAtRelease, blob, pcr->digestAtRelease);
	} else {
		pcr->digestAtRelease = NULL;
	}

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_TSS_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TSS_PCR_INFO_SHORT *pcr)
{
	Trspi_LoadBlob_UINT32(offset, pcr->sizeOfSelect, blob);
	Trspi_LoadBlob(offset, pcr->sizeOfSelect, blob, pcr->selection);
	Trspi_LoadBlob_BYTE(offset, pcr->localityAtRelease, blob);
	Trspi_LoadBlob_UINT32(offset, pcr->sizeOfDigestAtRelease, blob);
	Trspi_LoadBlob(offset, pcr->sizeOfDigestAtRelease, blob, pcr->digestAtRelease);
}

TSS_RESULT
Trspi_UnloadBlob_TSS_DELEGATION_TABLE_ENTRY(UINT64 *offset, BYTE *blob,
					    TSS_DELEGATION_TABLE_ENTRY *entry)
{
	TSS_RESULT result;

	if (!entry) {
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		(void)Trspi_UnloadBlob_TSS_PCR_INFO_SHORT(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, &entry->tableIndex, blob);
	Trspi_UnloadBlob_BYTE(offset, &entry->label, blob);
	if ((result = Trspi_UnloadBlob_TSS_PCR_INFO_SHORT(offset, blob, &entry->pcrInfo)))
		return result;
	Trspi_UnloadBlob_UINT32(offset, &entry->per1, blob);
	Trspi_UnloadBlob_UINT32(offset, &entry->per2, blob);
	Trspi_UnloadBlob_UINT32(offset, &entry->familyID, blob);
	Trspi_UnloadBlob_UINT32(offset, &entry->verificationCount, blob);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_TSS_DELEGATION_TABLE_ENTRY(UINT64 *offset, BYTE *blob,
					  TSS_DELEGATION_TABLE_ENTRY *entry)
{
	Trspi_LoadBlob_UINT32(offset, entry->tableIndex, blob);
	Trspi_LoadBlob_BYTE(offset, entry->label, blob);
	Trspi_LoadBlob_TSS_PCR_INFO_SHORT(offset, blob, &entry->pcrInfo);
	Trspi_LoadBlob_UINT32(offset, entry->per1, blob);
	Trspi_LoadBlob_UINT32(offset, entry->per2, blob);
	Trspi_LoadBlob_UINT32(offset, entry->familyID, blob);
	Trspi_LoadBlob_UINT32(offset, entry->verificationCount, blob);
}

TSS_RESULT
Trspi_UnloadBlob_PCR_COMPOSITE(UINT64 *offset, BYTE *blob, TCPA_PCR_COMPOSITE *out)
{
	TSS_RESULT result;

	if (!out) {
		UINT32 valueSize;

		Trspi_UnloadBlob_PCR_SELECTION(offset, blob, NULL);
		Trspi_UnloadBlob_UINT32(offset, &valueSize, blob);
		Trspi_UnloadBlob(offset, valueSize, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = Trspi_UnloadBlob_PCR_SELECTION(offset, blob, &out->select)))
		return result;

	Trspi_UnloadBlob_UINT32(offset, &out->valueSize, blob);
	out->pcrValue = malloc(out->valueSize);
	if (out->pcrValue == NULL) {
		LogError("malloc of %u bytes failed.", out->valueSize);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(offset, out->valueSize, blob, (BYTE *)out->pcrValue);

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_MIGRATIONKEYAUTH(UINT64 *offset, BYTE *blob, TPM_MIGRATIONKEYAUTH *migAuth)
{
	TSS_RESULT result;

	if (!migAuth) {
		(void)Trspi_UnloadBlob_PUBKEY(offset, blob, NULL);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_DIGEST(offset, blob, NULL);

		return TSS_SUCCESS;
	}

	if ((result = Trspi_UnloadBlob_PUBKEY(offset, blob, &migAuth->migrationKey)))
		return result;

	Trspi_UnloadBlob_UINT16(offset, &migAuth->migrationScheme, blob);
	Trspi_UnloadBlob_DIGEST(offset, blob, &migAuth->digest);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_MIGRATIONKEYAUTH(UINT64 *offset, BYTE *blob, TPM_MIGRATIONKEYAUTH *migAuth)
{
	Trspi_LoadBlob_PUBKEY(offset, blob, &migAuth->migrationKey);
	Trspi_LoadBlob_UINT16(offset, migAuth->migrationScheme, blob);
	Trspi_LoadBlob_DIGEST(offset, blob, &migAuth->digest);
}

void
Trspi_LoadBlob_MSA_COMPOSITE(UINT64 *offset, BYTE *blob, TPM_MSA_COMPOSITE *msaComp)
{
	UINT32 i;

	Trspi_LoadBlob_UINT32(offset, msaComp->MSAlist, blob);
	for (i = 0; i < msaComp->MSAlist; i++)
		Trspi_LoadBlob_DIGEST(offset, blob, &msaComp->migAuthDigest[i]);
}

void
Trspi_LoadBlob_CMK_AUTH(UINT64 *offset, BYTE *blob, TPM_CMK_AUTH *cmkAuth)
{
	Trspi_LoadBlob_DIGEST(offset, blob, &cmkAuth->migrationAuthorityDigest);
	Trspi_LoadBlob_DIGEST(offset, blob, &cmkAuth->destinationKeyDigest);
	Trspi_LoadBlob_DIGEST(offset, blob, &cmkAuth->sourceKeyDigest);
}

TSS_RESULT
Trspi_Hash_MSA_COMPOSITE(Trspi_HashCtx *c, TPM_MSA_COMPOSITE *m)
{
	UINT32 i;
	TPM_DIGEST *digest;
	TSS_RESULT result;

	result = Trspi_Hash_UINT32(c, m->MSAlist);
	digest = m->migAuthDigest;
	for (i = 0; i < m->MSAlist; i++) {
		result |= Trspi_Hash_DIGEST(c, digest->digest);
		digest++;
	}

	return result;
}

TSS_RESULT
Trspi_UnloadBlob_TSS_PLATFORM_CLASS(UINT64 *offset, BYTE *blob, TSS_PLATFORM_CLASS *platClass)
{
	if (!platClass){
		UINT32 classURISize;

		Trspi_UnloadBlob_UINT32(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, &classURISize, blob);
		Trspi_UnloadBlob(offset, classURISize, blob, NULL);

		return TSS_SUCCESS;
	}
	Trspi_UnloadBlob_UINT32(offset, &platClass->platformClassSimpleIdentifier, blob);
	Trspi_UnloadBlob_UINT32(offset, &platClass->platformClassURISize, blob);

	platClass->pPlatformClassURI = malloc(platClass->platformClassURISize);
	if (platClass->pPlatformClassURI == NULL) {
		LogError("malloc of %u bytes failed.", platClass->platformClassURISize);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}
	Trspi_UnloadBlob(offset, platClass->platformClassURISize, blob,
			 (BYTE *)platClass->pPlatformClassURI);

	return TSS_SUCCESS;
}

void
Trspi_LoadBlob_CAP_VERSION_INFO(UINT64 *offset, BYTE *blob, TPM_CAP_VERSION_INFO *v)
{
	Trspi_LoadBlob_UINT16(offset, v->tag, blob);
	Trspi_LoadBlob_TCPA_VERSION(offset, blob, *(TCPA_VERSION *)(&v->version));
	Trspi_LoadBlob_UINT16(offset, v->specLevel, blob);
	Trspi_LoadBlob_BYTE(offset, v->errataRev, blob);
	Trspi_LoadBlob(offset, sizeof(v->tpmVendorID), blob, v->tpmVendorID);
	Trspi_LoadBlob_UINT16(offset, v->vendorSpecificSize, blob);
	Trspi_LoadBlob(offset, v->vendorSpecificSize, blob, v->vendorSpecific);
}

TSS_RESULT
Trspi_UnloadBlob_CAP_VERSION_INFO(UINT64 *offset, BYTE *blob, TPM_CAP_VERSION_INFO *v)
{
	if (!v) {
		UINT16 vendorSpecificSize;

		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_VERSION(offset, blob, NULL);
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob(offset, 4, blob, NULL);
		Trspi_UnloadBlob_UINT16(offset, &vendorSpecificSize, blob);

		(*offset) += vendorSpecificSize;

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &v->tag, blob);
	Trspi_UnloadBlob_VERSION(offset, blob, (TCPA_VERSION *)&v->version);
	Trspi_UnloadBlob_UINT16(offset, &v->specLevel, blob);
	Trspi_UnloadBlob_BYTE(offset, &v->errataRev, blob);
	Trspi_UnloadBlob(offset, sizeof(v->tpmVendorID), blob, v->tpmVendorID);
	Trspi_UnloadBlob_UINT16(offset, &v->vendorSpecificSize, blob);

	if (v->vendorSpecificSize > 0) {
		if ((v->vendorSpecific = malloc(v->vendorSpecificSize)) == NULL) {
			LogError("malloc of %u bytes failed.", v->vendorSpecificSize);
			return TSPERR(TSS_E_OUTOFMEMORY);
		}

		Trspi_UnloadBlob(offset, v->vendorSpecificSize, blob, v->vendorSpecific);
	} else {
		v->vendorSpecific = NULL;
	}

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_NV_INDEX(UINT64 *offset, BYTE *blob, TPM_NV_INDEX *v)
{
	if (!v) {
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT32(offset, v, blob);

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_NV_ATTRIBUTES(UINT64 *offset, BYTE *blob, TPM_NV_ATTRIBUTES *v)
{
	if (!v) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &v->tag, blob);
	Trspi_UnloadBlob_UINT32(offset, &v->attributes, blob);

	return TSS_SUCCESS;
}

TSS_RESULT
Trspi_UnloadBlob_NV_DATA_PUBLIC(UINT64 *offset, BYTE *blob, TPM_NV_DATA_PUBLIC *v)
{
	if (!v) {
		Trspi_UnloadBlob_UINT16(offset, NULL, blob);
		Trspi_UnloadBlob_NV_INDEX(offset, blob, NULL);
		Trspi_UnloadBlob_PCR_INFO_SHORT(offset, blob, NULL);
		Trspi_UnloadBlob_PCR_INFO_SHORT(offset, blob, NULL);
		Trspi_UnloadBlob_NV_ATTRIBUTES(offset, blob, NULL);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_BYTE(offset, NULL, blob);
		Trspi_UnloadBlob_UINT32(offset, NULL, blob);

		return TSS_SUCCESS;
	}

	Trspi_UnloadBlob_UINT16(offset, &v->tag, blob);
	Trspi_UnloadBlob_NV_INDEX(offset, blob, &v->nvIndex);
	Trspi_UnloadBlob_PCR_INFO_SHORT(offset, blob, &v->pcrInfoRead);
	Trspi_UnloadBlob_PCR_INFO_SHORT(offset, blob, &v->pcrInfoWrite);
	Trspi_UnloadBlob_NV_ATTRIBUTES(offset, blob, &v->permission);
	Trspi_UnloadBlob_BYTE(offset, &v->bReadSTClear, blob);
	Trspi_UnloadBlob_BYTE(offset, &v->bWriteSTClear, blob);
	Trspi_UnloadBlob_BYTE(offset, &v->bWriteDefine, blob);
	Trspi_UnloadBlob_UINT32(offset, &v->dataSize, blob);

	return TSS_SUCCESS;
}
