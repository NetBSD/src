
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_DAAARAKEY_H_
#define _OBJ_DAAARAKEY_H_

#ifdef TSS_BUILD_DAA

/* structures */
struct tr_daaarakey_obj {
	UINT32 session_handle;
	TPM_HANDLE tpm_handle;
};

/* prototypes */
void       daaarakey_free(void *data);
TSS_RESULT obj_daaarakey_add(TSS_HCONTEXT tspContext, TSS_HOBJECT *phObject);
TSS_RESULT obj_daaarakey_remove(TSS_HDAA_ISSUER_KEY, TSS_HCONTEXT);
TSS_BOOL   obj_is_daaarakey(TSS_HDAA_ISSUER_KEY);
TSS_RESULT obj_daaarakey_get_tsp_context(TSS_HDAA_ISSUER_KEY, TSS_HCONTEXT *);
TSS_RESULT obj_daaarakey_get_handle_tpm(TSS_HDAA_ISSUER_KEY, TPM_HANDLE *);
TSS_RESULT obj_daaarakey_set_handle_tpm(TSS_HDAA_ISSUER_KEY, TPM_HANDLE);
TSS_RESULT obj_daaarakey_get_session_handle(TSS_HDAA_ISSUER_KEY, UINT32 *);
TSS_RESULT obj_daaarakey_set_session_handle(TSS_HDAA_ISSUER_KEY, UINT32);

#define DAAARAKEY_LIST_DECLARE		struct obj_list daaarakey_list
#define DAAARAKEY_LIST_DECLARE_EXTERN	extern struct obj_list daaarakey_list
#define DAAARAKEY_LIST_INIT()		list_init(&daaarakey_list)
#define DAAARAKEY_LIST_CONNECT(a,b)	obj_connectContext_list(&daaarakey_list, a, b)
#define DAAARAKEY_LIST_CLOSE(a)		obj_list_close(&daaarakey_list, &daaarakey_free, a)

#else

#define obj_is_daaarakey(a)	FALSE

#define DAAARAKEY_LIST_DECLARE
#define DAAARAKEY_LIST_DECLARE_EXTERN
#define DAAARAKEY_LIST_INIT()
#define DAAARAKEY_LIST_CONNECT(a,b)
#define DAAARAKEY_LIST_CLOSE(a)

#endif

#endif
