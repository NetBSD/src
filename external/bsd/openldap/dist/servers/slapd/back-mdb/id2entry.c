/*	$NetBSD: id2entry.c,v 1.3 2021/08/14 16:15:00 christos Exp $	*/

/* id2entry.c - routines to deal with the id2entry database */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2021 The OpenLDAP Foundation.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: id2entry.c,v 1.3 2021/08/14 16:15:00 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/errno.h>

#include "back-mdb.h"

typedef struct Ecount {
	ber_len_t len;	/* total entry size */
	ber_len_t dlen;	/* contiguous data size */
	int nattrs;
	int nvals;
	int offset;
	Attribute *multi;
} Ecount;

static int mdb_entry_partsize(struct mdb_info *mdb, MDB_txn *txn, Entry *e,
	Ecount *eh);
static int mdb_entry_encode(Operation *op, Entry *e, MDB_val *data,
	Ecount *ec);
static Entry *mdb_entry_alloc( Operation *op, int nattrs, int nvals );

#define ID2VKSZ	(sizeof(ID)+2)

int
mdb_id2v_compare(
	const MDB_val *usrkey,
	const MDB_val *curkey
)
{
	unsigned short *uv, *cv;
	ID ui, ci;
	int rc;

	memcpy(&ui, usrkey->mv_data, sizeof(ID));
	memcpy(&ci, curkey->mv_data, sizeof(ID));
	if (ui < ci)
		return -1;
	if (ui > ci)
		return 1;
	uv = usrkey->mv_data;
	cv = curkey->mv_data;
	return uv[sizeof(ID)/2] - cv[sizeof(ID)/2];
}

/* usrkey[0] is the key in DB format, as described at mdb_mval_put.
 * usrkey[1] is the value we'll actually match against.
 * usrkey[2] is the attributeDescription for this value.
 */
int
mdb_id2v_dupsort(
	const MDB_val *usrkey,
	const MDB_val *curkey
)
{
	AttributeDescription *ad = usrkey[2].mv_data;
	struct berval bv1, bv2;
	int rc, match, olen;
	unsigned short s;
	char *ptr;

	ptr = curkey->mv_data + curkey->mv_size - 2;
	memcpy(&s, ptr, 2);
	bv2.bv_val = curkey->mv_data;
	bv2.bv_len = curkey->mv_size - 3;
	if (s)
		bv2.bv_len -= (s+1);

	bv1.bv_val = usrkey[1].mv_data;
	bv1.bv_len = usrkey[1].mv_size;

	if (ad) {
		MatchingRule *mr = ad->ad_type->sat_equality;
		rc = mr->smr_match(&match, SLAP_MR_EQUALITY
		| SLAP_MR_VALUE_OF_ASSERTION_SYNTAX
		| SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH
		| SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH,
		ad->ad_type->sat_syntax, mr, &bv1, &bv2);
	} else {
		match = ber_bvcmp(&bv1, &bv2);
	}

	return match;
}

/* Values are stored as
 * [normalized-value NUL ] original-value NUL 2-byte-len
 * The trailing 2-byte-len is zero if there is no normalized value.
 * Otherwise, it is the length of the original-value.
 */
int mdb_mval_put(Operation *op, MDB_cursor *mc, ID id, Attribute *a)
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_val key, data[3];
	char *buf;
	char ivk[ID2VKSZ];
	unsigned i;
	unsigned short s;
	int rc, len;

	memcpy(ivk, &id, sizeof(id));
	s = mdb->mi_adxs[a->a_desc->ad_index];
	memcpy(ivk+sizeof(ID), &s, 2);
	key.mv_data = &ivk;
	key.mv_size = sizeof(ivk);
	if ((a->a_desc->ad_type->sat_flags & SLAP_AT_ORDERED) || a->a_desc == slap_schema.si_ad_objectClass)
		data[2].mv_data = NULL;
	else
		data[2].mv_data = a->a_desc;

	for (i=0; i<a->a_numvals; i++) {
		len = a->a_nvals[i].bv_len + 1 + 2;
		if (a->a_nvals != a->a_vals) {
			len += a->a_vals[i].bv_len + 1;
			data[1].mv_data = a->a_nvals[i].bv_val;
			data[1].mv_size = a->a_nvals[i].bv_len;
		} else {
			data[1].mv_data = a->a_vals[i].bv_val;
			data[1].mv_size = a->a_vals[i].bv_len;
		}
		data[0].mv_size = len;
		buf = op->o_tmpalloc( len, op->o_tmpmemctx );
		data[0].mv_data = buf;
		memcpy(buf, a->a_nvals[i].bv_val, a->a_nvals[i].bv_len);
		buf += a->a_nvals[i].bv_len;
		*buf++ = 0;
		if (a->a_nvals != a->a_vals) {
			s = a->a_vals[i].bv_len;
			memcpy(buf, a->a_vals[i].bv_val, a->a_vals[i].bv_len);
			buf += a->a_vals[i].bv_len;
			*buf++ = 0;
			memcpy(buf, &s, 2);
		} else {
			*buf++ = 0;
			*buf++ = 0;
		}
		rc = mdb_cursor_put(mc, &key, data, 0);
		op->o_tmpfree( data[0].mv_data, op->o_tmpmemctx );
		if (rc)
			return rc;
	}
	return 0;
}

