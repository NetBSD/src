/*	$NetBSD: tools.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

/* OpenLDAP WiredTiger backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2021 The OpenLDAP Foundation.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: tools.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include "back-wt.h"
#include "slap-config.h"

typedef struct dn_id {
    ID id;
    struct berval dn;
} dn_id;

#define HOLE_SIZE   4096
static dn_id hbuf[HOLE_SIZE], *holes = hbuf;
static unsigned nhmax = HOLE_SIZE;
static unsigned nholes;

static int index_nattrs;

static struct berval    *tool_base;
static int      tool_scope;
static Filter       *tool_filter;
static Entry        *tool_next_entry;

static wt_ctx *wc;
static WT_CURSOR *reader;
static WT_ITEM item;

int
wt_tool_entry_open( BackendDB *be, int mode )
{
    struct wt_info *wi = (struct wt_info *) be->be_private;
	WT_CONNECTION *conn = wi->wi_conn;
	int rc;

	wc = wt_ctx_init(wi);
    if( !wc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_tool_entry_open)
			   ": wt_ctx_get failed\n" );
		return -1;
    }

	rc = wc->session->open_cursor(wc->session, WT_TABLE_ID2ENTRY"(entry)"
								  ,NULL, NULL, &reader);
	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_tool_entry_open)
			   ": cursor open failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return -1;
	}

	return 0;
}

int
wt_tool_entry_close( BackendDB *be )
{
	int rc;

	if( reader ) {
		reader->close(reader);
		reader = NULL;
	}

	wt_ctx_free(NULL, wc);

    if( nholes ) {
        unsigned i;
        fprintf( stderr, "Error, entries missing!\n");
        for (i=0; i<nholes; i++) {
            fprintf(stderr, "  entry %ld: %s\n",
					holes[i].id, holes[i].dn.bv_val);
        }
        return -1;
    }

	return 0;
}

ID
wt_tool_entry_first_x( BackendDB *be,
					   struct berval *base,
					   int scope,
					   Filter *f )
{
	tool_base = base;
	tool_scope = scope;
	tool_filter = f;

	return wt_tool_entry_next( be );
}

ID
wt_tool_entry_next( BackendDB *be )
{
	int rc;
	ID id;

	rc = reader->next(reader);
	switch( rc ){
	case 0:
		break;
	case WT_NOTFOUND:
		return NOID;
	default:
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_tool_entry_next)
			   ": next failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return NOID;
	}

	rc = reader->get_key(reader, &id);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_tool_entry_next)
			   ": get_key failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
	}

	rc = reader->get_value(reader, &item);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_tool_entry_next)
			   ": get_value failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
	}
	return id;
}

static ber_len_t
entry_getlen(unsigned char **buf)
{
    ber_len_t len;
    int i;

    len = *(*buf)++;
    if (len <= 0x7f)
        return len;
    i = len & 0x7f;
    len = 0;
    for (;i > 0; i--) {
        len <<= 8;
        len |= *(*buf)++;
    }
    return len;
}

int wt_entry_header(WT_ITEM *item, EntryHeader *eh){
	unsigned char *ptr = (unsigned char *)item->data;

    /* Some overlays can create empty entries
     * so don't check for zeros here.
     */
	eh->nattrs = entry_getlen(&ptr);
    eh->nvals = entry_getlen(&ptr);
    eh->data = (char *)ptr;
	return LDAP_SUCCESS;
}

Entry *
wt_tool_entry_get( BackendDB *be, ID id )
{
	Entry *e = NULL;
	static EntryHeader eh;
	int rc, eoff;

	assert( be != NULL );
	assert( slapMode & SLAP_TOOL_MODE );

	rc = wt_entry_header( &item,  &eh );
	assert( rc == 0 );
	eoff = eh.data - (char *)item.data;

	eh.bv.bv_len = eh.nvals * sizeof( struct berval ) + item.size;
	eh.bv.bv_val = ch_realloc( eh.bv.bv_val, eh.bv.bv_len );
    memset(eh.bv.bv_val, 0xff, eh.bv.bv_len);
	eh.data = eh.bv.bv_val + eh.nvals * sizeof( struct berval );
    memcpy(eh.data, item.data, item.size);
    eh.data += eoff;

	rc = entry_decode( &eh, &e );
	assert( rc == 0 );

	if( rc == LDAP_SUCCESS ) {
		e->e_id = id;
	}

	return e;
}

static int wt_tool_next_id(
    Operation *op,
    Entry *e,
    struct berval *text,
    int hole )
{
    struct wt_info *wi = (struct wt_info *) op->o_bd->be_private;
	struct berval dn = e->e_name;
	struct berval ndn = e->e_nname;
	struct berval pdn, npdn;
	int rc;
	ID id = 0;
	ID pid = 0;

    if(ndn.bv_len == 0){
        e->e_id = 0;
        return 0;
    }

	rc = wt_dn2id(op, wc->session, &ndn, &id);
	if(rc == 0){
		e->e_id = id;
	}else if( rc == WT_NOTFOUND ){
		if ( !be_issuffix( op->o_bd, &ndn ) ) {
			ID eid = e->e_id;
			dnParent( &dn, &pdn );
			dnParent( &ndn, &npdn );
			e->e_name = pdn;
			e->e_nname = npdn;
			rc = wt_tool_next_id( op, e, text, 1 );
			e->e_name = dn;
			e->e_nname = ndn;
			if ( rc ) {
				return rc;
			}
			/* If parent didn't exist, it was created just now
			 * and its ID is now in e->e_id. Make sure the current
			 * entry gets added under the new parent ID.
			 */
			if ( eid != e->e_id ) {
				pid = e->e_id;
			}
		}else{
			pid = id;
		}
		wt_next_id( op->o_bd, &e->e_id );
		rc = wt_dn2id_add(op, wc->session, pid, e);
		if( rc ){
			snprintf( text->bv_val, text->bv_len,
					  "wt_dn2id_add failed: %s (%d)",
					  wiredtiger_strerror(rc), rc );
			Debug( LDAP_DEBUG_ANY,
				   "=> wt_tool_next_id: %s\n", text->bv_val );
		}

	}else if ( !hole ) {
		unsigned i, j;
		e->e_id = id;

		for ( i=0; i<nholes; i++) {
			if ( holes[i].id == e->e_id ) {
				free(holes[i].dn.bv_val);
				for (j=i;j<nholes;j++) holes[j] = holes[j+1];
				holes[j].id = 0;
				nholes--;
				break;
			} else if ( holes[i].id > e->e_id ) {
				break;
			}
		}
	}
    return rc;
}

static int
wt_tool_index_add(
    Operation *op,
    wt_ctx *wc,
    Entry *e )
{
	return wt_index_entry_add( op, wc, e );
}

ID
wt_tool_entry_put( BackendDB *be, Entry *e, struct berval *text )
{
    struct wt_info *wi = (struct wt_info *) be->be_private;
    int rc;

    Operation op = {0};
    Opheader ohdr = {0};

	assert( slapMode & SLAP_TOOL_MODE );
	assert( text != NULL );
	assert( text->bv_val != NULL );
	assert( text->bv_val[0] == '\0' ); /* overconservative? */

    Debug( LDAP_DEBUG_TRACE,
		   "=> " LDAP_XSTRING(wt_tool_entry_put)
		   ": ( \"%s\" )\n", e->e_dn );

    rc = wc->session->begin_transaction(wc->session, NULL);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_add)
			   ": begin_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		return NOID;
	}

	op.o_hdr = &ohdr;
    op.o_bd = be;
    op.o_tmpmemctx = NULL;
    op.o_tmpmfuncs = &ch_mfuncs;

    rc = wt_tool_next_id( &op, e, text, 0 );
	if( rc != 0 ) {
        snprintf( text->bv_val, text->bv_len,
				  "wt_tool_next_id failed: %s (%d)",
				  wiredtiger_strerror(rc), rc );
        Debug( LDAP_DEBUG_ANY,
			   "=> " LDAP_XSTRING(wt_tool_entry_put) ": %s\n",
			   text->bv_val );
		goto done;
	}

	rc = wt_id2entry_add( &op, wc->session, e );
	if( rc != 0 ) {
        snprintf( text->bv_val, text->bv_len,
				  "id2entry_add failed: %s (%d)",
				  wiredtiger_strerror(rc), rc );
        Debug( LDAP_DEBUG_ANY,
			   "=> " LDAP_XSTRING(wt_tool_entry_put) ": %s\n",
			   text->bv_val );
        goto done;
    }

	rc = wt_tool_index_add( &op, wc, e );
    if( rc != 0 ) {
        snprintf( text->bv_val, text->bv_len,
				  "index_entry_add failed: %s (%d)",
				  rc == LDAP_OTHER ? "Internal error" :
				  wiredtiger_strerror(rc), rc );
        Debug( LDAP_DEBUG_ANY,
			   "=> " LDAP_XSTRING(wt_tool_entry_put) ": %s\n",
			   text->bv_val );
        goto done;
    }

