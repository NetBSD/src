
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _TROUSERS_H_
#define _TROUSERS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Utility functions offered by trousers for use in your TSS app.
 *
 * All functions listed here are specific to the trousers TSS and should not be
 * used in applications that are intended to be portable.
 *
 */

/* Blob unloading functions */
void       Trspi_UnloadBlob(UINT64 *offset, size_t size, BYTE *container, BYTE *object);
void       Trspi_UnloadBlob_BYTE(UINT64 *offset, BYTE *dataOut, BYTE *blob);
void       Trspi_UnloadBlob_BOOL(UINT64 *offset, TSS_BOOL *dataOut, BYTE *blob);
void       Trspi_UnloadBlob_UINT64(UINT64 *offset, UINT64 *out, BYTE *blob);
void       Trspi_UnloadBlob_UINT32(UINT64 *offset, UINT32 *out, BYTE *blob);
void       Trspi_UnloadBlob_UINT16(UINT64 *offset, UINT16 *out, BYTE *blob);
void       Trspi_UnloadBlob_TSS_VERSION(UINT64 *offset, BYTE *blob, TSS_VERSION *out);
void       Trspi_UnloadBlob_TCPA_VERSION(UINT64 *offset, BYTE *blob, TCPA_VERSION *out);
TSS_RESULT Trspi_UnloadBlob_PCR_INFO(UINT64 *offset, BYTE *blob, TCPA_PCR_INFO *pcr);
TSS_RESULT Trspi_UnloadBlob_PCR_INFO_LONG(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_LONG *pcr);
TSS_RESULT Trspi_UnloadBlob_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_SHORT *pcr);
TSS_RESULT Trspi_UnloadBlob_PCR_SELECTION(UINT64 *offset, BYTE *blob, TCPA_PCR_SELECTION *pcr);
TSS_RESULT Trspi_UnloadBlob_PCR_COMPOSITE(UINT64 *offset, BYTE *blob, TCPA_PCR_COMPOSITE *out);
TSS_RESULT Trspi_UnloadBlob_STORED_DATA(UINT64 *offset, BYTE *blob, TCPA_STORED_DATA *data);
void       Trspi_UnloadBlob_KEY_FLAGS(UINT64 *offset, BYTE *blob, TCPA_KEY_FLAGS *flags);
TSS_RESULT Trspi_UnloadBlob_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_KEY_PARMS *keyParms);
void       Trspi_UnloadBlob_UUID(UINT64 *offset, BYTE *blob, TSS_UUID *uuid);
TSS_RESULT Trspi_UnloadBlob_STORE_PUBKEY(UINT64 *, BYTE *, TCPA_STORE_PUBKEY *);
void       Trspi_UnloadBlob_DIGEST(UINT64 *offset, BYTE *blob, TPM_DIGEST *digest);
TSS_RESULT Trspi_UnloadBlob_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_PUBKEY *pubKey);
TSS_RESULT Trspi_UnloadBlob_KEY(UINT64 *offset, BYTE *blob, TCPA_KEY *key);
TSS_RESULT Trspi_UnloadBlob_KEY12(UINT64 *offset, BYTE *blob, TPM_KEY12 *key);
TSS_RESULT Trspi_UnloadBlob_MIGRATIONKEYAUTH(UINT64 *offset, BYTE *blob, TPM_MIGRATIONKEYAUTH *migAuth);
TSS_RESULT Trspi_UnloadBlob_PCR_EVENT(UINT64 *offset, BYTE *blob, TSS_PCR_EVENT *event);
TSS_RESULT Trspi_UnloadBlob_KM_KEYINFO(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO *info);
TSS_RESULT Trspi_UnloadBlob_KM_KEYINFO2(UINT64 *offset, BYTE *blob, TSS_KM_KEYINFO2 *info);
TSS_RESULT Trspi_UnloadBlob_SYMMETRIC_KEY(UINT64 *offset, BYTE *blob, TCPA_SYMMETRIC_KEY *key);
TSS_RESULT Trspi_UnloadBlob_SYM_CA_ATTESTATION(UINT64 *offset, BYTE *blob, TCPA_SYM_CA_ATTESTATION *sym);
TSS_RESULT Trspi_UnloadBlob_ASYM_CA_CONTENTS(UINT64 *offset, BYTE *blob, TCPA_ASYM_CA_CONTENTS *asym);
TSS_RESULT Trspi_UnloadBlob_IDENTITY_REQ(UINT64 *offset, BYTE *blob, TCPA_IDENTITY_REQ *req);
TSS_RESULT Trspi_UnloadBlob_IDENTITY_PROOF(UINT64 *offset, BYTE *blob, TCPA_IDENTITY_PROOF *proof);
void	   Trspi_UnloadBlob_COUNTER_VALUE(UINT64 *offset, BYTE *blob, TPM_COUNTER_VALUE *ctr);
void	   Trspi_UnloadBlob_CURRENT_TICKS(UINT64 *offset, BYTE *blob, TPM_CURRENT_TICKS *ticks);
void	   Trspi_UnloadBlob_TRANSPORT_PUBLIC(UINT64 *offset, BYTE *blob, TPM_TRANSPORT_PUBLIC *t);
void       Trspi_UnloadBlob_NONCE(UINT64 *offset, BYTE* blob, TPM_NONCE *n);
TSS_RESULT Trspi_UnloadBlob_CERTIFY_INFO(UINT64 *offset, BYTE* blob, TPM_CERTIFY_INFO *c);
void       Trspi_UnloadBlob_TPM_FAMILY_LABEL(UINT64 *offset, BYTE *blob, TPM_FAMILY_LABEL *label);
void       Trspi_UnloadBlob_TPM_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TPM_FAMILY_TABLE_ENTRY *entry);
void       Trspi_UnloadBlob_TPM_DELEGATE_LABEL(UINT64 *offset, BYTE *blob, TPM_DELEGATE_LABEL *label);
void       Trspi_UnloadBlob_TPM_DELEGATIONS(UINT64 *offset, BYTE *blob, TPM_DELEGATIONS *delegations);
TSS_RESULT Trspi_UnloadBlob_TPM_DELEGATE_PUBLIC(UINT64 *offset, BYTE *blob, TPM_DELEGATE_PUBLIC *pub);
TSS_RESULT Trspi_UnloadBlob_TPM_DELEGATE_OWNER_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_OWNER_BLOB *owner);
TSS_RESULT Trspi_UnloadBlob_TPM_DELEGATE_KEY_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_KEY_BLOB *key);
void       Trspi_UnloadBlob_TSS_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TSS_FAMILY_TABLE_ENTRY *entry);
TSS_RESULT Trspi_UnloadBlob_TSS_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TSS_PCR_INFO_SHORT *pcr);
TSS_RESULT Trspi_UnloadBlob_TSS_DELEGATION_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TSS_DELEGATION_TABLE_ENTRY *entry);
TSS_RESULT Trspi_UnloadBlob_TSS_PLATFORM_CLASS(UINT64 *offset, BYTE *blob, TSS_PLATFORM_CLASS *platClass);
TSS_RESULT Trspi_UnloadBlob_CAP_VERSION_INFO(UINT64 *offset, BYTE *blob, TPM_CAP_VERSION_INFO *v);
TSS_RESULT Trspi_UnloadBlob_NV_INDEX(UINT64 *offset, BYTE *blob, TPM_NV_INDEX *v);
TSS_RESULT Trspi_UnloadBlob_NV_ATTRIBUTES(UINT64 *offset, BYTE *blob, TPM_NV_ATTRIBUTES *v);
TSS_RESULT Trspi_UnloadBlob_NV_DATA_PUBLIC(UINT64 *offset, BYTE *blob, TPM_NV_DATA_PUBLIC *v);

