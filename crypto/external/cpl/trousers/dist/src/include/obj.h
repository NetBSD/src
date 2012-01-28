
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_H_
#define _OBJ_H_

#include "threads.h"

/* definitions */

/* When TRUE, the object has PCRs associated with it */
#define TSS_OBJ_FLAG_PCRS	0x00000001
/* When TRUE, the object has a usage auth secret associated with it */
#define TSS_OBJ_FLAG_USAGEAUTH	0x00000002
/* When TRUE, the object has a migration auth secret associated with it */
#define TSS_OBJ_FLAG_MIGAUTH	0x00000004
/* When TRUE, the object has previously been registered in USER PS */
#define TSS_OBJ_FLAG_USER_PS	0x00000008
/* When TRUE, the object has previously been registered in SYSTEM PS */
#define TSS_OBJ_FLAG_SYSTEM_PS	0x00000010
/* When TRUE, the key has been created and cannot be altered */
#define TSS_OBJ_FLAG_KEY_SET	0x00000020

/* structures */
struct tsp_object {
	UINT32 handle;
	UINT32 tspContext;
	TSS_FLAG flags;
	void *data;
	struct tsp_object *next;
};

struct obj_list {
	struct tsp_object *head;
	MUTEX_DECLARE(lock);
};

/* prototypes */
TSS_RESULT	   obj_getTpmObject(UINT32, TSS_HOBJECT *);
TSS_HOBJECT	   obj_GetPolicyOfObject(UINT32, UINT32);
void		   __tspi_obj_list_init();
TSS_HOBJECT	   obj_get_next_handle();
TSS_RESULT	   obj_list_add(struct obj_list *, UINT32, TSS_FLAG, void *, TSS_HOBJECT *);
TSS_RESULT	   obj_list_remove(struct obj_list *, void (*)(void *), TSS_HOBJECT, TSS_HCONTEXT);
void		   obj_list_put(struct obj_list *);
struct tsp_object *obj_list_get_obj(struct obj_list *, UINT32);
struct tsp_object *obj_list_get_tspcontext(struct obj_list *, UINT32);
void		   obj_list_close(struct obj_list *, void (*)(void *), TSS_HCONTEXT);
void		   obj_connectContext(TSS_HCONTEXT, TCS_CONTEXT_HANDLE);
void		   obj_close_context(TSS_HCONTEXT);
void               obj_lists_remove_policy_refs(TSS_HPOLICY, TSS_HCONTEXT);

/* prototypes for functions that may traverse more than one list */
TSS_RESULT	   obj_tcskey_get_pubkeyhash(TCS_KEY_HANDLE, BYTE *);

#include "obj_tpm.h"
#include "obj_context.h"
#include "obj_hash.h"
#include "obj_pcrs.h"
#include "obj_policy.h"
#include "obj_rsakey.h"
#include "obj_encdata.h"
#include "obj_daacred.h"
#include "obj_daaarakey.h"
#include "obj_daaissuerkey.h"
#include "obj_nv.h"
#include "obj_delfamily.h"
#include "obj_migdata.h"


TPM_LIST_DECLARE_EXTERN;
CONTEXT_LIST_DECLARE_EXTERN;
HASH_LIST_DECLARE_EXTERN;
PCRS_LIST_DECLARE_EXTERN;
POLICY_LIST_DECLARE_EXTERN;
RSAKEY_LIST_DECLARE_EXTERN;
ENCDATA_LIST_DECLARE_EXTERN;
DAACRED_LIST_DECLARE_EXTERN;
DAAARAKEY_LIST_DECLARE_EXTERN;
DAAISSUERKEY_LIST_DECLARE_EXTERN;
NVSTORE_LIST_DECLARE_EXTERN;
DELFAMILY_LIST_DECLARE_EXTERN;
MIGDATA_LIST_DECLARE_EXTERN;

#endif