int mdb_mval_del(Operation *op, MDB_cursor *mc, ID id, Attribute *a)
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_val key, data[3];
	char *ptr;
	char ivk[ID2VKSZ];
	unsigned i;
	int rc;
	unsigned short s;

	memcpy(ivk, &id, sizeof(id));
	s = mdb->mi_adxs[a->a_desc->ad_index];
	memcpy(ivk+sizeof(ID), &s, 2);
	key.mv_data = &ivk;
	key.mv_size = sizeof(ivk);
	if ((a->a_desc->ad_type->sat_flags & SLAP_AT_ORDERED) || a->a_desc == slap_schema.si_ad_objectClass)
		data[2].mv_data = NULL;
	else
		data[2].mv_data = a->a_desc;

	if (a->a_numvals) {
		for (i=0; i<a->a_numvals; i++) {
			data[0].mv_data = a->a_nvals[i].bv_val;
			data[0].mv_size = a->a_nvals[i].bv_len+1;
			if (a->a_nvals != a->a_vals) {
				data[1].mv_data = a->a_nvals[i].bv_val;
				data[1].mv_size = a->a_nvals[i].bv_len;
			} else {
				data[1].mv_data = a->a_vals[i].bv_val;
				data[1].mv_size = a->a_vals[i].bv_len;
			}
			rc = mdb_cursor_get(mc, &key, data, MDB_GET_BOTH_RANGE);
			if (rc)
				return rc;
			rc = mdb_cursor_del(mc, 0);
			if (rc)
				return rc;
		}
	} else {
		rc = mdb_cursor_get(mc, &key, data, MDB_SET);
		if (rc)
			return rc;
		rc = mdb_cursor_del(mc, MDB_NODUPDATA);
	}
	return rc;
}

static int mdb_mval_get(Operation *op, MDB_cursor *mc, ID id, Attribute *a, int have_nvals)
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_val key, data[3];
	char *ptr;
	char ivk[ID2VKSZ];
	unsigned i;
	int rc = 0;
	unsigned short s;

	memcpy(ivk, &id, sizeof(id));
	s = mdb->mi_adxs[a->a_desc->ad_index];
	memcpy(ivk+sizeof(ID), &s, 2);
	key.mv_data = &ivk;
	key.mv_size = sizeof(ivk);

	/* not needed */
	if ((a->a_desc->ad_type->sat_flags & SLAP_AT_ORDERED) || a->a_desc == slap_schema.si_ad_objectClass)
		data[2].mv_data = NULL;
	else
		data[2].mv_data = a->a_desc;

	if (have_nvals)
		a->a_nvals = a->a_vals + a->a_numvals + 1;
	else
		a->a_nvals = a->a_vals;
	for (i=0; i<a->a_numvals; i++) {
		if (!i)
			rc = mdb_cursor_get(mc, &key, data, MDB_SET);
		else
			rc = mdb_cursor_get(mc, &key, data, MDB_NEXT_DUP);
		if (rc)
			break;
		ptr = (char*)data[0].mv_data + data[0].mv_size - 2;
		memcpy(&s, ptr, 2);
		if (have_nvals) {
			a->a_nvals[i].bv_val = data[0].mv_data;
			a->a_vals[i].bv_len = s;
			a->a_vals[i].bv_val = ptr - a->a_vals[i].bv_len - 1;
			a->a_nvals[i].bv_len = a->a_vals[i].bv_val - a->a_nvals[i].bv_val - 1;
		} else {
			assert(!s);
			a->a_vals[i].bv_val = data[0].mv_data;
			a->a_vals[i].bv_len = data[0].mv_size - 3;
		}
	}
	a->a_numvals = i;
	BER_BVZERO(&a->a_vals[i]);
	if (have_nvals) {
		BER_BVZERO(&a->a_nvals[i]);
	}
	return rc;
}

#define ADD_FLAGS	(MDB_NOOVERWRITE|MDB_APPEND)

static int mdb_id2entry_put(
	Operation *op,
	MDB_txn *txn,
	MDB_cursor *mc,
	Entry *e,
	int flag )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	Ecount ec;
	MDB_val key, data;
	int rc, adding = flag, prev_ads = mdb->mi_numads;

	/* We only store rdns, and they go in the dn2id database. */

	key.mv_data = &e->e_id;
	key.mv_size = sizeof(ID);

	rc = mdb_entry_partsize( mdb, txn, e, &ec );
	if (rc) {
		rc = LDAP_OTHER;
		goto fail;
	}

	flag |= MDB_RESERVE;

	if (e->e_id < mdb->mi_nextid)
		flag &= ~MDB_APPEND;

	if (mdb->mi_maxentrysize && ec.len > mdb->mi_maxentrysize) {
		rc = LDAP_ADMINLIMIT_EXCEEDED;
		goto fail;
	}

