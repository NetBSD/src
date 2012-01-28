
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_PCRS_H_
#define _OBJ_PCRS_H_

#ifdef TSS_BUILD_PCRS_LIST

/* structures */
struct tr_pcrs_obj {
	UINT32 type;
	union {
		TPM_PCR_INFO info11;
		TPM_PCR_INFO_SHORT infoshort;
		TPM_PCR_INFO_LONG infolong;
	} info;
	TPM_PCRVALUE *pcrs;
};

/* obj_pcrs.c */
void       pcrs_free(void *data);
TSS_BOOL   obj_is_pcrs(TSS_HOBJECT);
TSS_RESULT obj_pcrs_get_tsp_context(TSS_HPCRS, TSS_HCONTEXT *);
TSS_RESULT obj_pcrs_add(TSS_HCONTEXT, UINT32, TSS_HOBJECT *);
TSS_RESULT obj_pcrs_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_RESULT obj_pcrs_get_type(TSS_HPCRS, UINT32 *);
TSS_RESULT obj_pcrs_select_index(TSS_HPCRS, UINT32);
TSS_RESULT obj_pcrs_select_index_ex(TSS_HPCRS, UINT32, UINT32);
TSS_RESULT obj_pcrs_get_value(TSS_HPCRS, UINT32, UINT32 *, BYTE **);
TSS_RESULT obj_pcrs_set_value(TSS_HPCRS, UINT32, UINT32, BYTE *);
TSS_RESULT obj_pcrs_set_values(TSS_HPCRS hPcrs, TCPA_PCR_COMPOSITE *);
TSS_RESULT obj_pcrs_get_selection(TSS_HPCRS, UINT32 *, BYTE *);
TSS_RESULT obj_pcrs_get_digest_at_release(TSS_HPCRS, UINT32 *, BYTE **);
TSS_RESULT obj_pcrs_set_digest_at_release(TSS_HPCRS, TPM_COMPOSITE_HASH);
TSS_RESULT obj_pcrs_create_info_type(TSS_HPCRS, UINT32 *, UINT32 *, BYTE **);
TSS_RESULT obj_pcrs_create_info(TSS_HPCRS, UINT32 *, BYTE **);
TSS_RESULT obj_pcrs_create_info_long(TSS_HPCRS, UINT32 *, BYTE **);
TSS_RESULT obj_pcrs_create_info_short(TSS_HPCRS, UINT32 *, BYTE **);
TSS_RESULT obj_pcrs_get_locality(TSS_HPCRS, UINT32 *);
TSS_RESULT obj_pcrs_set_locality(TSS_HPCRS, UINT32);

#define PCRS_LIST_DECLARE		struct obj_list pcrs_list
#define PCRS_LIST_DECLARE_EXTERN	extern struct obj_list pcrs_list
#define PCRS_LIST_INIT()		list_init(&pcrs_list)
#define PCRS_LIST_CONNECT(a,b)		obj_connectContext_list(&pcrs_list, a, b)
#define PCRS_LIST_CLOSE(a)		obj_list_close(&pcrs_list, &pcrs_free, a)

#else

#define obj_is_pcrs(a)	FALSE

#define PCRS_LIST_DECLARE
#define PCRS_LIST_DECLARE_EXTERN
#define PCRS_LIST_INIT()
#define PCRS_LIST_CONNECT(a,b)
#define PCRS_LIST_CLOSE(a)

#endif

#endif
