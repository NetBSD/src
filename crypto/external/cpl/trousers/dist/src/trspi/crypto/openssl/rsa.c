
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

/*
 * rsa.c - openssl TSS crypto routines
 *
 * Kent Yoder <shpedoikal@gmail.com>
 *
 */

#include <string.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>

#include "trousers/tss.h"
#include "trousers/trousers.h"
#include "trousers_types.h"
#include "spi_utils.h"
#include "tsplog.h"

#ifdef TSS_DEBUG
#define DEBUG_print_openssl_errors() \
	do { \
		ERR_load_crypto_strings(); \
		ERR_print_errors_fp(stderr); \
	} while (0)
#else
#define DEBUG_print_openssl_errors()
#endif


/*
 * Hopefully this will make the code clearer since
 * OpenSSL returns 1 on success
 */
#define EVP_SUCCESS 1

/* XXX int set to unsigned int values */
int
Trspi_RSA_Encrypt(unsigned char *dataToEncrypt, /* in */
		unsigned int dataToEncryptLen,  /* in */
		unsigned char *encryptedData,   /* out */
		unsigned int *encryptedDataLen, /* out */
		unsigned char *publicKey,
		unsigned int keysize)
{
	int rv;
	unsigned char exp[] = { 0x01, 0x00, 0x01 }; /* 65537 hex */
	unsigned char oaepPad[] = "TCPA";
	int oaepPadLen = 4;
	RSA *rsa = RSA_new();
	BYTE encodedData[256];
	int encodedDataLen;

	if (rsa == NULL) {
		rv = TSPERR(TSS_E_OUTOFMEMORY);
		goto err;
	}

	/* set the public key value in the OpenSSL object */
	rsa->n = BN_bin2bn(publicKey, keysize, rsa->n);
	/* set the public exponent */
	rsa->e = BN_bin2bn(exp, sizeof(exp), rsa->e);

	if (rsa->n == NULL || rsa->e == NULL) {
		rv = TSPERR(TSS_E_OUTOFMEMORY);
		goto err;
	}

	/* padding constraint for PKCS#1 OAEP padding */
	if ((int)dataToEncryptLen >= (RSA_size(rsa) - ((2 * SHA_DIGEST_LENGTH) + 1))) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	}

	encodedDataLen = MIN(RSA_size(rsa), 256);

	/* perform our OAEP padding here with custom padding parameter */
	rv = RSA_padding_add_PKCS1_OAEP(encodedData, encodedDataLen, dataToEncrypt,
			dataToEncryptLen, oaepPad, oaepPadLen);
	if (rv != EVP_SUCCESS) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	}

	/* call OpenSSL with no additional padding */
	rv = RSA_public_encrypt(encodedDataLen, encodedData,
				encryptedData, rsa, RSA_NO_PADDING);
	if (rv == -1) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	}

	/* RSA_public_encrypt returns the size of the encrypted data */
	*encryptedDataLen = rv;
	rv = TSS_SUCCESS;
	goto out;

err:
	DEBUG_print_openssl_errors();
out:
	if (rsa)
		RSA_free(rsa);
        return rv;
}