again:
	data.mv_size = ec.dlen;
	if ( mc )
		rc = mdb_cursor_put( mc, &key, &data, flag );
	else
		rc = mdb_put( txn, mdb->mi_id2entry, &key, &data, flag );
	if (rc == MDB_SUCCESS) {
		rc = mdb_entry_encode( op, e, &data, &ec );
		if( rc != LDAP_SUCCESS )
			goto fail;
		/* Handle adds of large multi-valued attrs here.
		 * Modifies handle them directly.
		 */
		if (adding && ec.multi) {
			MDB_cursor *mvc;
			Attribute *a;
			rc = mdb_cursor_open( txn, mdb->mi_dbis[MDB_ID2VAL], &mvc );
			if( !rc ) {
				for ( a = ec.multi; a; a=a->a_next ) {
					if (!(a->a_flags & SLAP_ATTR_BIG_MULTI))
						continue;
					rc = mdb_mval_put( op, mvc, e->e_id, a );
					if( rc )
						break;
				}
				mdb_cursor_close( mvc );
			}
			if ( rc ) {
				Debug( LDAP_DEBUG_ANY,
					"mdb_id2entry_put: mdb_mval_put failed: %s(%d) \"%s\"\n",
					mdb_strerror(rc), rc,
					e->e_nname.bv_val );
				rc = LDAP_OTHER;
				goto fail;
			}
		}
	}
	if (rc) {
		/* Was there a hole from slapadd? */
		if ( (flag & MDB_NOOVERWRITE) && data.mv_size == 0 ) {
			flag &= ~ADD_FLAGS;
			goto again;
		}
		Debug( LDAP_DEBUG_ANY,
			"mdb_id2entry_put: mdb_put failed: %s(%d) \"%s\"\n",
			mdb_strerror(rc), rc,
			e->e_nname.bv_val );
		if ( rc != MDB_KEYEXIST )
			rc = LDAP_OTHER;
	}
fail:
	if (rc) {
		mdb_ad_unwind( mdb, prev_ads );
	}
	return rc;
}

/*
 * This routine adds (or updates) an entry on disk.
 */
int mdb_id2entry_add(
	Operation *op,
	MDB_txn *txn,
	MDB_cursor *mc,
	Entry *e )
{
	return mdb_id2entry_put(op, txn, mc, e, ADD_FLAGS);
}

int mdb_id2entry_update(
	Operation *op,
	MDB_txn *txn,
	MDB_cursor *mc,
	Entry *e )
{
	return mdb_id2entry_put(op, txn, mc, e, 0);
}

int mdb_id2edata(
	Operation *op,
	MDB_cursor *mc,
	ID id,
	MDB_val *data )
{
	MDB_val key;
	int rc;

	key.mv_data = &id;
	key.mv_size = sizeof(ID);

	/* fetch it */
	rc = mdb_cursor_get( mc, &key, data, MDB_SET );
	/* stubs from missing parents - DB is actually invalid */
	if ( rc == MDB_SUCCESS && !data->mv_size )
		rc = MDB_NOTFOUND;
	return rc;
}

int mdb_id2entry(
	Operation *op,
	MDB_cursor *mc,
	ID id,
	Entry **e )
{
	MDB_val key, data;
	int rc = 0;

	*e = NULL;

	key.mv_data = &id;
	key.mv_size = sizeof(ID);

	/* fetch it */
	rc = mdb_cursor_get( mc, &key, &data, MDB_SET );
	if ( rc == MDB_NOTFOUND ) {
		/* Looking for root entry on an empty-dn suffix? */
		if ( !id && BER_BVISEMPTY( &op->o_bd->be_nsuffix[0] )) {
			struct berval gluebv = BER_BVC("glue");
			Entry *r = mdb_entry_alloc(op, 2, 4);
			Attribute *a = r->e_attrs;
			struct berval *bptr;

			r->e_id = 0;
			r->e_ocflags = SLAP_OC_GLUE|SLAP_OC__END;
			bptr = a->a_vals;
			a->a_flags = SLAP_ATTR_DONT_FREE_DATA | SLAP_ATTR_DONT_FREE_VALS;
			a->a_desc = slap_schema.si_ad_objectClass;
			a->a_nvals = a->a_vals;
			a->a_numvals = 1;
			*bptr++ = gluebv;
			BER_BVZERO(bptr);
			bptr++;
			a->a_next = a+1;
			a = a->a_next;
			a->a_flags = SLAP_ATTR_DONT_FREE_DATA | SLAP_ATTR_DONT_FREE_VALS;
			a->a_desc = slap_schema.si_ad_structuralObjectClass;
			a->a_vals = bptr;
			a->a_nvals = a->a_vals;
			a->a_numvals = 1;
			*bptr++ = gluebv;
			BER_BVZERO(bptr);
			a->a_next = NULL;
			*e = r;
			return MDB_SUCCESS;
		}
	}
	/* stubs from missing parents - DB is actually invalid */
	if ( rc == MDB_SUCCESS && !data.mv_size )
		rc = MDB_NOTFOUND;
	if ( rc ) return rc;

	rc = mdb_entry_decode( op, mdb_cursor_txn( mc ), &data, id, e );
	if ( rc ) return rc;

	(*e)->e_id = id;
	(*e)->e_name.bv_val = NULL;
	(*e)->e_nname.bv_val = NULL;

	return rc;
}