/* Blob loading functions */
void Trspi_LoadBlob_BOUND_DATA(UINT64 *, TCPA_BOUND_DATA, UINT32, BYTE *);
void Trspi_LoadBlob_CHANGEAUTH_VALIDATE(UINT64 *, BYTE *, TPM_CHANGEAUTH_VALIDATE *);
void Trspi_LoadBlob(UINT64 *offset, size_t size, BYTE *to, BYTE *from);
void Trspi_LoadBlob_UINT32(UINT64 *offset, UINT32 in, BYTE *blob);
void Trspi_LoadBlob_UINT16(UINT64 *offset, UINT16 in, BYTE *blob);
void Trspi_LoadBlob_BYTE(UINT64 *offset, BYTE data, BYTE *blob);
void Trspi_LoadBlob_BOOL(UINT64 *offset, TSS_BOOL data, BYTE *blob);
void Trspi_LoadBlob_RSA_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_RSA_KEY_PARMS *parms);
void Trspi_LoadBlob_TSS_VERSION(UINT64 *offset, BYTE *blob, TSS_VERSION version);
void Trspi_LoadBlob_TCPA_VERSION(UINT64 *offset, BYTE *blob, TCPA_VERSION version);
void Trspi_LoadBlob_PCR_INFO(UINT64 *offset, BYTE *blob, TCPA_PCR_INFO *pcr);
void Trspi_LoadBlob_PCR_INFO_LONG(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_LONG *pcr);
void Trspi_LoadBlob_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TPM_PCR_INFO_SHORT *pcr);
void Trspi_LoadBlob_PCR_SELECTION(UINT64 *offset, BYTE *blob, TCPA_PCR_SELECTION *pcr);
void Trspi_LoadBlob_STORED_DATA(UINT64 *offset, BYTE *blob, TCPA_STORED_DATA *data);
void Trspi_LoadBlob_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_PUBKEY *pubKey);
void Trspi_LoadBlob_KEY(UINT64 *offset, BYTE *blob, TCPA_KEY *key);
void Trspi_LoadBlob_KEY12(UINT64 *offset, BYTE *blob, TPM_KEY12 *key);
void Trspi_LoadBlob_KEY_FLAGS(UINT64 *offset, BYTE *blob, TCPA_KEY_FLAGS *flags);
void Trspi_LoadBlob_KEY_PARMS(UINT64 *offset, BYTE *blob, TCPA_KEY_PARMS *keyInfo);
void Trspi_LoadBlob_STORE_PUBKEY(UINT64 *offset, BYTE *blob, TCPA_STORE_PUBKEY *store);
void Trspi_LoadBlob_UUID(UINT64 *offset, BYTE *blob, TSS_UUID uuid);
void Trspi_LoadBlob_CERTIFY_INFO(UINT64 *offset, BYTE *blob, TCPA_CERTIFY_INFO *certify);
void Trspi_LoadBlob_STORE_ASYMKEY(UINT64 *offset, BYTE *blob, TCPA_STORE_ASYMKEY *store);
void Trspi_LoadBlob_PCR_EVENT(UINT64 *offset, BYTE *blob, TSS_PCR_EVENT *event);
void Trspi_LoadBlob_PRIVKEY_DIGEST(UINT64 *offset, BYTE *blob, TCPA_KEY *key);
void Trspi_LoadBlob_PRIVKEY_DIGEST12(UINT64 *offset, BYTE *blob, TPM_KEY12 *key);
void Trspi_LoadBlob_SYMMETRIC_KEY(UINT64 *offset, BYTE *blob, TCPA_SYMMETRIC_KEY *key);
void Trspi_LoadBlob_SYM_CA_ATTESTATION(UINT64 *offset, BYTE *blob, TCPA_SYM_CA_ATTESTATION *sym);
void Trspi_LoadBlob_ASYM_CA_CONTENTS(UINT64 *offset, BYTE *blob, TCPA_ASYM_CA_CONTENTS *asym);
void Trspi_LoadBlob_IDENTITY_REQ(UINT64 *offset, BYTE *blob, TCPA_IDENTITY_REQ *req);
void Trspi_LoadBlob_COUNTER_VALUE(UINT64 *offset, BYTE *blob, TPM_COUNTER_VALUE *ctr);
void Trspi_LoadBlob_TRANSPORT_PUBLIC(UINT64 *offset, BYTE *blob, TPM_TRANSPORT_PUBLIC *t);
void Trspi_LoadBlob_TRANSPORT_AUTH(UINT64 *offset, BYTE *blob, TPM_TRANSPORT_AUTH *t);
void Trspi_LoadBlob_SIGN_INFO(UINT64 *offset, BYTE *blob, TPM_SIGN_INFO *s);
void Trspi_LoadBlob_DIGEST(UINT64 *offset, BYTE *blob, TPM_DIGEST *digest);
void Trspi_LoadBlob_NONCE(UINT64 *offset, BYTE *blob, TPM_NONCE *n);
void Trspi_LoadBlob_TPM_FAMILY_LABEL(UINT64 *offset, BYTE *blob, TPM_FAMILY_LABEL *label);
void Trspi_LoadBlob_TPM_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TPM_FAMILY_TABLE_ENTRY *entry);
void Trspi_LoadBlob_TPM_DELEGATE_LABEL(UINT64 *offset, BYTE *blob, TPM_DELEGATE_LABEL *label);
void Trspi_LoadBlob_TPM_DELEGATIONS(UINT64 *offset, BYTE *blob, TPM_DELEGATIONS *delegations);
void Trspi_LoadBlob_TPM_DELEGATE_PUBLIC(UINT64 *offset, BYTE *blob, TPM_DELEGATE_PUBLIC *pub);
void Trspi_LoadBlob_TPM_DELEGATE_OWNER_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_OWNER_BLOB *owner);
void Trspi_LoadBlob_TPM_DELEGATE_KEY_BLOB(UINT64 *offset, BYTE *blob, TPM_DELEGATE_KEY_BLOB *key);
void Trspi_LoadBlob_TSS_FAMILY_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TSS_FAMILY_TABLE_ENTRY *entry);
void Trspi_LoadBlob_TSS_PCR_INFO_SHORT(UINT64 *offset, BYTE *blob, TSS_PCR_INFO_SHORT *pcr);
void Trspi_LoadBlob_TSS_DELEGATION_TABLE_ENTRY(UINT64 *offset, BYTE *blob, TSS_DELEGATION_TABLE_ENTRY *entry);
void Trspi_LoadBlob_MIGRATIONKEYAUTH(UINT64 *offset, BYTE *blob, TPM_MIGRATIONKEYAUTH *migAuth);
void Trspi_LoadBlob_MSA_COMPOSITE(UINT64 *offset, BYTE *blob, TPM_MSA_COMPOSITE *msaComp);
void Trspi_LoadBlob_CMK_AUTH(UINT64 *offset, BYTE *blob, TPM_CMK_AUTH *cmkAuth);
void Trspi_LoadBlob_CAP_VERSION_INFO(UINT64 *offset, BYTE *blob, TPM_CAP_VERSION_INFO *v);

