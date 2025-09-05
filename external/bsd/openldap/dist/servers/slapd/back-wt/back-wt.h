/*	$NetBSD: back-wt.h,v 1.3 2025/09/05 21:16:31 christos Exp $	*/

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

#ifndef _BACK_WT_H_
#define _BACK_WT_H_

#include <portable.h>

#include <ac/errno.h>
#include <sys/stat.h>

#include "slap.h"
#include "wiredtiger.h"

/* The default search IDL stack cache depth */
#define DEFAULT_SEARCH_STACK_DEPTH  16

#define WT_CONFIG_MAX 2048

struct wt_info {
	WT_CONNECTION *wi_conn;
	WT_CONNECTION *wi_cache;
	char *wi_home;
	char *wi_config;
	ID	wi_lastid;

	slap_mask_t wi_defaultmask;
	int         wi_nattrs;
	struct wt_attrinfo **wi_attrs;
	void *wi_search_stack;
	int wi_search_stack_depth;

	struct re_s *wi_index_task;

	int wi_flags;
#define WT_IS_OPEN      0x01
#define WT_OPEN_INDEX   0x02
#define WT_DEL_INDEX    0x08
#define WT_RE_OPEN      0x10
#define WT_NEED_UPGRADE 0x20
#define WT_USE_IDLCACHE 0x40
};

#define WT_TABLE_ID2ENTRY "table:id2entry"
#define WT_TABLE_DN2ID "table:dn2id"

#define WT_INDEX_DN "index:id2entry:dn"
#define WT_INDEX_NDN "index:dn2id:ndn"
#define WT_INDEX_PID "index:dn2id:pid"
/* Currently, revdn is primary key, the revdn index is obsolete. */
#define WT_INDEX_REVDN "index:dn2id:revdn"

/* table for cache */
#define WT_TABLE_IDLCACHE "table:idlcache"

#define ITEMzero(item) (memset((item), 0, sizeof(WT_ITEM)))
#define ITEM2bv(item,bv) ((bv)->bv_val = (item)->data, \
						  (bv)->bv_len = (item)->size)
#define bv2ITEM(bv,item) ((item)->data = (bv)->bv_val, \
						 (item)->size = (bv)->bv_len )

#define WT_INDEX_CACHE_SIZE 1024

typedef struct {
	WT_SESSION *session;
	int is_begin_transaction;
	WT_CURSOR *dn2id;
	WT_CURSOR *dn2id_w;
	WT_CURSOR *dn2id_ndn;
	WT_CURSOR *dn2entry;
	WT_CURSOR *id2entry;
	WT_CURSOR *id2entry_add;
	WT_CURSOR *id2entry_update;
	WT_SESSION *idlcache_session;
	WT_CURSOR *index_pid;
} wt_ctx;

/* for the cache of attribute information (which are indexed, etc.) */
typedef struct wt_attrinfo {
	AttributeDescription *ai_desc; /* attribute description cn;lang-en */
	slap_mask_t ai_indexmask;   /* how the attr is indexed  */
	slap_mask_t ai_newmask; /* new settings to replace old mask */
	#ifdef LDAP_COMP_MATCH
	ComponentReference* ai_cr; /*component indexing*/
	#endif
} AttrInfo;

/* These flags must not clash with SLAP_INDEX flags or ops in slap.h! */
#define	WT_INDEX_DELETING	0x8000U	/* index is being modified */
#define	WT_INDEX_UPDATE_OP	0x03	/* performing an index update */

#include "proto-wt.h"

#endif /* _BACK_WT_H_ */

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