int mdb_id2entry_delete(
	BackendDB *be,
	MDB_txn *tid,
	Entry *e )
{
	struct mdb_info *mdb = (struct mdb_info *) be->be_private;
	MDB_dbi dbi = mdb->mi_id2entry;
	MDB_val key;
	MDB_cursor *mvc;
	int rc;

	key.mv_data = &e->e_id;
	key.mv_size = sizeof(ID);

	/* delete from database */
	rc = mdb_del( tid, dbi, &key, NULL );
	if (rc)
		return rc;
	rc = mdb_cursor_open( tid, mdb->mi_dbis[MDB_ID2VAL], &mvc );
	if (rc)
		return rc;

	rc = mdb_cursor_get( mvc, &key, NULL, MDB_SET_RANGE );
	if (rc) {
		if (rc == MDB_NOTFOUND)
			rc = MDB_SUCCESS;
		return rc;
	}
	while (*(ID *)key.mv_data == e->e_id ) {
		rc = mdb_cursor_del( mvc, MDB_NODUPDATA );
		if (rc)
			return rc;
		rc = mdb_cursor_get( mvc, &key, NULL, MDB_GET_CURRENT );
		if (rc) {
			if (rc == MDB_NOTFOUND)
				rc = MDB_SUCCESS;
			break;
		}
	}
	return rc;
}

static Entry * mdb_entry_alloc(
	Operation *op,
	int nattrs,
	int nvals )
{
	Entry *e = op->o_tmpalloc( sizeof(Entry) +
		nattrs * sizeof(Attribute) +
		nvals * sizeof(struct berval), op->o_tmpmemctx );
	BER_BVZERO(&e->e_bv);
	e->e_private = e;
	if (nattrs) {
		e->e_attrs = (Attribute *)(e+1);
		e->e_attrs->a_vals = (struct berval *)(e->e_attrs+nattrs);
	} else {
		e->e_attrs = NULL;
	}

	return e;
}

int mdb_entry_return(
	Operation *op,
	Entry *e
)
{
	if ( !e )
		return 0;
	if ( e->e_private ) {
		if ( op->o_hdr && op->o_tmpmfuncs ) {
			op->o_tmpfree( e->e_nname.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( e->e_name.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( e, op->o_tmpmemctx );
		} else {
			ch_free( e->e_nname.bv_val );
			ch_free( e->e_name.bv_val );
			ch_free( e );
		}
	} else {
		entry_free( e );
	}
	return 0;
}

int mdb_entry_release(
	Operation *op,
	Entry *e,
	int rw )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	struct mdb_op_info *moi = NULL;
 
	/* slapMode : SLAP_SERVER_MODE, SLAP_TOOL_MODE,
			SLAP_TRUNCATE_MODE, SLAP_UNDEFINED_MODE */
 
	int release = 1;
	if ( slapMode & SLAP_SERVER_MODE ) {
		OpExtra *oex;
		LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
			release = 0;
			if ( oex->oe_key == mdb ) {
				mdb_entry_return( op, e );
				moi = (mdb_op_info *)oex;
				/* If it was setup by entry_get we should probably free it */
				if (( moi->moi_flag & (MOI_FREEIT|MOI_KEEPER)) == MOI_FREEIT ) {
					moi->moi_ref--;
					if ( moi->moi_ref < 1 ) {
						mdb_txn_reset( moi->moi_txn );
						moi->moi_ref = 0;
						LDAP_SLIST_REMOVE( &op->o_extra, &moi->moi_oe, OpExtra, oe_next );
						op->o_tmpfree( moi, op->o_tmpmemctx );
					}
				}
				break;
			}
		}
	}

	if (release)
		mdb_entry_return( op, e );
 
	return 0;
}

/* return LDAP_SUCCESS IFF we can retrieve the specified entry.
 */