/* Cryptographic Functions */

/* Hash @BufSize bytes at location @Buf using the algorithm @HashType.  Currently only
 * TSS_HASH_SHA1 is a suported type, so 20 bytes will be written to @Digest */
TSS_RESULT Trspi_Hash(UINT32 HashType, UINT32 BufSize, BYTE *Buf, BYTE *Digest);

typedef struct _Trspi_HashCtx {
	void *ctx;
} Trspi_HashCtx;

TSS_RESULT Trspi_HashInit(Trspi_HashCtx *c, UINT32 type);
TSS_RESULT Trspi_HashUpdate(Trspi_HashCtx *c, UINT32 size, BYTE *data);
TSS_RESULT Trspi_HashFinal(Trspi_HashCtx *c, BYTE *out_digest);

/* Functions to support incremental hashing */
TSS_RESULT Trspi_Hash_UINT16(Trspi_HashCtx *c, UINT16 i);
TSS_RESULT Trspi_Hash_UINT32(Trspi_HashCtx *c, UINT32 i);
TSS_RESULT Trspi_Hash_UINT64(Trspi_HashCtx *c, UINT64 i);
TSS_RESULT Trspi_Hash_DAA_PK(Trspi_HashCtx *c, TSS_DAA_PK *pk);
TSS_RESULT Trspi_Hash_PUBKEY(Trspi_HashCtx *c, TCPA_PUBKEY *pubKey);
TSS_RESULT Trspi_Hash_BYTE(Trspi_HashCtx *c, BYTE data);
TSS_RESULT Trspi_Hash_BOOL(Trspi_HashCtx *c, TSS_BOOL data);
TSS_RESULT Trspi_Hash_RSA_KEY_PARMS(Trspi_HashCtx *c, TCPA_RSA_KEY_PARMS *parms);
TSS_RESULT Trspi_Hash_VERSION(Trspi_HashCtx *c, TSS_VERSION *version);
TSS_RESULT Trspi_Hash_STORED_DATA(Trspi_HashCtx *c, TCPA_STORED_DATA *data);
TSS_RESULT Trspi_Hash_PCR_SELECTION(Trspi_HashCtx *c, TCPA_PCR_SELECTION *pcr);
TSS_RESULT Trspi_Hash_KEY(Trspi_HashCtx *c, TCPA_KEY *key);
TSS_RESULT Trspi_Hash_KEY12(Trspi_HashCtx *c, TPM_KEY12 *key);
TSS_RESULT Trspi_Hash_KEY_FLAGS(Trspi_HashCtx *c, TCPA_KEY_FLAGS *flags);
TSS_RESULT Trspi_Hash_KEY_PARMS(Trspi_HashCtx *c, TCPA_KEY_PARMS *keyInfo);
TSS_RESULT Trspi_Hash_STORE_PUBKEY(Trspi_HashCtx *c, TCPA_STORE_PUBKEY *store);
TSS_RESULT Trspi_Hash_UUID(Trspi_HashCtx *c, TSS_UUID uuid);
TSS_RESULT Trspi_Hash_PCR_EVENT(Trspi_HashCtx *c, TSS_PCR_EVENT *event);
TSS_RESULT Trspi_Hash_PRIVKEY_DIGEST(Trspi_HashCtx *c, TCPA_KEY *key);
TSS_RESULT Trspi_Hash_PRIVKEY_DIGEST12(Trspi_HashCtx *c, TPM_KEY12 *key);
TSS_RESULT Trspi_Hash_SYMMETRIC_KEY(Trspi_HashCtx *c, TCPA_SYMMETRIC_KEY *key);
TSS_RESULT Trspi_Hash_IDENTITY_REQ(Trspi_HashCtx *c, TCPA_IDENTITY_REQ *req);
TSS_RESULT Trspi_Hash_CHANGEAUTH_VALIDATE(Trspi_HashCtx *c, TPM_CHANGEAUTH_VALIDATE *caValidate);
TSS_RESULT Trspi_Hash_SYM_CA_ATTESTATION(Trspi_HashCtx *c, TCPA_SYM_CA_ATTESTATION *sym);
TSS_RESULT Trspi_Hash_ASYM_CA_CONTENTS(Trspi_HashCtx *c, TCPA_ASYM_CA_CONTENTS *asym);
TSS_RESULT Trspi_Hash_BOUND_DATA(Trspi_HashCtx *c, TCPA_BOUND_DATA *bd, UINT32 payloadLength);
TSS_RESULT Trspi_Hash_TRANSPORT_AUTH(Trspi_HashCtx *c, TPM_TRANSPORT_AUTH *a);
TSS_RESULT Trspi_Hash_TRANSPORT_LOG_IN(Trspi_HashCtx *c, TPM_TRANSPORT_LOG_IN *l);
TSS_RESULT Trspi_Hash_TRANSPORT_LOG_OUT(Trspi_HashCtx *c, TPM_TRANSPORT_LOG_OUT *l);
TSS_RESULT Trspi_Hash_CURRENT_TICKS(Trspi_HashCtx *c, TPM_CURRENT_TICKS *t);
TSS_RESULT Trspi_Hash_SIGN_INFO(Trspi_HashCtx *c, TPM_SIGN_INFO *s);
TSS_RESULT Trspi_Hash_MSA_COMPOSITE(Trspi_HashCtx *c, TPM_MSA_COMPOSITE *m);
#define Trspi_Hash_DIGEST(c, d)		Trspi_HashUpdate(c, TPM_SHA1_160_HASH_LEN, d)
#define Trspi_Hash_NONCE(c, d)		Trspi_HashUpdate(c, TPM_SHA1_160_HASH_LEN, d)
#define Trspi_Hash_ENCAUTH(c, d)	Trspi_HashUpdate(c, TPM_SHA1_160_HASH_LEN, d)
#define Trspi_Hash_HMAC(c, d)		Trspi_HashUpdate(c, TPM_SHA1_160_HASH_LEN, d)
#define Trspi_Hash_SECRET(c, d)		Trspi_HashUpdate(c, TPM_SHA1_160_HASH_LEN, d)


