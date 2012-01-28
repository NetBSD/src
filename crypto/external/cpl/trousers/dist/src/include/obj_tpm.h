
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_TPM_H_
#define _OBJ_TPM_H_

/* structures */
struct tr_tpm_obj {
	TSS_HPOLICY policy;
#ifdef TSS_BUILD_TSS12
	TSS_HPOLICY operatorPolicy;
#endif
	TSS_ALGORITHM_ID collateAlg;
	TSS_ALGORITHM_ID activateAlg;
	PVOID collateAppData;
	PVOID activateAppData;
	TSS_RESULT (*Tspicb_CollateIdentity)(
			PVOID lpAppData,
			UINT32 ulTCPAPlainIdentityProofLength,
			BYTE *rgbTCPAPlainIdentityProof,
			TSS_ALGORITHM_ID algID,
			UINT32* ulSessionKeyLength,
			BYTE *rgbSessionKey,
			UINT32 *pulTCPAIdentityProofLength,
			BYTE *rgbTCPAIdentityProof);
	TSS_RESULT (*Tspicb_ActivateIdentity)(
			PVOID lpAppData,
			UINT32 ulSessionKeyLength,
			BYTE *rgbSessionKey,
			UINT32 ulSymCAAttestationBlobLength,
			BYTE *rgbSymCAAttestationBlob,
			UINT32 *pulCredentialLength,
			BYTE *rgbCredential);
	TSS_COUNTER_ID ctr_id;
	UINT32 EndorsementCredSize;
	BYTE *EndorsementCred;
	UINT32 PlatformCredSize;
	BYTE *PlatformCred;
	UINT32 PlatformConfCredSize;
	BYTE *PlatformConfCred;
	UINT32 ConformanceCredSize;
	BYTE *ConformanceCred;
};

/* prototypes */
TSS_RESULT	   obj_getTpmObject(UINT32, TSS_HOBJECT *);

/* obj_tpm.c */
void       tpm_free(void *);
TSS_BOOL   obj_is_tpm(TSS_HOBJECT);
TSS_RESULT obj_tpm_get_tsp_context(TSS_HTPM, TSS_HCONTEXT *);
TSS_RESULT obj_tpm_get(TSS_HCONTEXT, TSS_HTPM *);
TSS_RESULT obj_tpm_set_policy(TSS_HTPM, TSS_HPOLICY);
TSS_RESULT obj_tpm_add(TSS_HCONTEXT, TSS_HOBJECT *);
TSS_RESULT obj_tpm_get_policy(TSS_HTPM, UINT32, TSS_HPOLICY *);
TSS_RESULT obj_tpm_set_cb12(TSS_HTPM, TSS_FLAG, BYTE *);
TSS_RESULT obj_tpm_get_cb12(TSS_HTPM, TSS_FLAG, UINT32 *, BYTE **);
TSS_RESULT obj_tpm_set_cb11(TSS_HTPM, TSS_FLAG, TSS_FLAG, UINT32);
TSS_RESULT obj_tpm_get_cb11(TSS_HTPM, TSS_FLAG, UINT32 *);
void       obj_tpm_remove_policy_refs(TSS_HPOLICY, TSS_HCONTEXT);
TSS_RESULT obj_tpm_get_current_counter(TSS_HTPM, TSS_COUNTER_ID *);
TSS_RESULT obj_tpm_set_cred(TSS_HTPM, TSS_FLAG, UINT32, BYTE *);
TSS_RESULT obj_tpm_get_cred(TSS_HTPM, TSS_FLAG, UINT32 *, BYTE **);

#define TPM_LIST_DECLARE		struct obj_list tpm_list
#define TPM_LIST_DECLARE_EXTERN		extern struct obj_list tpm_list
#define TPM_LIST_INIT()			list_init(&tpm_list)
#define TPM_LIST_CONNECT(a,b)		obj_connectContext_list(&tpm_list, a, b)
#define TPM_LIST_CLOSE(a)		obj_list_close(&tpm_list, &tpm_free, a)

#endif