int mdb_entry_get(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **ent )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	struct mdb_op_info *moi = NULL;
	MDB_txn *txn = NULL;
	Entry *e = NULL;
	int	rc;
	const char *at_name = at ? at->ad_cname.bv_val : "(null)";

	Debug( LDAP_DEBUG_ARGS,
		"=> mdb_entry_get: ndn: \"%s\"\n", ndn->bv_val );
	Debug( LDAP_DEBUG_ARGS,
		"=> mdb_entry_get: oc: \"%s\", at: \"%s\"\n",
		oc ? oc->soc_cname.bv_val : "(null)", at_name );

	rc = mdb_opinfo_get( op, mdb, rw == 0, &moi );
	if ( rc )
		return LDAP_OTHER;
	txn = moi->moi_txn;

	/* can we find entry */
	rc = mdb_dn2entry( op, txn, NULL, ndn, &e, NULL, 0 );
	switch( rc ) {
	case MDB_NOTFOUND:
	case 0:
		break;
	default:
		return (rc != LDAP_BUSY) ? LDAP_OTHER : LDAP_BUSY;
	}
	if (e == NULL) {
		Debug( LDAP_DEBUG_ACL,
			"=> mdb_entry_get: cannot find entry: \"%s\"\n",
				ndn->bv_val );
		rc = LDAP_NO_SUCH_OBJECT;
		goto return_results;
	}
	
	Debug( LDAP_DEBUG_ACL,
		"=> mdb_entry_get: found entry: \"%s\"\n",
		ndn->bv_val );

	if ( oc && !is_entry_objectclass( e, oc, 0 )) {
		Debug( LDAP_DEBUG_ACL,
			"<= mdb_entry_get: failed to find objectClass %s\n",
			oc->soc_cname.bv_val );
		rc = LDAP_NO_SUCH_ATTRIBUTE;
		goto return_results;
	}

	/* NOTE: attr_find() or attrs_find()? */
	if ( at && attr_find( e->e_attrs, at ) == NULL ) {
		Debug( LDAP_DEBUG_ACL,
			"<= mdb_entry_get: failed to find attribute %s\n",
			at->ad_cname.bv_val );
		rc = LDAP_NO_SUCH_ATTRIBUTE;
		goto return_results;
	}

return_results:
	if( rc != LDAP_SUCCESS ) {
		/* free entry */
		mdb_entry_release( op, e, rw );
	} else {
		*ent = e;
	}

	Debug( LDAP_DEBUG_TRACE,
		"mdb_entry_get: rc=%d\n",
		rc );
	return(rc);
}

static void
mdb_reader_free( void *key, void *data )
{
	MDB_txn *txn = data;

	if ( txn ) mdb_txn_abort( txn );
}

/* free up any keys used by the main thread */
void
mdb_reader_flush( MDB_env *env )
{
	void *data;
	void *ctx = ldap_pvt_thread_pool_context();

	if ( !ldap_pvt_thread_pool_getkey( ctx, env, &data, NULL ) ) {
		ldap_pvt_thread_pool_setkey( ctx, env, NULL, 0, NULL, NULL );
		mdb_reader_free( env, data );
	}
}

extern MDB_txn *mdb_tool_txn;

