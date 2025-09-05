/*	$NetBSD: proto-wt.h,v 1.3 2025/09/05 21:16:32 christos Exp $	*/

/* OpenLDAP WiredTiger backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2024 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was developed by HAMANO Tsukasa <hamano@osstech.co.jp>
 * based on back-bdb for inclusion in OpenLDAP Software.
 * WiredTiger is a product of MongoDB Inc.
 */

#ifndef _PROTO_WT_H_
#define _PROTO_WT_H_

LDAP_BEGIN_DECL

#define WT_UCTYPE  "WT"

/*
 * attr.c
 */

AttrInfo *wt_attr_mask( struct wt_info *wi, AttributeDescription *desc );
void wt_attr_flush( struct wt_info *wi );
void wt_attr_index_unparse( struct wt_info *wi, BerVarray *bva );
int wt_attr_index_config(
	struct wt_info  *wi,
	const char      *fname,
	int         lineno,
	int         argc,
	char        **argv,
	struct      config_reply_s *c_reply);
void wt_attr_index_destroy( struct wt_info *wi );

/*
 * id2entry.c
 */
int wt_id2entry(BackendDB *be, wt_ctx *wc, ID id, Entry **ep );
int wt_id2entry_add(Operation *op, wt_ctx *wc, Entry *e );
int wt_id2entry_update(Operation *op, wt_ctx *wc, Entry *e );
int wt_id2entry_delete(Operation *op, wt_ctx *wc, Entry *e );

BI_entry_release_rw wt_entry_release;
BI_entry_get_rw wt_entry_get;

int wt_entry_return(Entry *e);
int wt_entry_release(Operation *op, Entry *e, int rw);

/*
 * idl.c
 */

unsigned wt_idl_search( ID *ids, ID id );

ID wt_idl_first( ID *ids, ID *cursor );
ID wt_idl_next( ID *ids, ID *cursor );
int wt_idl_append_one( ID *ids, ID id );
void wt_idl_sort( ID *ids, ID *tmp );
int wt_idl_intersection( ID *a, ID *b );
int wt_filter_candidates(
	Operation *op,
	wt_ctx *wc,
	Filter *f,
	ID *ids,
	ID *tmp,
	ID *stack );
int
wt_idl_union(
	ID  *a,
	ID  *b );

/*
 * index.c
 */

extern AttrInfo *
wt_index_mask LDAP_P((
	Backend *be,
	AttributeDescription *desc,
	struct berval *atname ));

int wt_index_entry LDAP_P(( Operation *op, wt_ctx *wc, int r, Entry *e ));
int wt_index_values(
	Operation *op,
	wt_ctx *wc,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int opid );
int wt_index_param(
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	slap_mask_t *maskp,
	struct berval *prefixp );

WT_CURSOR *wt_index_open(wt_ctx *wc, struct berval *name, int create);

#define wt_index_entry_add(op,t,e) \
	wt_index_entry((op),(t),SLAP_INDEX_ADD_OP,(e))
#define wt_index_entry_del(op,t,e) \
	wt_index_entry((op),(t),SLAP_INDEX_DELETE_OP,(e))

/*
 * key.c
 */
int
wt_key_read( Backend *be,
			 WT_CURSOR *cursor,
			 struct berval *k,
			 ID *ids,
			 WT_CURSOR **saved_cursor,
			 int get_flag);

int
wt_key_change( Backend *be,
			   WT_CURSOR *cursor,
			   struct berval *k,
			   ID id,
			   int op);

/*
 * nextid.c
 */
int wt_next_id(BackendDB *be, ID *out);
int wt_last_id( BackendDB *be, WT_SESSION *session, ID *out );

/*
 * modify.c
 */
int wt_modify_internal(
	Operation *op,
	wt_ctx *wc,
	Modifications *modlist,
	Entry *e,
	const char **text,
	char *textbuf,
	size_t textlen );

/*
 * config.c
 */
int wt_back_init_cf( BackendInfo *bi );

/*
 * dn2id.c
 */

int
wt_dn2id(
	Operation *op,
	wt_ctx *wc,
    struct berval *ndn,
    ID *id);

int
wt_dn2id_add(
	Operation *op,
	wt_ctx *wc,
	ID pid,
	Entry *e);

int
wt_dn2idl(
	Operation *op,
	wt_ctx *wc,
	struct berval *ndn,
	Entry *e,
	ID *ids,
	ID *stack);

int
wt_dn2id_delete(
	Operation *op,
	wt_ctx *wc,
	struct berval *ndn);

int
wt_dn2id_has_children(
	Operation *op,
	wt_ctx *wc,
	ID id );

/*
 * dn2entry.c
 */
int wt_dn2entry( BackendDB *be,
				 wt_ctx *wc,
				 struct berval *ndn,
				 Entry **ep );

int wt_dn2pentry( BackendDB *be,
				  wt_ctx *wc,
				  struct berval *ndn,
				  Entry **ep );
int wt_dn2aentry( BackendDB *be,
				  wt_ctx *wc,
				  struct berval *ndn,
				  Entry **ep );

/*
 * former ctx.c
 */
wt_ctx *wt_ctx_init(struct wt_info *wi);
void wt_ctx_free(void *key, void *data);
wt_ctx *wt_ctx_get(Operation *op, struct wt_info *wi);

/*
 * former cache.c
 */
int wt_idlcache_get(wt_ctx *wc, struct berval *ndn, int scope, ID *ids);
int wt_idlcache_set(wt_ctx *wc, struct berval *ndn, int scope, ID *ids);
int wt_idlcache_begin(wt_ctx *wc, struct berval *ndn, int scope);
int wt_idlcache_clear(Operation *op, wt_ctx *wc, struct berval *ndn);

/*
 * former external.h
 */

extern BI_init              wt_back_initialize;
extern BI_db_config         wt_db_config;
extern BI_op_add            wt_add;
extern BI_op_bind           wt_bind;
extern BI_op_compare        wt_compare;
extern BI_op_delete         wt_delete;
extern BI_op_modify         wt_modify;
extern BI_op_modrdn         wt_modrdn;
extern BI_op_search         wt_search;
extern BI_op_extended       wt_extended;

extern BI_operational       wt_operational;
extern BI_has_subordinates  wt_hasSubordinates;

/* tools.c */
int wt_entry_header(WT_ITEM *item, EntryHeader *eh);
extern BI_tool_entry_open    wt_tool_entry_open;
extern BI_tool_entry_close   wt_tool_entry_close;
extern BI_tool_entry_first_x wt_tool_entry_first_x;
extern BI_tool_entry_next    wt_tool_entry_next;
extern BI_tool_entry_get     wt_tool_entry_get;
extern BI_tool_entry_put     wt_tool_entry_put;
extern BI_tool_entry_reindex wt_tool_entry_reindex;
extern BI_tool_dn2id_get     wt_tool_dn2id_get;
extern BI_tool_entry_modify  wt_tool_entry_modify;
extern BI_tool_entry_delete  wt_tool_entry_delete;

LDAP_END_DECL

#endif /* _PROTO_WT_H */

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */

