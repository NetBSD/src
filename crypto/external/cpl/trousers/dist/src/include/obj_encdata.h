
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_ENCDATA_H_
#define _OBJ_ENCDATA_H_

#ifdef TSS_BUILD_ENCDATA_LIST

/* structures */
struct tr_encdata_obj {
	TSS_HPOLICY usagePolicy;
	UINT32 encryptedDataLength;
	BYTE *encryptedData;
	union {
		TPM_PCR_INFO info11;
		TPM_PCR_INFO_LONG infolong;
	} pcrInfo;
	UINT32 pcrInfoType;
	UINT32 type;
#ifdef TSS_BUILD_SEALX
	UINT32 protectMode;
#endif
};

/* obj_encdata.c */
void       encdata_free(void *data);
TSS_BOOL   obj_is_encdata(TSS_HOBJECT);
TSS_RESULT obj_encdata_set_policy(TSS_HKEY, TSS_HPOLICY);
TSS_RESULT obj_encdata_set_data(TSS_HENCDATA, UINT32, BYTE *);
TSS_RESULT obj_encdata_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_RESULT obj_encdata_get_tsp_context(TSS_HENCDATA, TSS_HCONTEXT *);
TSS_RESULT obj_encdata_add(TSS_HCONTEXT, UINT32, TSS_HOBJECT *);
TSS_RESULT obj_encdata_get_data(TSS_HENCDATA, UINT32 *, BYTE **);
TSS_RESULT obj_encdata_get_pcr_selection(TSS_HENCDATA, TSS_FLAG, TSS_FLAG, UINT32 *, BYTE **);
TSS_RESULT obj_encdata_get_pcr_locality(TSS_HENCDATA, TSS_FLAG, UINT32 *);
TSS_RESULT obj_encdata_get_pcr_digest(TSS_HENCDATA, TSS_FLAG, TSS_FLAG, UINT32 *, BYTE **);
TSS_RESULT obj_encdata_set_pcr_info(TSS_HENCDATA, UINT32, BYTE *);
TSS_RESULT obj_encdata_get_policy(TSS_HENCDATA, UINT32, TSS_HPOLICY *);
void       obj_encdata_remove_policy_refs(TSS_HPOLICY, TSS_HCONTEXT);
#ifdef TSS_BUILD_SEALX
TSS_RESULT obj_encdata_set_seal_protect_mode(TSS_HENCDATA, UINT32);
TSS_RESULT obj_encdata_get_seal_protect_mode(TSS_HENCDATA, UINT32 *);
#endif


#define ENCDATA_LIST_DECLARE		struct obj_list encdata_list
#define ENCDATA_LIST_DECLARE_EXTERN	extern struct obj_list encdata_list
#define ENCDATA_LIST_INIT()		list_init(&encdata_list)
#define ENCDATA_LIST_CONNECT(a,b)	obj_connectContext_list(&encdata_list, a, b)
#define ENCDATA_LIST_CLOSE(a)		obj_list_close(&encdata_list, &encdata_free, a)

#else

#define obj_is_encdata(a)	FALSE

#define ENCDATA_LIST_DECLARE
#define ENCDATA_LIST_DECLARE_EXTERN
#define ENCDATA_LIST_INIT()
#define ENCDATA_LIST_CONNECT(a,b)
#define ENCDATA_LIST_CLOSE(a)

#endif

#endif