int
mdb_opinfo_get( Operation *op, struct mdb_info *mdb, int rdonly, mdb_op_info **moip )
{
	int rc, renew = 0;
	void *data;
	void *ctx;
	mdb_op_info *moi = NULL;
	OpExtra *oex;

	assert( op != NULL );

	if ( !mdb || !moip ) return -1;

	/* If no op was provided, try to find the ctx anyway... */
	if ( op ) {
		ctx = op->o_threadctx;
	} else {
		ctx = ldap_pvt_thread_pool_context();
	}

	if ( op ) {
		LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
			if ( oex->oe_key == mdb ) break;
		}
		moi = (mdb_op_info *)oex;
	}

	if ( !moi ) {
		moi = *moip;

		if ( !moi ) {
			if ( op ) {
				moi = op->o_tmpalloc(sizeof(struct mdb_op_info),op->o_tmpmemctx);
			} else {
				moi = ch_malloc(sizeof(mdb_op_info));
			}
			moi->moi_flag = MOI_FREEIT;
			*moip = moi;
		}
		LDAP_SLIST_INSERT_HEAD( &op->o_extra, &moi->moi_oe, oe_next );
		moi->moi_oe.oe_key = mdb;
		moi->moi_ref = 0;
		moi->moi_txn = NULL;
	}

	if ( !rdonly ) {
		/* This op started as a reader, but now wants to write. */
		if ( moi->moi_flag & MOI_READER ) {
			moi = *moip;
			LDAP_SLIST_INSERT_HEAD( &op->o_extra, &moi->moi_oe, oe_next );
		} else {
		/* This op is continuing an existing write txn */
			*moip = moi;
		}
		moi->moi_ref++;
		if ( !moi->moi_txn ) {
			if (( slapMode & SLAP_TOOL_MODE ) && mdb_tool_txn ) {
				moi->moi_txn = mdb_tool_txn;
			} else {
				int flag = 0;
#ifdef SLAP_CONTROL_X_LAZY_COMMIT
				if ( get_lazyCommit( op ))
					flag |= MDB_NOMETASYNC;
#endif
				rc = mdb_txn_begin( mdb->mi_dbenv, NULL, flag, &moi->moi_txn );
				if (rc) {
					Debug( LDAP_DEBUG_ANY, "mdb_opinfo_get: err %s(%d)\n",
						mdb_strerror(rc), rc );
				}
				return rc;
			}
		}
		return 0;
	}

	/* OK, this is a reader */
	if ( !moi->moi_txn ) {
		if (( slapMode & SLAP_TOOL_MODE ) && mdb_tool_txn ) {
			moi->moi_txn = mdb_tool_txn;
			goto ok;
		}
		if ( !ctx ) {
			/* Shouldn't happen unless we're single-threaded */
			rc = mdb_txn_begin( mdb->mi_dbenv, NULL, MDB_RDONLY, &moi->moi_txn );
			if (rc) {
				Debug( LDAP_DEBUG_ANY, "mdb_opinfo_get: err %s(%d)\n",
					mdb_strerror(rc), rc );
			}
			return rc;
		}
		if ( ldap_pvt_thread_pool_getkey( ctx, mdb->mi_dbenv, &data, NULL ) ) {
			rc = mdb_txn_begin( mdb->mi_dbenv, NULL, MDB_RDONLY, &moi->moi_txn );
			if (rc) {
				Debug( LDAP_DEBUG_ANY, "mdb_opinfo_get: err %s(%d)\n",
					mdb_strerror(rc), rc );
				return rc;
			}
			data = moi->moi_txn;
			if ( ( rc = ldap_pvt_thread_pool_setkey( ctx, mdb->mi_dbenv,
				data, mdb_reader_free, NULL, NULL ) ) ) {
				mdb_txn_abort( moi->moi_txn );
				moi->moi_txn = NULL;
				Debug( LDAP_DEBUG_ANY, "mdb_opinfo_get: thread_pool_setkey failed err (%d)\n",
					rc );
				return rc;
			}
		} else {
			moi->moi_txn = data;
			renew = 1;
		}
		moi->moi_flag |= MOI_READER;
	}
ok:
	if ( moi->moi_ref < 1 ) {
		moi->moi_ref = 0;
	}
	if ( renew ) {
		rc = mdb_txn_renew( moi->moi_txn );
		assert(!rc);
	}
	moi->moi_ref++;
	if ( *moip != moi )
		*moip = moi;

	return 0;
}

int mdb_txn( Operation *op, int txnop, OpExtra **ptr )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	mdb_op_info **moip = (mdb_op_info **)ptr, *moi = *moip;
	int rc;

	switch( txnop ) {
	case SLAP_TXN_BEGIN:
		rc = mdb_opinfo_get( op, mdb, 0, moip );
		if ( !rc ) {
			moi = *moip;
			moi->moi_flag |= MOI_KEEPER;
		}
		return rc;
	case SLAP_TXN_COMMIT:
		rc = mdb_txn_commit( moi->moi_txn );
		if ( rc )
			mdb->mi_numads = 0;
		op->o_tmpfree( moi, op->o_tmpmemctx );
		return rc;
	case SLAP_TXN_ABORT:
		mdb->mi_numads = 0;
		mdb_txn_abort( moi->moi_txn );
		op->o_tmpfree( moi, op->o_tmpmemctx );
		return 0;
	}
	return LDAP_OTHER;
}

/* Count up the sizes of the components of an entry */
static int mdb_entry_partsize(struct mdb_info *mdb, MDB_txn *txn, Entry *e,
	Ecount *eh)
{
	ber_len_t len, dlen;
	int i, nat = 0, nval = 0, nnval = 0, doff = 0;
	Attribute *a;
	unsigned hi;

	eh->multi = NULL;
	len = 4*sizeof(int);	/* nattrs, nvals, ocflags, offset */
	dlen = len;
	for (a=e->e_attrs; a; a=a->a_next) {
		/* For AttributeDesc, we only store the attr index */
		nat++;
		if (a->a_desc->ad_index >= MDB_MAXADS) {
			Debug( LDAP_DEBUG_ANY, "mdb_entry_partsize: too many AttributeDescriptions used\n" );
			return LDAP_OTHER;
		}
		if (!mdb->mi_adxs[a->a_desc->ad_index]) {
			int rc = mdb_ad_get(mdb, txn, a->a_desc);
			if (rc)
				return rc;
		}
		len += 2*sizeof(int);	/* AD index, numvals */
		dlen += 2*sizeof(int);
		nval += a->a_numvals + 1;	/* empty berval at end */
		mdb_attr_multi_thresh( mdb, a->a_desc, &hi, NULL );
		if (a->a_numvals > hi)
			a->a_flags |= SLAP_ATTR_BIG_MULTI;
		if (a->a_flags & SLAP_ATTR_BIG_MULTI)
			doff += a->a_numvals;
		for (i=0; i<a->a_numvals; i++) {
			int alen = a->a_vals[i].bv_len + 1 + sizeof(int);	/* len */
			len += alen;
			if (a->a_flags & SLAP_ATTR_BIG_MULTI) {
				if (!eh->multi)
					eh->multi = a;
			} else {
				dlen += alen;
			}
		}
		if (a->a_nvals != a->a_vals) {
			nval += a->a_numvals + 1;
			nnval++;
			if (a->a_flags & SLAP_ATTR_BIG_MULTI)
				doff += a->a_numvals;
			for (i=0; i<a->a_numvals; i++) {
				int alen = a->a_nvals[i].bv_len + 1 + sizeof(int);
				len += alen;
				if (!(a->a_flags & SLAP_ATTR_BIG_MULTI))
					dlen += alen;
			}
		}
	}
	/* padding */
	dlen = (dlen + sizeof(ID)-1) & ~(sizeof(ID)-1);
	eh->len = len;
	eh->dlen = dlen;
	eh->nattrs = nat;
	eh->nvals = nval;
	eh->offset = nat + nval - nnval - doff;
	return 0;
}

