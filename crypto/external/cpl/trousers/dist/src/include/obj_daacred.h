
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_DAACRED_H_
#define _OBJ_DAACRED_H_

#ifdef TSS_BUILD_DAA

/* structures */
struct tr_daacred_obj {
	UINT32 session_handle; // set by [join|sign] stage 0.
	TPM_HANDLE tpm_handle;
};

/* prototypes */
void       daacred_free(void *data);
TSS_RESULT obj_daacred_add(TSS_HCONTEXT tspContext, TSS_HOBJECT *phObject);
TSS_RESULT obj_daacred_remove(TSS_HDAA_CREDENTIAL, TSS_HCONTEXT);
TSS_BOOL   obj_is_daacred(TSS_HDAA_CREDENTIAL);
TSS_RESULT obj_daacred_get_tsp_context(TSS_HDAA_CREDENTIAL, TSS_HCONTEXT *);
TSS_RESULT obj_daacred_get_handle_tpm(TSS_HDAA_CREDENTIAL, TPM_HANDLE *);
TSS_RESULT obj_daacred_set_handle_tpm(TSS_HDAA_CREDENTIAL, TPM_HANDLE);
TSS_RESULT obj_daacred_get_session_handle(TSS_HDAA_CREDENTIAL, UINT32 *);
TSS_RESULT obj_daacred_set_session_handle(TSS_HDAA_CREDENTIAL, UINT32);

#define DAACRED_LIST_DECLARE		struct obj_list daacred_list
#define DAACRED_LIST_DECLARE_EXTERN	extern struct obj_list daacred_list
#define DAACRED_LIST_INIT()		list_init(&daacred_list)
#define DAACRED_LIST_CONNECT(a,b)	obj_connectContext_list(&daacred_list, a, b)
#define DAACRED_LIST_CLOSE(a)		obj_list_close(&daacred_list, &daacred_free, a)

#else

#define obj_is_daacred(a)	FALSE

#define DAACRED_LIST_DECLARE
#define DAACRED_LIST_DECLARE_EXTERN
#define DAACRED_LIST_INIT()
#define DAACRED_LIST_CONNECT(a,b)
#define DAACRED_LIST_CLOSE(a)

#endif

#endif
