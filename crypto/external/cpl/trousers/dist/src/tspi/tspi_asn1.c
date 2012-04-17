
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <openssl/asn1.h>
#include <openssl/asn1t.h>

#ifndef TSS_BUILD_ASN1_OPENSSL
#include <arpa/inet.h>
#endif

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "tsplog.h"

#define TSS_OPENSSL_ASN1_ERROR	(0xffffffff)

#if (OPENSSL_VERSION_NUMBER >= 0x0090800FL)
#define OPENSSL_COMPAT_CONST const
#else
#define OPENSSL_COMPAT_CONST
#endif

#define OPENSSL_COMPAT_ASN1_SEQUENCE(tname) \
	static const ASN1_TEMPLATE tname##_seq_tt[] 

typedef struct tdTSS_BLOB {
	ASN1_INTEGER *		structVersion;
	ASN1_INTEGER *		blobType;
	ASN1_INTEGER *		blobLength;
	ASN1_OCTET_STRING *	blob;
} TSS_BLOB;

OPENSSL_COMPAT_ASN1_SEQUENCE(TSS_BLOB) = {
	ASN1_SIMPLE(TSS_BLOB, structVersion, ASN1_INTEGER),
	ASN1_SIMPLE(TSS_BLOB, blobType, ASN1_INTEGER),
	ASN1_SIMPLE(TSS_BLOB, blobLength, ASN1_INTEGER),
	ASN1_SIMPLE(TSS_BLOB, blob, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(TSS_BLOB)
IMPLEMENT_ASN1_FUNCTIONS(TSS_BLOB)


TSS_RESULT
Tspi_EncodeDER_TssBlob(UINT32 rawBlobSize,		/* in */
			BYTE *rawBlob,			/* in */
			UINT32 blobType,		/* in */
			UINT32 *derBlobSize,		/* in/out */
			BYTE *derBlob)			/* out */
{
#ifdef TSS_BUILD_ASN1_OPENSSL
	TSS_BLOB *tssBlob = NULL;
#endif
	BYTE *encBlob = NULL;
	UINT32 encBlobLen;

	if ((rawBlobSize == 0) || (rawBlob == NULL))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((blobType < TSS_BLOB_TYPE_KEY) || (blobType > TSS_BLOB_TYPE_CMK_BYTE_STREAM))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((*derBlobSize != 0) && (derBlob == NULL))
		return TSPERR(TSS_E_BAD_PARAMETER);

	/* The TSS working group has stated that the ASN1 encoding will be done in a
	 * specific way that generates an ASN1 encoding that is exactly 20 bytes
	 * larger than the blob being encoded.
	 *
	 * OpenSSL uses the smallest number of bytes possible to encode and object
	 * and as a result cannot be used to perform the encoding.  The encoding
	 * must be done manually.
	 *
	 * The 20 byte fixed header will result in issues for objects greater than
	 * 2^16 in size since some fields are now limited to 16-bit lengths.
	 */

#ifdef TSS_BUILD_ASN1_OPENSSL
	tssBlob = TSS_BLOB_new();
	if (!tssBlob)
		return TSPERR(TSS_E_OUTOFMEMORY);

	if (ASN1_INTEGER_set(tssBlob->structVersion, TSS_BLOB_STRUCT_VERSION) == 0) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if (ASN1_INTEGER_set(tssBlob->blobType, blobType) == 0) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if (ASN1_INTEGER_set(tssBlob->blobLength, rawBlobSize) == 0) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	if (ASN1_OCTET_STRING_set(tssBlob->blob, rawBlob, rawBlobSize) == 0) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_OUTOFMEMORY);
	}

	encBlobLen = i2d_TSS_BLOB(tssBlob, &encBlob);
	if (encBlobLen <= 0) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (*derBlobSize != 0) {
		if (encBlobLen <= *derBlobSize) {
			memcpy(derBlob, encBlob, encBlobLen);
		}
		else {
			OPENSSL_free(encBlob);
			TSS_BLOB_free(tssBlob);
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
	}

	*derBlobSize = encBlobLen;

	OPENSSL_free(encBlob);
	TSS_BLOB_free(tssBlob);
#else
	if ((rawBlobSize + 16) > UINT16_MAX)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	encBlobLen = rawBlobSize + 20;

	if (*derBlobSize != 0) {
		if (encBlobLen <= *derBlobSize) {
			UINT16 *pShort;
			UINT32 *pLong;

			encBlob = derBlob;
			encBlob[0] = 0x30;	/* Sequence tag */
			encBlob[1] = 0x82;	/* Length in the two octets that follow */
			encBlob += 2;
			pShort = (UINT16 *)encBlob;
			*pShort = htons(rawBlobSize + 16);
			encBlob += sizeof(UINT16);

			encBlob[0] = 0x02;	/* Integer tag */
			encBlob[1] = 0x01;	/* Length is one */
			encBlob[2] = (BYTE)TSS_BLOB_STRUCT_VERSION;
			encBlob += 3;

			encBlob[0] = 0x02;	/* Integer tag */
			encBlob[1] = 0x01;	/* Length is one */
			encBlob[2] = (BYTE)blobType;
			encBlob += 3;

			encBlob[0] = 0x02;	/* Integer tag */
			encBlob[1] = 0x04;	/* Length is four */
			encBlob += 2;
			pLong = (UINT32 *)encBlob;
			*pLong = htonl(rawBlobSize);
			encBlob += sizeof(UINT32);

			encBlob[0] = 0x04;	/* Octet string tag */
			encBlob[1] = 0x82;	/* Length in the two octets that follow */
			encBlob += 2;
			pShort = (UINT16 *)encBlob;
			*pShort = htons(rawBlobSize);
			encBlob += sizeof(UINT16);
			memcpy(encBlob, rawBlob, rawBlobSize);
		}
		else
			return TSPERR(TSS_E_BAD_PARAMETER);
	}

	*derBlobSize = encBlobLen;
#endif

	return TSS_SUCCESS;
}

TSS_RESULT
Tspi_DecodeBER_TssBlob(UINT32 berBlobSize,		/* in */
			BYTE *berBlob,			/* in */
			UINT32 *blobType,		/* out */
			UINT32 *rawBlobSize,		/* in/out */
			BYTE *rawBlob)			/* out */
{
	TSS_BLOB *tssBlob = NULL;
	OPENSSL_COMPAT_CONST BYTE *encBlob = berBlob;

	UINT32 encBlobLen = berBlobSize;
	UINT32 decStructVersion, decBlobType, decBlobSize;

	if ((berBlobSize == 0) || (berBlob == NULL))
		return TSPERR(TSS_E_BAD_PARAMETER);

	if ((*rawBlobSize != 0) && (rawBlob == NULL))
		return TSPERR(TSS_E_BAD_PARAMETER);

	tssBlob = d2i_TSS_BLOB(NULL, &encBlob, encBlobLen);
	if (!tssBlob)
		return TSPERR(TSS_E_INTERNAL_ERROR);

	decStructVersion = ASN1_INTEGER_get(tssBlob->structVersion);
	if (decStructVersion == TSS_OPENSSL_ASN1_ERROR) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if (decStructVersion > TSS_BLOB_STRUCT_VERSION) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_BAD_PARAMETER);
	}

	decBlobType = ASN1_INTEGER_get(tssBlob->blobType);
	if (decBlobType == TSS_OPENSSL_ASN1_ERROR) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}
	if ((decBlobType < TSS_BLOB_TYPE_KEY) || (decBlobType > TSS_BLOB_TYPE_CMK_BYTE_STREAM)) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_BAD_PARAMETER);
	}

	decBlobSize = ASN1_INTEGER_get(tssBlob->blobLength);
	if (decBlobSize == TSS_OPENSSL_ASN1_ERROR) {
		TSS_BLOB_free(tssBlob);
		return TSPERR(TSS_E_INTERNAL_ERROR);
	}

	if (*rawBlobSize != 0) {
		if (decBlobSize <= *rawBlobSize) {
			memcpy(rawBlob, tssBlob->blob->data, decBlobSize);
		}
		else {
			TSS_BLOB_free(tssBlob);
			return TSPERR(TSS_E_BAD_PARAMETER);
		}
	}

	*rawBlobSize = decBlobSize;
	*blobType = decBlobType;

	TSS_BLOB_free(tssBlob);

	return TSS_SUCCESS;
}