UINT32 Trspi_HMAC(UINT32 HashType, UINT32 SecretSize, BYTE*Secret, UINT32 BufSize, BYTE*Buf, BYTE*hmacOut);

/* RSA encrypt @dataToEncryptLen bytes at location @dataToEncrypt using public key
 * @publicKey of size @keysize. This data will be encrypted using OAEP padding in
 * the openssl library using the OAEP padding parameter "TCPA".  This will allow
 * data encrypted with this function to be decrypted by a TPM using non-legacy keys */
int Trspi_RSA_Encrypt(unsigned char *dataToEncrypt,
		unsigned int dataToEncryptLen,
		unsigned char *encryptedData,
		unsigned int *encryptedDataLen,
		unsigned char *publicKey,
		unsigned int keysize);

TSS_RESULT Trspi_Verify(UINT32 HashType, BYTE *pHash, UINT32 iHashLength,
			unsigned char *pModulus, int iKeyLength,
			BYTE *pSignature, UINT32 sig_len);

int Trspi_RSA_Public_Encrypt(unsigned char *in, unsigned int inlen,
			     unsigned char *out, unsigned int *outlen,
			     unsigned char *pubkey, unsigned int pubsize,
			     unsigned int e, int padding);

#define TR_RSA_PKCS1_PADDING		1
#define TR_RSA_PKCS1_OAEP_PADDING	2
#define TR_RSA_NO_PADDING		3

#define Trspi_RSA_PKCS15_Encrypt(in,inlen,out,outlen,pubKey,pubSize) \
        Trspi_RSA_Public_Encrypt(in,inlen,out,outlen,pubKey,pubSize,65537,TR_RSA_PKCS1_PADDING)

#define Trspi_RSA_OAEP_Encrypt(in,inlen,out,outlen,pubKey,pubSize) \
        Trspi_RSA_Public_Encrypt(in,inlen,out,outlen,pubKey,pubSize,65537, \
				 TR_RSA_PKCS1_OAEP_PADDING)

#define Trspi_TPM_RSA_OAEP_Encrypt(in,inlen,out,outlen,pubKey,pubSize) \
        Trspi_RSA_Encrypt(in,inlen,out,outlen,pubKey,pubSize)

/* Symmetric Encryption */

TSS_RESULT Trspi_Encrypt_ECB(UINT16 alg, BYTE *key, BYTE *in, UINT32 in_len,
			     BYTE *out, UINT32 *out_len);
TSS_RESULT Trspi_Decrypt_ECB(UINT16 alg, BYTE *key, BYTE *in, UINT32 in_len,
			     BYTE *out, UINT32 *out_len);

#define TR_SYM_MODE_ECB	1
#define TR_SYM_MODE_CBC	2
#define TR_SYM_MODE_CTR	3
#define TR_SYM_MODE_OFB	4