/* Flag bits for an encoded attribute */
#define MDB_AT_SORTED	(1U<<(sizeof(unsigned int)*CHAR_BIT-1))
	/* the values are in sorted order */
#define MDB_AT_MULTI	(1<<(sizeof(unsigned int)*CHAR_BIT-2))
	/* the values of this multi-valued attr are stored separately */

#define MDB_AT_NVALS	(1U<<(sizeof(unsigned int)*CHAR_BIT-1))
	/* this attribute has normalized values */

/* Flatten an Entry into a buffer. The buffer starts with the count of the
 * number of attributes in the entry, the total number of values in the
 * entry, and the e_ocflags. It then contains a list of integers for each
 * attribute. For each attribute the first integer gives the index of the
 * matching AttributeDescription, followed by the number of values in the
 * attribute. If the MDB_AT_SORTED bit of the attr index is set, the
 * attribute's values are already sorted. If the MDB_AT_MULTI bit of the
 * attr index is set, the values are stored separately.
 *
 * If the MDB_AT_NVALS bit of numvals is set, the attribute also has
 * normalized values present. (Note - a_numvals is an unsigned int, so this
 * means it's possible to receive an attribute that we can't encode due
 * to size overflow. In practice, this should not be an issue.)
 *
 * Then the length of each value is listed. If there are normalized values,
 * their lengths come next. This continues for each attribute. After all
 * of the lengths for the last attribute, the actual values are copied,
 * with a NUL terminator after each value.
 * The buffer is padded to the sizeof(ID). The entire buffer size is
 * precomputed so that a single malloc can be performed.
 */
static int mdb_entry_encode(Operation *op, Entry *e, MDB_val *data, Ecount *eh)
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	ber_len_t i;
	Attribute *a;
	unsigned char *ptr;
	unsigned int *lp, l;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_entry_encode(0x%08lx): %s\n",
		(long) e->e_id, e->e_dn );

	/* make sure e->e_ocflags is set */
	if (is_entry_referral(e))
		;	/* empty */

	lp = (unsigned int *)data->mv_data;
	*lp++ = eh->nattrs;
	*lp++ = eh->nvals;
	*lp++ = (unsigned int)e->e_ocflags;
	*lp++ = eh->offset;
	ptr = (unsigned char *)(lp + eh->offset);

	for (a=e->e_attrs; a; a=a->a_next) {
		if (!a->a_desc->ad_index)
			return LDAP_UNDEFINED_TYPE;
		l = mdb->mi_adxs[a->a_desc->ad_index];
		if (a->a_flags & SLAP_ATTR_BIG_MULTI)
			l |= MDB_AT_MULTI;
		if (a->a_flags & SLAP_ATTR_SORTED_VALS)
			l |= MDB_AT_SORTED;
		*lp++ = l;
		l = a->a_numvals;
		if (a->a_nvals != a->a_vals)
			l |= MDB_AT_NVALS;
		*lp++ = l;
		if (a->a_flags & SLAP_ATTR_BIG_MULTI) {
			continue;
		} else {
			if (a->a_vals) {
				for (i=0; a->a_vals[i].bv_val; i++);
				assert( i == a->a_numvals );
				for (i=0; i<a->a_numvals; i++) {
					*lp++ = a->a_vals[i].bv_len;
					memcpy(ptr, a->a_vals[i].bv_val,
						a->a_vals[i].bv_len);
					ptr += a->a_vals[i].bv_len;
					*ptr++ = '\0';
				}
				if (a->a_nvals != a->a_vals) {
					for (i=0; i<a->a_numvals; i++) {
						*lp++ = a->a_nvals[i].bv_len;
						memcpy(ptr, a->a_nvals[i].bv_val,
							a->a_nvals[i].bv_len);
						ptr += a->a_nvals[i].bv_len;
						*ptr++ = '\0';
					}
				}
			}
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<= mdb_entry_encode(0x%08lx): %s\n",
		(long) e->e_id, e->e_dn );

	return 0;
}

