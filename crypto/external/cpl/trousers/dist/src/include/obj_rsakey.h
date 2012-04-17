
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_RSAKEY_H_
#define _OBJ_RSAKEY_H_

#ifdef TSS_BUILD_RSAKEY_LIST

/* rsakey specific flags */
#define TSS_RSAKEY_FLAG_OWNEREVICT (0x00000001)

/* structures */
struct tr_rsakey_obj {
	int type;
	TSS_KEY key;
	TSS_FLAG flags;
	TSS_HPOLICY usagePolicy;
	TSS_HPOLICY migPolicy;
	TSS_UUID uuid;
	TCS_KEY_HANDLE tcsHandle;
#ifdef TSS_BUILD_CMK
	TPM_HMAC msaApproval;
	TPM_DIGEST msaDigest;
#endif
	union {
		TPM_PCR_INFO info11;
		TPM_PCR_INFO_LONG infolong;
	} pcrInfo;
	UINT32 pcrInfoType;
};

/* obj_rsakey.c */
void       __tspi_rsakey_free(void *data);
TSS_BOOL   obj_is_rsakey(TSS_HOBJECT);
TSS_RESULT obj_rsakey_add(TSS_HCONTEXT, TSS_FLAG, TSS_HOBJECT *);
TSS_RESULT obj_rsakey_add_by_key(TSS_HCONTEXT, TSS_UUID *, BYTE *, TSS_FLAG, TSS_HKEY *);
TSS_RESULT obj_rsakey_set_policy(TSS_HKEY, TSS_HPOLICY);
TSS_RESULT obj_rsakey_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_RESULT obj_rsakey_get_tsp_context(TSS_HKEY, TSS_HCONTEXT *);
TSS_RESULT obj_rsakey_set_pstype(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_pstype(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_get_usage(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_usage(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_set_migratable(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_set_redirected(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_set_volatile(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_authdata_usage(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_authdata_usage(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_alg(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_alg(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_es(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_es(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_ss(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_ss(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_set_num_primes(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_num_primes(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_flags(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_flags(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_size(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_get_size(TSS_HKEY, UINT32 *);
TSS_BOOL   obj_rsakey_is_migratable(TSS_HKEY);
TSS_BOOL   obj_rsakey_is_redirected(TSS_HKEY);
TSS_BOOL   obj_rsakey_is_volatile(TSS_HKEY);
TSS_RESULT obj_rsakey_get_policy(TSS_HKEY, UINT32, TSS_HPOLICY *, TSS_BOOL *);
TSS_RESULT obj_rsakey_get_policies(TSS_HKEY, TSS_HPOLICY *, TSS_HPOLICY *, TSS_BOOL *);
TSS_RESULT obj_rsakey_get_blob(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_priv_blob(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_pub_blob(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_version(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_exponent(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_set_exponent(TSS_HKEY, UINT32, BYTE *);
TSS_RESULT obj_rsakey_get_modulus(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_set_modulus(TSS_HKEY, UINT32, BYTE *);
TSS_RESULT obj_rsakey_get_uuid(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_parent_uuid(TSS_HKEY, TSS_FLAG *, TSS_UUID *);
TSS_RESULT obj_rsakey_set_uuids(TSS_HKEY, TSS_FLAG, TSS_UUID *, TSS_FLAG, TSS_UUID *);
TSS_RESULT obj_rsakey_set_uuid(TSS_HKEY, TSS_FLAG, TSS_UUID *);
TSS_RESULT obj_rsakey_set_tcpakey(TSS_HKEY, UINT32 , BYTE *);
TSS_RESULT obj_rsakey_get_pcr_digest(TSS_HKEY, UINT32, TSS_FLAG, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_pcr_selection(TSS_HKEY, UINT32, TSS_FLAG, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_get_pcr_locality(TSS_HKEY, TSS_FLAG, UINT32 *);
TSS_RESULT obj_rsakey_set_pubkey(TSS_HKEY, UINT32, BYTE *);
TSS_RESULT obj_rsakey_set_privkey(TSS_HKEY, UINT32, UINT32, BYTE *);
TSS_RESULT obj_rsakey_set_pcr_data(TSS_HKEY, TSS_HPOLICY);
TSS_RESULT obj_rsakey_set_key_parms(TSS_HKEY, TCPA_KEY_PARMS *);
TSS_RESULT obj_rsakey_get_by_uuid(TSS_UUID *, TSS_HKEY *);
TSS_RESULT obj_rsakey_get_by_pub(UINT32, BYTE *, TSS_HKEY *);
TSS_RESULT obj_rsakey_get_tcs_handle(TSS_HKEY, TCS_KEY_HANDLE *);
TSS_RESULT obj_rsakey_set_tcs_handle(TSS_HKEY, TCS_KEY_HANDLE);
void       obj_rsakey_remove_policy_refs(TSS_HPOLICY, TSS_HCONTEXT);
TSS_RESULT obj_rsakey_get_transport_attribs(TSS_HKEY, TCS_KEY_HANDLE *, TPM_DIGEST *);
#ifdef TSS_BUILD_CMK
TSS_BOOL   obj_rsakey_is_cmk(TSS_HKEY);
TSS_RESULT obj_rsakey_set_cmk(TSS_HKEY, UINT32);
TSS_RESULT obj_rsakey_set_msa_approval(TSS_HKEY, UINT32, BYTE *);
TSS_RESULT obj_rsakey_get_msa_approval(TSS_HKEY, UINT32 *, BYTE **);
TSS_RESULT obj_rsakey_set_msa_digest(TSS_HKEY, UINT32, BYTE *);
TSS_RESULT obj_rsakey_get_msa_digest(TSS_HKEY, UINT32 *, BYTE **);
#endif
TSS_RESULT obj_rsakey_get_ownerevict(TSS_HKEY, UINT32 *);
TSS_RESULT obj_rsakey_set_ownerevict(TSS_HKEY, TSS_BOOL);
TSS_RESULT obj_rsakey_set_srk_pubkey(BYTE *);

#define RSAKEY_LIST_DECLARE		struct obj_list rsakey_list
#define RSAKEY_LIST_DECLARE_EXTERN	extern struct obj_list rsakey_list
#define RSAKEY_LIST_INIT()		list_init(&rsakey_list)
#define RSAKEY_LIST_CONNECT(a,b)	obj_connectContext_list(&rsakey_list, a, b)
#define RSAKEY_LIST_CLOSE(a)		obj_list_close(&rsakey_list, &__tspi_rsakey_free, a)

#else

#define obj_is_rsakey(a)	FALSE

#define RSAKEY_LIST_DECLARE
#define RSAKEY_LIST_DECLARE_EXTERN
#define RSAKEY_LIST_INIT()
#define RSAKEY_LIST_CONNECT(a,b)
#define RSAKEY_LIST_CLOSE(a)

#endif

#endif
