
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */

#ifndef _OBJ_DELFAMILY_H_
#define _OBJ_DELFAMILY_H_

#ifdef TSS_BUILD_DELEGATION

#define TSS_DELFAMILY_FLAGS_STATE_ENABLED	(((UINT32)1)<<0)
#define TSS_DELFAMILY_FLAGS_STATE_LOCKED	(((UINT32)1)<<1)

/* structures */
struct tr_delfamily_obj {
	UINT32 stateFlags;
	UINT32 verCount;
	UINT32 familyID;
	BYTE label;
};

/* obj_delfamily.c */
void       delfamily_free(void *data);
TSS_BOOL   obj_is_delfamily(TSS_HOBJECT);
TSS_RESULT obj_delfamily_add(TSS_HCONTEXT, TSS_HOBJECT *);
TSS_RESULT obj_delfamily_remove(TSS_HDELFAMILY, TSS_HOBJECT);
void       obj_delfamily_find_by_familyid(TSS_HOBJECT, UINT32, TSS_HDELFAMILY *);
TSS_RESULT obj_delfamily_get_tsp_context(TSS_HDELFAMILY, TSS_HCONTEXT *);
TSS_RESULT obj_delfamily_set_locked(TSS_HDELFAMILY, TSS_BOOL, TSS_BOOL);
TSS_RESULT obj_delfamily_get_locked(TSS_HDELFAMILY, TSS_BOOL *);
TSS_RESULT obj_delfamily_set_enabled(TSS_HDELFAMILY, TSS_BOOL, TSS_BOOL);
TSS_RESULT obj_delfamily_get_enabled(TSS_HDELFAMILY, TSS_BOOL *);
TSS_RESULT obj_delfamily_set_vercount(TSS_HDELFAMILY, UINT32);
TSS_RESULT obj_delfamily_get_vercount(TSS_HDELFAMILY, UINT32 *);
TSS_RESULT obj_delfamily_set_familyid(TSS_HDELFAMILY, UINT32);
TSS_RESULT obj_delfamily_get_familyid(TSS_HDELFAMILY, UINT32 *);
TSS_RESULT obj_delfamily_set_label(TSS_HDELFAMILY, BYTE);
TSS_RESULT obj_delfamily_get_label(TSS_HDELFAMILY, BYTE *);


#define DELFAMILY_LIST_DECLARE		struct obj_list delfamily_list
#define DELFAMILY_LIST_DECLARE_EXTERN	extern struct obj_list delfamily_list
#define DELFAMILY_LIST_INIT()		list_init(&delfamily_list)
#define DELFAMILY_LIST_CONNECT(a,b)	obj_connectContext_list(&delfamily_list, a, b)
#define DELFAMILY_LIST_CLOSE(a)		obj_list_close(&delfamily_list, &delfamily_free, a)

#else

#define obj_is_delfamily(a)		FALSE

#define DELFAMILY_LIST_DECLARE
#define DELFAMILY_LIST_DECLARE_EXTERN
#define DELFAMILY_LIST_INIT()
#define DELFAMILY_LIST_CONNECT(a,b)
#define DELFAMILY_LIST_CLOSE(a)

#endif

#endif