done:
	if ( rc == 0 ){
		rc = wc->session->commit_transaction(wc->session, NULL);
		if( rc != 0 ) {
			snprintf( text->bv_val, text->bv_len,
					  "txn_commit failed: %s (%d)",
					  wiredtiger_strerror(rc), rc );
			Debug( LDAP_DEBUG_ANY,
				   "=> " LDAP_XSTRING(wt_tool_entry_put) ": %s\n",
				   text->bv_val );
            e->e_id = NOID;
		}
	}else{
		rc = wc->session->rollback_transaction(wc->session, NULL);
		snprintf( text->bv_val, text->bv_len,
				  "txn_aborted! %s (%d)",
				  rc == LDAP_OTHER ? "Internal error" :
				  wiredtiger_strerror(rc), rc );
        Debug( LDAP_DEBUG_ANY,
			   "=> " LDAP_XSTRING(wt_tool_entry_put) ": %s\n",
			   text->bv_val );
        e->e_id = NOID;
	}

	return e->e_id;
}

int wt_tool_entry_reindex(
	BackendDB *be,
	ID id,
	AttributeDescription **adv )
{
	struct wt_info *wi = (struct wt_info *) be->be_private;
	int rc;
	Entry *e;
	Operation op = {0};
	Opheader ohdr = {0};

	Debug( LDAP_DEBUG_ARGS,
		   "=> " LDAP_XSTRING(wt_tool_entry_reindex) "( %ld )\n",
		   (long) id );
	assert( tool_base == NULL );
	assert( tool_filter == NULL );

	/* No indexes configured, nothing to do. Could return an
     * error here to shortcut things.
     */
	if (!wi->wi_attrs) {
		return 0;
	}

	/* Check for explicit list of attrs to index */
	if ( adv ) {
		int i, j, n;

		if ( wi->wi_attrs[0]->ai_desc != adv[0] ) {
			/* count */
			for ( n = 0; adv[n]; n++ ) ;

			/* insertion sort */
			for ( i = 0; i < n; i++ ) {
				AttributeDescription *ad = adv[i];
				for ( j = i-1; j>=0; j--) {
					if ( SLAP_PTRCMP( adv[j], ad ) <= 0 ) break;
					adv[j+1] = adv[j];
				}
				adv[j+1] = ad;
			}
		}

		for ( i = 0; adv[i]; i++ ) {
			if ( wi->wi_attrs[i]->ai_desc != adv[i] ) {
				for ( j = i+1; j < wi->wi_nattrs; j++ ) {
					if ( wi->wi_attrs[j]->ai_desc == adv[i] ) {
						AttrInfo *ai = wi->wi_attrs[i];
						wi->wi_attrs[i] = wi->wi_attrs[j];
						wi->wi_attrs[j] = ai;
						break;
					}
				}
				if ( j == wi->wi_nattrs ) {
					Debug( LDAP_DEBUG_ANY,
						   LDAP_XSTRING(wt_tool_entry_reindex)
						   ": no index configured for %s\n",
						   adv[i]->ad_cname.bv_val );
					return -1;
				}
			}
		}
		wi->wi_nattrs = i;
	}

	e = wt_tool_entry_get( be, id );

	if( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_tool_entry_reindex)
			   ": could not locate id=%ld\n",
			   (long) id );
		return -1;
	}

	op.o_hdr = &ohdr;
	op.o_bd = be;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;

	rc = wc->session->begin_transaction(wc->session, NULL);
	if( rc ){
		Debug( LDAP_DEBUG_ANY,
			   LDAP_XSTRING(wt_dn2id_add)
			   ": begin_transaction failed: %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
		goto done;
	}
	Debug( LDAP_DEBUG_TRACE,
		   "=> " LDAP_XSTRING(wt_tool_entry_reindex) "( %ld, \"%s\" )\n",
		   (long) id, e->e_dn );

	rc = wt_tool_index_add( &op, wc, e );

done:
	if ( rc == 0 ){
		rc = wc->session->commit_transaction(wc->session, NULL);
		if( rc ) {
			Debug( LDAP_DEBUG_ANY,
				   "=> " LDAP_XSTRING(wt_tool_entry_reindex)
				   "commit_transaction failed: %s (%d)\n",
				   wiredtiger_strerror(rc), rc );
		}
	}else{
		rc = wc->session->rollback_transaction(wc->session, NULL);
		Debug( LDAP_DEBUG_ANY,
			   "=> " LDAP_XSTRING(wt_tool_entry_reindex)
			   ": rollback transaction %s (%d)\n",
			   wiredtiger_strerror(rc), rc );
	}

	wt_entry_release( &op, e, 0 );

	return rc;
}

/*
 * Local variables:
 * indent-tabs-mode: t
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