TSS_RESULT
Trspi_Verify(UINT32 HashType, BYTE *pHash, UINT32 iHashLength,
	     unsigned char *pModulus, int iKeyLength,
	     BYTE *pSignature, UINT32 sig_len)
{
	int rv, nid;
	unsigned char exp[] = { 0x01, 0x00, 0x01 }; /* The default public exponent for the TPM */
	unsigned char buf[256];
	RSA *rsa = RSA_new();

	if (rsa == NULL) {
		rv = TSPERR(TSS_E_OUTOFMEMORY);
		goto err;
	}

	/* We assume we're verifying data from a TPM, so there are only
	 * two options, SHA1 data and PKCSv1.5 encoded signature data.
	 */
	switch (HashType) {
		case TSS_HASH_SHA1:
			nid = NID_sha1;
			break;
		case TSS_HASH_OTHER:
			nid = NID_undef;
			break;
		default:
			rv = TSPERR(TSS_E_BAD_PARAMETER);
			goto out;
			break;
	}

	/* set the public key value in the OpenSSL object */
	rsa->n = BN_bin2bn(pModulus, iKeyLength, rsa->n);
	/* set the public exponent */
	rsa->e = BN_bin2bn(exp, sizeof(exp), rsa->e);

	if (rsa->n == NULL || rsa->e == NULL) {
		rv = TSPERR(TSS_E_OUTOFMEMORY);
		goto err;
	}

	/* if we don't know the structure of the data we're verifying, do a public decrypt
	 * and compare manually. If we know we're looking for a SHA1 hash, allow OpenSSL
	 * to do the work for us.
	 */
	if (nid == NID_undef) {
		rv = RSA_public_decrypt(sig_len, pSignature, buf, rsa, RSA_PKCS1_PADDING);
		if ((UINT32)rv != iHashLength) {
			rv = TSPERR(TSS_E_FAIL);
			goto out;
		} else if (memcmp(pHash, buf, iHashLength)) {
			rv = TSPERR(TSS_E_FAIL);
			goto out;
		}
	} else {
		if ((rv = RSA_verify(nid, pHash, iHashLength, pSignature, sig_len, rsa)) == 0) {
			rv = TSPERR(TSS_E_FAIL);
			goto out;
		}
	}

	rv = TSS_SUCCESS;
	goto out;

err:
	DEBUG_print_openssl_errors();
out:
	if (rsa)
		RSA_free(rsa);
        return rv;
}

int
Trspi_RSA_Public_Encrypt(unsigned char *in, unsigned int inlen,
			 unsigned char *out, unsigned int *outlen,
			 unsigned char *pubkey, unsigned int pubsize,
			 unsigned int e, int padding)
{
	int rv, e_size = 3;
	unsigned char exp[] = { 0x01, 0x00, 0x01 };
	RSA *rsa = RSA_new();

	if (rsa == NULL) {
		rv = TSPERR(TSS_E_OUTOFMEMORY);
		goto err;
	}

	switch (e) {
		case 0:
			/* fall through */
		case 65537:
			break;
		case 17:
			exp[0] = 17;
			e_size = 1;
			break;
		case 3:
			exp[0] = 3;
			e_size = 1;
			break;
		default:
			rv = TSPERR(TSS_E_INTERNAL_ERROR);
			goto out;
			break;
	}

	switch (padding) {
		case TR_RSA_PKCS1_OAEP_PADDING:
			padding = RSA_PKCS1_OAEP_PADDING;
			break;
		case TR_RSA_PKCS1_PADDING:
			padding = RSA_PKCS1_PADDING;
			break;
		case TR_RSA_NO_PADDING:
			padding = RSA_NO_PADDING;
			break;
		default:
			rv = TSPERR(TSS_E_INTERNAL_ERROR);
			goto out;
			break;
	}

	/* set the public key value in the OpenSSL object */
	rsa->n = BN_bin2bn(pubkey, pubsize, rsa->n);
	/* set the public exponent */
	rsa->e = BN_bin2bn(exp, e_size, rsa->e);

	if (rsa->n == NULL || rsa->e == NULL) {
		rv = TSPERR(TSS_E_OUTOFMEMORY);
		goto err;
	}

	rv = RSA_public_encrypt(inlen, in, out, rsa, padding);
	if (rv == -1) {
		rv = TSPERR(TSS_E_INTERNAL_ERROR);
		goto err;
	}

	/* RSA_public_encrypt returns the size of the encrypted data */
	*outlen = rv;
	rv = TSS_SUCCESS;
	goto out;

err:
	DEBUG_print_openssl_errors();
out:
	if (rsa)
		RSA_free(rsa);
        return rv;
}
