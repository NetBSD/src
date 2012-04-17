
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_POLICY_H_
#define _OBJ_POLICY_H_

/* structures */
struct tr_policy_obj {
	BYTE SecretLifetime;
	TSS_BOOL SecretSet;
	UINT32 SecretMode;
	UINT32 SecretCounter;
	UINT32 SecretTimeStamp;
	UINT32 SecretSize;
	BYTE Secret[20];
	UINT32 type;
	BYTE *popupString;
	UINT32 popupStringLength;
	UINT32 hashMode;
	TSS_ALGORITHM_ID hmacAlg;
	TSS_ALGORITHM_ID xorAlg;
	TSS_ALGORITHM_ID takeownerAlg;
	TSS_ALGORITHM_ID changeauthAlg;
#ifdef TSS_BUILD_SEALX
	TSS_ALGORITHM_ID sealxAlg;
#endif
	PVOID hmacAppData;
	PVOID xorAppData;
	PVOID takeownerAppData;
	PVOID changeauthAppData;
#ifdef TSS_BUILD_SEALX
	PVOID sealxAppData;
#endif
#ifdef TSS_BUILD_DELEGATION
	/* The per1 and per2 are only used when creating a delegation.
	   After that, the blob or index is used to retrieve the information */
	UINT32 delegationPer1;
	UINT32 delegationPer2;

	UINT32 delegationType;
	TSS_BOOL delegationIndexSet;	/* Since 0 is a valid index value */
	UINT32 delegationIndex;
	UINT32 delegationBlobLength;
	BYTE *delegationBlob;
#endif
	TSS_RESULT (*Tspicb_CallbackHMACAuth)(
			PVOID lpAppData,
			TSS_HOBJECT hAuthorizedObject,
			TSS_BOOL ReturnOrVerify,
			UINT32 ulPendingFunction,
			TSS_BOOL ContinueUse,
			UINT32 ulSizeNonces,
			BYTE *rgbNonceEven,
			BYTE *rgbNonceOdd,
			BYTE *rgbNonceEvenOSAP,
			BYTE *rgbNonceOddOSAP,
			UINT32 ulSizeDigestHmac,
			BYTE *rgbParamDigest,
			BYTE *rgbHmacData);
	TSS_RESULT (*Tspicb_CallbackXorEnc)(
			PVOID lpAppData,
			TSS_HOBJECT hOSAPObject,
			TSS_HOBJECT hObject,
			TSS_FLAG PurposeSecret,
			UINT32 ulSizeNonces,
			BYTE *rgbNonceEven,
			BYTE *rgbNonceOdd,
			BYTE *rgbNonceEvenOSAP,
			BYTE *rgbNonceOddOSAP,
			UINT32 ulSizeEncAuth,
			BYTE *rgbEncAuthUsage,
			BYTE *rgbEncAuthMigration);
	TSS_RESULT (*Tspicb_CallbackTakeOwnership)(
			PVOID lpAppData,
			TSS_HOBJECT hObject,
			TSS_HKEY hObjectPubKey,
			UINT32 ulSizeEncAuth,
			BYTE *rgbEncAuth);
	TSS_RESULT (*Tspicb_CallbackChangeAuthAsym)(
			PVOID lpAppData,
			TSS_HOBJECT hObject,
			TSS_HKEY hObjectPubKey,
			UINT32 ulSizeEncAuth,
			UINT32 ulSizeAithLink,
			BYTE *rgbEncAuth,
			BYTE *rgbAuthLink);
#ifdef TSS_BUILD_SEALX
	TSS_RESULT (*Tspicb_CallbackSealxMask)(
			PVOID lpAppData,
			TSS_HKEY hKey,
			TSS_HENCDATA hEncData,
			TSS_ALGORITHM_ID algID,
			UINT32 ulSizeNonces,
			BYTE *rgbNonceEven,
			BYTE *rgbNonceOdd,
			BYTE *rgbNonceEvenOSAP,
			BYTE *rgbNonceOddOSAP,
			UINT32 ulDataLength,
			BYTE *rgbDataToMask,
			BYTE *rgbMaskedData);
#endif
};

/* obj_policy.c */
void       __tspi_policy_free(void *data);
TSS_BOOL   anyPopupPolicies(TSS_HCONTEXT);
TSS_BOOL   obj_is_policy(TSS_HOBJECT);
TSS_RESULT obj_policy_get_tsp_context(TSS_HPOLICY, TSS_HCONTEXT *);
/* One of these 2 flags should be passed to obj_policy_get_secret so that if a popup must
 * be executed to get the secret, we know whether or not the new dialog should be displayed,
 * which will ask for confirmation */
#define TR_SECRET_CTX_NEW	TRUE
#define TR_SECRET_CTX_NOT_NEW	FALSE
TSS_RESULT obj_policy_get_secret(TSS_HPOLICY, TSS_BOOL, TCPA_SECRET *);
TSS_RESULT obj_policy_flush_secret(TSS_HPOLICY);
TSS_RESULT obj_policy_set_secret_object(TSS_HPOLICY, TSS_FLAG, UINT32,
					TCPA_DIGEST *, TSS_BOOL);