TSS_RESULT Trspi_SymEncrypt(UINT16 alg, UINT16 mode, BYTE *key, BYTE *iv, BYTE *in, UINT32 in_len,
			    BYTE *out, UINT32 *out_len);
TSS_RESULT Trspi_SymDecrypt(UINT16 alg, UINT16 mode, BYTE *key, BYTE *iv, BYTE *in, UINT32 in_len,
			    BYTE *out, UINT32 *out_len);

TSS_RESULT Trspi_MGF1(UINT32 alg, UINT32 seedLen, BYTE *seed, UINT32 outLen, BYTE *out);

/* String Functions */

/* Below UNICODE is in reference to the TSS type of that name, which is
 * actually UTF-16. */

/* Convert @string to a UNICODE string. On error, NULL is returned. If len
 * is non-NULL, *len will be set to the size of the returned buffer. */
BYTE *Trspi_Native_To_UNICODE(BYTE *string, unsigned *len);

/* convert UNICODE @string to a string from the current codeset. If len
 * is non-NULL, *len will be set to the size of the returned buffer. */
BYTE *Trspi_UNICODE_To_Native(BYTE *string, unsigned *len);

/* Error Functions */

/* return a human readable string based on the result */
char *Trspi_Error_String(TSS_RESULT);

/* return a human readable error layer ( "tpm", "tddl", etc...) */
char *Trspi_Error_Layer(TSS_RESULT);

/* return just the error code bits of the result */
TSS_RESULT Trspi_Error_Code(TSS_RESULT);

#ifdef __cplusplus
}
#endif

/* masks */
#define TSS_KEY_SIZE_MASK	0x00000F00
#define TSS_KEY_TYPE_MASK	0x000000F0
#define TSS_ENCDATA_TYPE_MASK	0x0000000F

/* These should be passed an TSS_FLAG parameter as to
 * Tspi_Context_CreateObject
 */
#define TSS_KEY_SIZE(x)		(x & TSS_KEY_SIZE_MASK)
#define TSS_KEY_TYPE(x)		(x & TSS_KEY_TYPE_MASK)
#define TSS_ENCDATA_TYPE(x)	(x & TSS_ENCDATA_TYPE_MASK)

#define TSS_LOCALITY_ALL       (TPM_LOC_ZERO|TPM_LOC_ONE|TPM_LOC_TWO|TPM_LOC_THREE|TPM_LOC_FOUR)

#endif