/* Retrieve an Entry that was stored using entry_encode above.
 *
 * Note: everything is stored in a single contiguous block, so
 * you can not free individual attributes or names from this
 * structure. Attempting to do so will likely corrupt memory.
 */

int mdb_entry_decode(Operation *op, MDB_txn *txn, MDB_val *data, ID id, Entry **e)
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	int i, j, nattrs, nvals;
	int rc;
	Attribute *a;
	Entry *x;
	const char *text;
	unsigned int *lp = (unsigned int *)data->mv_data;
	unsigned char *ptr;
	BerVarray bptr;
	MDB_cursor *mvc = NULL;

	Debug( LDAP_DEBUG_TRACE,
		"=> mdb_entry_decode:\n" );

	nattrs = *lp++;
	nvals = *lp++;
	x = mdb_entry_alloc(op, nattrs, nvals);
	x->e_ocflags = *lp++;
	if (!nvals) {
		goto done;
	}
	a = x->e_attrs;
	bptr = a->a_vals;
	i = *lp++;
	ptr = (unsigned char *)(lp + i);

	for (;nattrs>0; nattrs--) {
		int have_nval = 0, multi = 0;
		a->a_flags = SLAP_ATTR_DONT_FREE_DATA | SLAP_ATTR_DONT_FREE_VALS;
		i = *lp++;
		if (i & MDB_AT_SORTED) {
			i ^= MDB_AT_SORTED;
			a->a_flags |= SLAP_ATTR_SORTED_VALS;
		}
		if (i & MDB_AT_MULTI) {
			i ^= MDB_AT_MULTI;
			a->a_flags |= SLAP_ATTR_BIG_MULTI;
			multi = 1;
		}
		if (i > mdb->mi_numads) {
			rc = mdb_ad_read(mdb, txn);
			if (rc)
				goto leave;
			if (i > mdb->mi_numads) {
				Debug( LDAP_DEBUG_ANY,
					"mdb_entry_decode: attribute index %d not recognized\n",
					i );
				rc = LDAP_OTHER;
				goto leave;
			}
		}
		a->a_desc = mdb->mi_ads[i];
		a->a_numvals = *lp++;
		if (a->a_numvals & MDB_AT_NVALS) {
			a->a_numvals ^= MDB_AT_NVALS;
			have_nval = 1;
		}
		a->a_vals = bptr;
		if (multi) {
			if (!mvc) {
				rc = mdb_cursor_open(txn, mdb->mi_dbis[MDB_ID2VAL], &mvc);
				if (rc)
					goto leave;
			}
			i = a->a_numvals;
			mdb_mval_get(op, mvc, id, a, have_nval);
			bptr += i + 1;
			if (have_nval)
				bptr += i + 1;
		} else {
			for (i=0; i<a->a_numvals; i++) {
				bptr->bv_len = *lp++;
				bptr->bv_val = (char *)ptr;
				ptr += bptr->bv_len+1;
				bptr++;
			}
			bptr->bv_val = NULL;
			bptr->bv_len = 0;
			bptr++;

			if (have_nval) {
				a->a_nvals = bptr;
				for (i=0; i<a->a_numvals; i++) {
					bptr->bv_len = *lp++;
					bptr->bv_val = (char *)ptr;
					ptr += bptr->bv_len+1;
					bptr++;
				}
				bptr->bv_val = NULL;
				bptr->bv_len = 0;
				bptr++;
			} else {
				a->a_nvals = a->a_vals;
			}
		}

		/* FIXME: This is redundant once a sorted entry is saved into the DB */
		if (( a->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL )
			&& !(a->a_flags & SLAP_ATTR_SORTED_VALS)) {
			rc = slap_sort_vals( (Modifications *)a, &text, &j, NULL );
			if ( rc == LDAP_SUCCESS ) {
				a->a_flags |= SLAP_ATTR_SORTED_VALS;
			} else if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
				/* should never happen */
				Debug( LDAP_DEBUG_ANY,
					"mdb_entry_decode: attributeType %s value #%d provided more than once\n",
					a->a_desc->ad_cname.bv_val, j );
				goto leave;
			}
		}
		a->a_next = a+1;
		a = a->a_next;
	}
	a[-1].a_next = NULL;
done:
	Debug(LDAP_DEBUG_TRACE, "<= mdb_entry_decode\n" );
	*e = x;
	rc = 0;

leave:
	if (mvc)
		mdb_cursor_close(mvc);
	return rc;
}
