/*
 * The Initial Developer of the Original Code is Intel Corporation.
 * Portions created by Intel Corporation are Copyright (C) 2007 Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 *
 * trousers - An open source TCG Software Stack
 *
 * Author: james.xu@intel.com Rossey.liu@intel.com
 *
 */

#ifndef _OBJ_NVSTORE_H_
#define _OBJ_NVSTORE_H_

#ifdef TSS_BUILD_NV

#define MAX_PUBLIC_DATA_SIZE 1024
#define TSS_LOCALITY_MASK 0x1f

typedef struct objNV_DATA_PUBLIC
{
	TPM_STRUCTURE_TAG tag;
	TPM_NV_INDEX nvIndex;
	TPM_NV_ATTRIBUTES permission;
	TPM_BOOL bReadSTClear;
	TPM_BOOL bWriteSTClear;
	TPM_BOOL bWriteDefine;
	UINT32 dataSize;
}NV_DATA_PUBLIC;

/* structures */
struct tr_nvstore_obj {
	TPM_STRUCTURE_TAG tag;
	TPM_NV_INDEX nvIndex;
	TPM_NV_ATTRIBUTES permission;
	TPM_BOOL bReadSTClear;
	TPM_BOOL bWriteSTClear;
	TPM_BOOL bWriteDefine;
	UINT32 dataSize;
	TSS_HPOLICY policy;
};

/* obj_nv.c */
void       nvstore_free(void *data);
TSS_RESULT obj_nvstore_add(TSS_HCONTEXT, TSS_HOBJECT *);
TSS_BOOL   obj_is_nvstore(TSS_HOBJECT tspContext);
TSS_RESULT obj_nvstore_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_RESULT obj_nvstore_get_tsp_context(TSS_HNVSTORE, TSS_HCONTEXT *);
TSS_RESULT obj_nvstore_set_index(TSS_HNVSTORE, UINT32);
TSS_RESULT obj_nvstore_get_index(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_set_datasize(TSS_HNVSTORE, UINT32);
TSS_RESULT obj_nvstore_get_datasize(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_set_permission(TSS_HNVSTORE, UINT32);
TSS_RESULT obj_nvstore_get_permission_from_tpm(TSS_HNVSTORE hNvstore, UINT32 * permission);
TSS_RESULT obj_nvstore_get_permission(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_set_policy(TSS_HNVSTORE, TSS_HPOLICY);
TSS_RESULT obj_nvstore_get_policy(TSS_HNVSTORE, UINT32, TSS_HPOLICY *);
TSS_RESULT obj_nvstore_get_datapublic(TSS_HNVSTORE, UINT32 *, BYTE *);
TSS_RESULT obj_nvstore_get_readdigestatrelease(TSS_HNVSTORE, UINT32 *, BYTE **);
TSS_RESULT obj_nvstore_get_readpcrselection(TSS_HNVSTORE, UINT32 *, BYTE **);
TSS_RESULT obj_nvstore_get_writedigestatrelease(TSS_HNVSTORE, UINT32 *, BYTE **);
TSS_RESULT obj_nvstore_get_writepcrselection(TSS_HNVSTORE, UINT32 *, BYTE **);
TSS_RESULT obj_nvstore_get_state_readstclear(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_get_state_writedefine(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_get_state_writestclear(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_get_readlocalityatrelease(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_get_writelocalityatrelease(TSS_HNVSTORE, UINT32 *);
TSS_RESULT obj_nvstore_create_pcrshortinfo(TSS_HNVSTORE, TSS_HPCRS, UINT32 *, BYTE **);

#define NVSTORE_LIST_DECLARE		struct obj_list nvstore_list
#define NVSTORE_LIST_DECLARE_EXTERN	extern struct obj_list nvstore_list
#define NVSTORE_LIST_INIT()		list_init(&nvstore_list)
#define NVSTORE_LIST_CONNECT(a,b)	obj_connectContext_list(&nvstore_list, a, b)
#define NVSTORE_LIST_CLOSE(a)		obj_list_close(&nvstore_list, &nvstore_free, a)
#else
#define NVSTORE_LIST_DECLARE
#define NVSTORE_LIST_DECLARE_EXTERN
#define NVSTORE_LIST_INIT()
#define NVSTORE_LIST_CONNECT(a,b)
#define NVSTORE_LIST_CLOSE(a)
#endif
#endif

