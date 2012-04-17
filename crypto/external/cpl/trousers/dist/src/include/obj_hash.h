
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2007
 *
 */

#ifndef _OBJ_HASH_H_
#define _OBJ_HASH_H_

#ifdef TSS_BUILD_HASH_LIST

/* structures */
struct tr_hash_obj {
	UINT32 type;
	BYTE *hashData;
	UINT32 hashSize;
	UINT32 hashUpdateSize;
	BYTE *hashUpdateBuffer;
};

/* obj_hash.c */
void       __tspi_hash_free(void *data);
TSS_RESULT obj_hash_add(TSS_HCONTEXT, UINT32, TSS_HOBJECT *);
TSS_BOOL   obj_is_hash(TSS_HOBJECT);
TSS_RESULT obj_hash_remove(TSS_HOBJECT, TSS_HCONTEXT);
TSS_RESULT obj_hash_get_tsp_context(TSS_HHASH, TSS_HCONTEXT *);
TSS_RESULT obj_hash_set_value(TSS_HHASH, UINT32, BYTE *);
TSS_RESULT obj_hash_get_value(TSS_HHASH, UINT32 *, BYTE **);
TSS_RESULT obj_hash_update_value(TSS_HHASH, UINT32, BYTE *);

#define HASH_LIST_DECLARE		struct obj_list hash_list
#define HASH_LIST_DECLARE_EXTERN	extern struct obj_list hash_list
#define HASH_LIST_INIT()		list_init(&hash_list)
#define HASH_LIST_CONNECT(a,b)		obj_connectContext_list(&hash_list, a, b)
#define HASH_LIST_CLOSE(a)		obj_list_close(&hash_list, &__tspi_hash_free, a)

#else

#define obj_is_hash(a)	FALSE

#define HASH_LIST_DECLARE
#define HASH_LIST_DECLARE_EXTERN
#define HASH_LIST_INIT()
#define HASH_LIST_CONNECT(a,b)
#define HASH_LIST_CLOSE(a)

#endif

#endif