TSS_RESULT obj_policy_set_secret(TSS_HPOLICY, TSS_FLAG, UINT32, BYTE *);
TSS_RESULT obj_policy_get_type(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_RESULT obj_policy_add(TSS_HCONTEXT, UINT32, TSS_HOBJECT *);
TSS_RESULT obj_policy_set_type(TSS_HPOLICY, UINT32);
TSS_RESULT obj_policy_set_cb12(TSS_HPOLICY, TSS_FLAG, BYTE *);
TSS_RESULT obj_policy_get_cb12(TSS_HPOLICY, TSS_FLAG, UINT32 *, BYTE **);
TSS_RESULT obj_policy_set_cb11(TSS_HPOLICY, TSS_FLAG, TSS_FLAG, UINT32);
TSS_RESULT obj_policy_get_cb11(TSS_HPOLICY, TSS_FLAG, UINT32 *);
TSS_RESULT obj_policy_get_lifetime(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_set_lifetime(TSS_HPOLICY, UINT32, UINT32);
TSS_RESULT obj_policy_get_counter(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_get_string(TSS_HPOLICY, UINT32 *size, BYTE **);
TSS_RESULT obj_policy_set_string(TSS_HPOLICY, UINT32 size, BYTE *);
TSS_RESULT obj_policy_get_secs_until_expired(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_has_expired(TSS_HPOLICY, TSS_BOOL *);
TSS_RESULT obj_policy_get_mode(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_dec_counter(TSS_HPOLICY);
TSS_RESULT obj_policy_do_hmac(TSS_HPOLICY, TSS_HOBJECT, TSS_BOOL, UINT32,
			      TSS_BOOL, UINT32, BYTE *, BYTE *, BYTE *, BYTE *,
			      UINT32, BYTE *, BYTE *);
TSS_RESULT obj_policy_do_xor(TSS_HPOLICY, TSS_HOBJECT, TSS_HOBJECT, TSS_FLAG,
		UINT32, BYTE *, BYTE *, BYTE *, BYTE *, UINT32, BYTE *, BYTE *);
TSS_RESULT obj_policy_do_takeowner(TSS_HPOLICY, TSS_HOBJECT, TSS_HKEY, UINT32, BYTE *);
TSS_RESULT obj_policy_validate_auth_oiap(TSS_HPOLICY, TCPA_DIGEST *, TPM_AUTH *);
TSS_RESULT obj_policy_get_hash_mode(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_set_hash_mode(TSS_HPOLICY, UINT32);
TSS_RESULT obj_policy_get_xsap_params(TSS_HPOLICY, TPM_COMMAND_CODE, TPM_ENTITY_TYPE *, UINT32 *,
				      BYTE **, BYTE *, TSS_CALLBACK *, TSS_CALLBACK *,
				      TSS_CALLBACK *, UINT32 *, TSS_BOOL);
TSS_RESULT obj_policy_is_secret_set(TSS_HPOLICY, TSS_BOOL *);
#ifdef TSS_BUILD_DELEGATION
TSS_RESULT obj_policy_set_delegation_type(TSS_HPOLICY, UINT32);
TSS_RESULT obj_policy_get_delegation_type(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_set_delegation_index(TSS_HPOLICY, UINT32);
TSS_RESULT obj_policy_get_delegation_index(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_set_delegation_per1(TSS_HPOLICY, UINT32);
TSS_RESULT obj_policy_get_delegation_per1(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_set_delegation_per2(TSS_HPOLICY, UINT32);
TSS_RESULT obj_policy_get_delegation_per2(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_set_delegation_blob(TSS_HPOLICY, UINT32, UINT32, BYTE *);
TSS_RESULT obj_policy_get_delegation_blob(TSS_HPOLICY, UINT32, UINT32 *, BYTE **);
TSS_RESULT obj_policy_get_delegation_label(TSS_HPOLICY, BYTE *);
TSS_RESULT obj_policy_get_delegation_familyid(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_get_delegation_vercount(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_get_delegation_pcr_locality(TSS_HPOLICY, UINT32 *);
TSS_RESULT obj_policy_get_delegation_pcr_digest(TSS_HPOLICY, UINT32 *, BYTE **);
TSS_RESULT obj_policy_get_delegation_pcr_selection(TSS_HPOLICY, UINT32 *, BYTE **);
TSS_RESULT obj_policy_is_delegation_index_set(TSS_HPOLICY, TSS_BOOL *);

void obj_policy_clear_delegation(struct tr_policy_obj *);
TSS_RESULT obj_policy_get_delegate_public(struct tsp_object *, TPM_DELEGATE_PUBLIC *);
#endif

#define POLICY_LIST_DECLARE		struct obj_list policy_list
#define POLICY_LIST_DECLARE_EXTERN	extern struct obj_list policy_list
#define POLICY_LIST_INIT()		list_init(&policy_list)
#define POLICY_LIST_CONNECT(a,b)	obj_connectContext_list(&policy_list, a, b)
#define POLICY_LIST_CLOSE(a)		obj_list_close(&policy_list, &__tspi_policy_free, a)

#endif
