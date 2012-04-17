
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_DAA_H_
#define _OBJ_DAA_H_

#ifdef TSS_BUILD_DAA

/* structures */
struct tr_daa_obj {
	UINT32 session_handle; // set by [join|sign] stage 0.
	TPM_HANDLE tpm_handle;
};

/* obj_daa.c */
void       daa_free(void *data);
TSS_RESULT obj_daa_add(TSS_HCONTEXT tspContext, TSS_HOBJECT *phObject);
TSS_RESULT obj_daa_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_BOOL   obj_is_daa(TSS_HOBJECT);
TSS_RESULT obj_daa_get_tsp_context(TSS_HDAA, TSS_HCONTEXT *);
TSS_RESULT obj_daa_get_handle_tpm(TSS_HDAA, TPM_HANDLE *);
TSS_RESULT obj_daa_set_handle_tpm(TSS_HDAA, TPM_HANDLE);
TSS_RESULT obj_daa_get_session_handle(TSS_HDAA, UINT32 *);
TSS_RESULT obj_daa_set_session_handle(TSS_HDAA, UINT32);

#define DAA_LIST_DECLARE		struct obj_list daa_list
#define DAA_LIST_DECLARE_EXTERN		extern struct obj_list daa_list
#define DAA_LIST_INIT()			list_init(&daa_list)
#define DAA_LIST_CONNECT(a,b)		obj_connectContext_list(&daa_list, a, b)
#define DAA_LIST_CLOSE(a)		obj_list_close(&daa_list, &daa_free, a)

#else

#define obj_is_daa(a)	FALSE

#define DAA_LIST_DECLARE
#define DAA_LIST_DECLARE_EXTERN
#define DAA_LIST_INIT()
#define DAA_LIST_CONNECT(a,b)
#define DAA_LIST_CLOSE(a)

#endif

#endif
