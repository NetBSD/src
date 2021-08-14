/*	$NetBSD: datamorph.c,v 1.2 2021/08/14 16:14:51 christos Exp $	*/

/* datamorph.c - enumerated and native integer value support */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2021 The OpenLDAP Foundation.
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
 * This work was developed in 2016 by Ondřej Kuzník for Symas Corp.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: datamorph.c,v 1.2 2021/08/14 16:14:51 christos Exp $");

#include "portable.h"

#ifdef SLAPD_OVER_DATAMORPH

#include <inttypes.h>
#include <ac/stdlib.h>

#if defined(__linux__)
#include <endian.h>

#elif defined(sun)

#define be16toh(x) BE_16(x)
#define le16toh(x) LE_16(x)
#define htobe16(x) BE_16(x)
#define htole16(x) LE_16(x)

#define be32toh(x) BE_32(x)
#define le32toh(x) LE_32(x)
#define htobe32(x) BE_32(x)
#define htole32(x) LE_32(x)

#define be64toh(x) BE_64(x)
#define le64toh(x) LE_64(x)
#define htobe64(x) BE_64(x)
#define htole64(x) LE_64(x)

#elif defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/endian.h>

#elif defined(__OpenBSD__)
#include <sys/endian.h>

#define be16toh(x) betoh16(x)
#define le16toh(x) letoh16(x)

#define be32toh(x) betoh32(x)
#define le32toh(x) letoh32(x)

#define be64toh(x) betoh64(x)
#define le64toh(x) letoh64(x)

#elif defined(__BYTE_ORDER__) && \
		( defined(__GNUC__) || defined(__clang__) )

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define be16toh(x) __builtin_bswap16(x)
#define le16toh(x) (x)
#define htobe16(x) __builtin_bswap16(x)
#define htole16(x) (x)

#define be32toh(x) __builtin_bswap32(x)
#define le32toh(x) (x)
#define htobe32(x) __builtin_bswap32(x)
#define htole32(x) (x)

#define be64toh(x) __builtin_bswap64(x)
#define le64toh(x) (x)
#define htobe64(x) __builtin_bswap64(x)
#define htole64(x) (x)

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define be16toh(x) (x)
#define le16toh(x) __builtin_bswap16(x)
#define htobe16(x) (x)
#define htole16(x) __builtin_bswap16(x)

#define be32toh(x) (x)
#define le32toh(x) __builtin_bswap32(x)
#define htobe32(x) (x)
#define htole32(x) __builtin_bswap32(x)

#define be64toh(x) (x)
#define le64toh(x) __builtin_bswap64(x)
#define htobe64(x) (x)
#define htole64(x) __builtin_bswap64(x)

#else
#error "Only support pure big and little endian at the moment"
#endif

#else
#error "I lack the way to check my endianness and convert to/from big-endian"
#endif

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"
#include "ldap_queue.h"

typedef enum datamorph_type_t {
	DATAMORPH_UNSET,
	DATAMORPH_ENUM,
	DATAMORPH_INT,
} datamorph_type;

typedef enum datamorph_flags_t {
	DATAMORPH_FLAG_SIGNED = 1 << 0,
	DATAMORPH_FLAG_LOWER = 1 << 1,
	DATAMORPH_FLAG_UPPER = 1 << 2,
} datamorph_flags;

typedef union datamorph_interval_bound_t {
	int64_t i;
	uint64_t u;
} datamorph_interval_bound;

typedef struct transformation_info_t {
	AttributeDescription *attr;
	datamorph_type type;
	union {
		struct {
			Avlnode *to_db;
			struct berval from_db[256];
		} maps;
#define ti_enum info.maps
		struct {
			datamorph_flags flags;
			unsigned int size;
			datamorph_interval_bound lower, upper;
		} interval;
#define ti_int info.interval
	} info;
} transformation_info;

typedef struct datamorph_enum_mapping_t {
	struct berval wire_value;
	uint8_t db_value;
	transformation_info *transformation;
} datamorph_enum_mapping;

typedef struct datamorph_info_t {
	Avlnode *transformations;
	transformation_info *wip_transformation;
} datamorph_info;

static int
transformation_mapping_cmp( const void *l, const void *r )
{
	const datamorph_enum_mapping *left = l, *right = r;

	return ber_bvcmp( &left->wire_value, &right->wire_value );
}

static int
transformation_info_cmp( const void *l, const void *r )
{
	const transformation_info *left = l, *right = r;

	return ( left->attr == right->attr ) ? 0 :
			( left->attr < right->attr ) ? -1 :
											1;
}

static int
transform_to_db_format_one(
		Operation *op,
		transformation_info *definition,
		struct berval *value,
		struct berval *outval )
{
	switch ( definition->type ) {
		case DATAMORPH_ENUM: {
			datamorph_enum_mapping *mapping, needle = { .wire_value = *value };
			struct berval db_value = { .bv_len = 1 };

			mapping = ldap_avl_find( definition->ti_enum.to_db, &needle,
					transformation_mapping_cmp );
			if ( !mapping ) {
				Debug( LDAP_DEBUG_ANY, "transform_to_db_format_one: "
						"value '%s' not mapped\n",
						value->bv_val );
				return LDAP_CONSTRAINT_VIOLATION;
			}

			db_value.bv_val = (char *)&mapping->db_value;
			ber_dupbv( outval, &db_value );
			assert( outval->bv_val );
			break;
		}

		case DATAMORPH_INT: {
			union {
				char s[8];
				uint8_t be8;
				uint16_t be16;
				uint32_t be32;
				uint64_t be64;
			} buf;
			struct berval db_value = { .bv_val = buf.s };
			char *ptr = value->bv_val + value->bv_len;
			uint64_t unsigned_value;
			int64_t signed_value;

			assert( definition->ti_int.size == 1 ||
					definition->ti_int.size == 2 ||
					definition->ti_int.size == 4 ||
					definition->ti_int.size == 8 );

			/* Read number */
			if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
				signed_value = strtoll( value->bv_val, &ptr, 10 );
			} else {
				unsigned_value = strtoull( value->bv_val, &ptr, 10 );
			}
			if ( *value->bv_val == '\0' || *ptr != '\0' ) {
				Debug( LDAP_DEBUG_ANY, "transform_to_db_format_one: "
						"value '%s' not an integer\n",
						value->bv_val );
				return LDAP_CONSTRAINT_VIOLATION;
			}
			/* Check it's within configured bounds */
			if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
				if ( signed_value < definition->ti_int.lower.i ||
						signed_value > definition->ti_int.upper.i ) {
					Debug( LDAP_DEBUG_ANY, "transform_to_db_format_one: "
							"value '%s' doesn't fit configured constraints\n",
							value->bv_val );
					return LDAP_CONSTRAINT_VIOLATION;
				}
			} else {
				if ( unsigned_value < definition->ti_int.lower.u ||
						unsigned_value > definition->ti_int.upper.u ) {
					Debug( LDAP_DEBUG_ANY, "transform_to_db_format_one: "
							"value '%s' doesn't fit configured constraints\n",
							value->bv_val );
					return LDAP_CONSTRAINT_VIOLATION;
				}
			}

			db_value.bv_len = definition->ti_int.size;
			switch ( definition->ti_int.size ) {
				case 1: {
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						buf.be8 = (unsigned char)((char)signed_value);
					} else {
						buf.be8 = unsigned_value;
					}
					break;
				}
				case 2: {
					uint16_t h16;
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						h16 = signed_value;
					} else {
						h16 = unsigned_value;
					}
					buf.be16 = htobe16( h16 );
					break;
				}
				case 4: {
					uint32_t h32;
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						h32 = signed_value;
					} else {
						h32 = unsigned_value;
					}
					buf.be32 = htobe32( h32 );
					break;
				}
				case 8: {
					uint64_t h64;
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						h64 = signed_value;
					} else {
						h64 = unsigned_value;
					}
					buf.be64 = htobe64( h64 );
					break;
				}
			}
			ber_dupbv( outval, &db_value );
			assert( outval->bv_val );
			break;
		}

		default:
			assert(0);
	}

	return LDAP_SUCCESS;
}

static int
transform_to_db_format(
		Operation *op,
		transformation_info *definition,
		BerVarray values,
		int numvals,
		BerVarray *out )
{
	struct berval *value;
	int i, rc = LDAP_SUCCESS;

	if ( numvals == 0 ) {
		for ( value = values; value; value++, numvals++ )
			; /* Count them */
	}

	assert( out );
	*out = ch_calloc( numvals + 1, sizeof(struct berval) );

	for ( i = 0; i < numvals; i++ ) {
		rc = transform_to_db_format_one(
				op, definition, &values[i], &(*out)[i] );
		if ( rc ) {
			break;
		}
	}

	if ( rc ) {
		for ( ; i >= 0; i-- ) {
			ch_free((*out)[i].bv_val);
		}
		ch_free(*out);
	}

	return rc;
}

static int
transform_from_db_format_one(
		Operation *op,
		transformation_info *definition,
		struct berval *value,
		struct berval *outval )
{
	switch ( definition->type ) {
		case DATAMORPH_ENUM: {
			uint8_t index = value->bv_val[0];
			struct berval *val = &definition->info.maps.from_db[index];

			if ( !BER_BVISNULL( val ) ) {
				ber_dupbv( outval, val );
				assert( outval->bv_val );
			} else {
				Debug( LDAP_DEBUG_ANY, "transform_from_db_format_one: "
						"DB value %d has no mapping!\n",
						index );
				/* FIXME: probably still need to return an error */
				BER_BVZERO( outval );
			}
			break;
		}

		case DATAMORPH_INT: {
			char buf[24];
			struct berval wire_value = { .bv_val = buf };
			union lens_t {
				uint8_t be8;
				uint16_t be16;
				uint32_t be32;
				uint64_t be64;
			} *lens = (union lens_t *)value->bv_val;
			uint64_t unsigned_value;
			int64_t signed_value;

			if ( value->bv_len != definition->ti_int.size ) {
				Debug( LDAP_DEBUG_ANY, "transform_from_db_format_one(%s): "
						"unexpected DB value of length %lu when configured "
						"for %u!\n",
						definition->attr->ad_cname.bv_val, value->bv_len,
						definition->ti_int.size );
				/* FIXME: probably still need to return an error */
				BER_BVZERO( outval );
				break;
			}

			assert( definition->ti_int.size == 1 ||
					definition->ti_int.size == 2 ||
					definition->ti_int.size == 4 ||
					definition->ti_int.size == 8 );

			switch ( definition->ti_int.size ) {
				case 1: {
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						signed_value = (int8_t)lens->be8;
					} else {
						unsigned_value = (uint8_t)lens->be8;
					}
					break;
				}
				case 2: {
					uint16_t h16 = be16toh( lens->be16 );
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						signed_value = (int16_t)h16;
					} else {
						unsigned_value = (uint16_t)h16;
					}
					break;
				}
				case 4: {
					uint32_t h32 = be32toh( lens->be32 );
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						signed_value = (int32_t)h32;
					} else {
						unsigned_value = (uint32_t)h32;
					}
					break;
				}
				case 8: {
					uint64_t h64 = be64toh( lens->be64 );
					if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
						signed_value = (int64_t)h64;
					} else {
						unsigned_value = (uint64_t)h64;
					}
					break;
				}
			}
			if ( definition->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
				wire_value.bv_len = sprintf( buf, "%" PRId64, signed_value );
			} else {
				wire_value.bv_len = sprintf( buf, "%" PRIu64, unsigned_value );
			}
			ber_dupbv( outval, &wire_value );
			assert( outval->bv_val );
			break;
		}

		default:
			assert(0);
	}

	return LDAP_SUCCESS;
}

static int
transform_from_db_format(
		Operation *op,
		transformation_info *definition,
		BerVarray values,
		int numvals,
		BerVarray *out )
{
	struct berval *value;
	int i, rc = LDAP_SUCCESS;

	if ( numvals == 0 ) {
		for ( value = values; value; value++, numvals++ )
			; /* Count them */
	}

	assert( out );
	*out = ch_calloc( numvals + 1, sizeof(struct berval) );

	for ( i = 0; i < numvals; i++ ) {
		struct berval bv;
		rc = transform_from_db_format_one( op, definition, &values[i], &bv );
		if ( !BER_BVISNULL( &bv ) ) {
			ber_bvarray_add( out, &bv );
		}
		if ( rc ) {
			break;
		}
	}

	if ( rc ) {
		for ( ; i >= 0; i-- ) {
			ch_free( (*out)[i].bv_val );
		}
		ch_free( *out );
	}

	return rc;
}

static int
datamorph_filter( Operation *op, datamorph_info *ov, Filter *f )
{
	switch ( f->f_choice ) {
		case LDAP_FILTER_PRESENT:
		/* The matching rules are not in place,
		 * so the filter will be ignored */
		case LDAP_FILTER_APPROX:
		case LDAP_FILTER_SUBSTRINGS:
		default:
			break;
			return LDAP_SUCCESS;

		case LDAP_FILTER_AND:
		case LDAP_FILTER_OR: {
			for ( f = f->f_and; f; f = f->f_next ) {
				int rc = datamorph_filter( op, ov, f );
				if ( rc != LDAP_SUCCESS ) {
					return rc;
				}
			}
		} break;

		case LDAP_FILTER_NOT:
			return datamorph_filter( op, ov, f->f_not );

		case LDAP_FILTER_EQUALITY:
		case LDAP_FILTER_GE:
		case LDAP_FILTER_LE: {
			transformation_info *t, needle = { .attr = f->f_ava->aa_desc };

			t = ldap_avl_find(
					ov->transformations, &needle, transformation_info_cmp );
			if ( t ) {
				struct berval new_val;
				int rc = transform_to_db_format_one(
						op, t, &f->f_ava->aa_value, &new_val );
				ch_free( f->f_ava->aa_value.bv_val );

				if ( rc != LDAP_SUCCESS ) {
					f->f_choice = SLAPD_FILTER_COMPUTED;
					f->f_result = SLAPD_COMPARE_UNDEFINED;
				} else {
					f->f_ava->aa_value = new_val;
				}
			}
		} break;
	}
	return LDAP_SUCCESS;
}

static int
datamorph_op_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	Entry *e = op->ora_e;
	Attribute *a, *next;
	AttributeDescription *stop = NULL;
	int rc = LDAP_SUCCESS;

	if ( !BER_BVISNULL( &e->e_nname ) && !BER_BVISEMPTY( &e->e_nname ) ) {
		LDAPRDN rDN;
		const char *p;
		int i;

		rc = ldap_bv2rdn_x( &e->e_nname, &rDN, (char **)&p, LDAP_DN_FORMAT_LDAP,
				op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "datamorph_op_add: "
					"can't parse rdn: dn=%s\n",
					op->o_req_ndn.bv_val );
			return SLAP_CB_CONTINUE;
		}

		for ( i = 0; rDN[i]; i++ ) {
			transformation_info needle = {};

			/* If we can't resolve the attribute, ignore it */
			if ( slap_bv2ad( &rDN[i]->la_attr, &needle.attr, &p ) ) {
				continue;
			}

			if ( ldap_avl_find( ov->transformations, &needle,
						 transformation_info_cmp ) ) {
				rc = LDAP_CONSTRAINT_VIOLATION;
				Debug( LDAP_DEBUG_TRACE, "datamorph_op_add: "
						"attempted to add transformed attribute in RDN\n" );
				break;
			}
		}

		ldap_rdnfree_x( rDN, op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			send_ldap_error( op, rs, rc,
					"datamorph: trying to add transformed attribute in RDN" );
			return rc;
		}
	}

	for ( a = e->e_attrs; a && a->a_desc != stop; a = next ) {
		transformation_info *t, needle = { .attr = a->a_desc };
		BerVarray new_vals;

		next = a->a_next;

		t = ldap_avl_find( ov->transformations, &needle, transformation_info_cmp );
		if ( !t ) continue;

		rc = transform_to_db_format(
				op, t, a->a_vals, a->a_numvals, &new_vals );
		if ( rc != LDAP_SUCCESS ) {
			goto fail;
		}

		(void)attr_delete( &e->e_attrs, needle.attr );

		rc = attr_merge( e, needle.attr, new_vals, NULL );
		ber_bvarray_free( new_vals );
		if ( rc != LDAP_SUCCESS ) {
			goto fail;
		}
		if ( !stop ) {
			stop = needle.attr;
		}
	}

	return SLAP_CB_CONTINUE;

fail:
	send_ldap_error(
			op, rs, rc, "datamorph: trying to add values outside definitions" );
	return rc;
}

static int
datamorph_op_compare( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	transformation_info *t, needle = { .attr = op->orc_ava->aa_desc };
	int rc = SLAP_CB_CONTINUE;

	t = ldap_avl_find( ov->transformations, &needle, transformation_info_cmp );
	if ( t ) {
		struct berval new_val;

		rc = transform_to_db_format_one(
				op, t, &op->orc_ava->aa_value, &new_val );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "datamorph_op_compare: "
					"transformation failed for '%s', rc=%d\n",
					op->orc_ava->aa_value.bv_val, rc );
			rs->sr_err = rc = LDAP_COMPARE_FALSE;
			send_ldap_result( op, rs );
			return rc;
		}
		ch_free( op->orc_ava->aa_value.bv_val );
		op->orc_ava->aa_value = new_val;
	}

	return SLAP_CB_CONTINUE;
}

static int
datamorph_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	Modifications *mod;
	int rc = SLAP_CB_CONTINUE;

	for ( mod = op->orm_modlist; mod; mod = mod->sml_next ) {
		transformation_info *t, needle = { .attr = mod->sml_desc };
		BerVarray new_vals = NULL;

		if ( mod->sml_numvals == 0 ) continue; /* Nothing to transform */

		t = ldap_avl_find( ov->transformations, &needle, transformation_info_cmp );
		if ( !t ) continue;

		assert( !mod->sml_nvalues );
		rc = transform_to_db_format(
				op, t, mod->sml_values, mod->sml_numvals, &new_vals );
		if ( rc != LDAP_SUCCESS ) {
			goto fail;
		}
		ber_bvarray_free( mod->sml_values );
		mod->sml_values = new_vals;
	}

	return SLAP_CB_CONTINUE;

fail:
	Debug( LDAP_DEBUG_TRACE, "datamorph_op_mod: "
			"dn=%s failed rc=%d\n",
			op->o_req_ndn.bv_val, rc );
	send_ldap_error( op, rs, rc,
			"datamorph: trying to operate on values outside definitions" );
	return rc;
}

static int
datamorph_op_modrdn( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	LDAPRDN rDN;
	const char *p;
	int i, rc;

	rc = ldap_bv2rdn_x( &op->orr_nnewrdn, &rDN, (char **)&p,
			LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "datamorph_op_modrdn: "
				"can't parse rdn for dn=%s\n",
				op->o_req_ndn.bv_val );
		return SLAP_CB_CONTINUE;
	}

	for ( i = 0; rDN[i]; i++ ) {
		transformation_info needle = {};

		/* If we can't resolve the attribute, ignore it */
		if ( slap_bv2ad( &rDN[i]->la_attr, &needle.attr, &p ) ) {
			continue;
		}

		if ( ldap_avl_find(
					 ov->transformations, &needle, transformation_info_cmp ) ) {
			rc = LDAP_CONSTRAINT_VIOLATION;
			Debug( LDAP_DEBUG_TRACE, "datamorph_op_modrdn: "
					"attempted to add transformed values in RDN\n" );
			break;
		}
	}

	ldap_rdnfree_x( rDN, op->o_tmpmemctx );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_error( op, rs, rc,
				"datamorph: trying to put transformed values in RDN" );
		return rc;
	}

	return SLAP_CB_CONTINUE;
}

static int
datamorph_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	Entry *e = NULL, *e_orig = rs->sr_entry;
	AttributeDescription *stop = NULL;
	Attribute *a, *next = NULL;
	int rc = SLAP_CB_CONTINUE;

	if ( rs->sr_type != REP_SEARCH ) {
		return rc;
	}

	for ( a = e_orig->e_attrs; a && a->a_desc != stop; a = next ) {
		transformation_info *t, needle = { .attr = a->a_desc };
		BerVarray new_vals;

		next = a->a_next;

		t = ldap_avl_find( ov->transformations, &needle, transformation_info_cmp );
		if ( !t ) continue;

		rc = transform_from_db_format(
				op, t, a->a_vals, a->a_numvals, &new_vals );
		if ( rc != LDAP_SUCCESS ) {
			break;
		}
		if ( !e ) {
			if ( rs->sr_flags & REP_ENTRY_MODIFIABLE ) {
				e = e_orig;
			} else {
				e = entry_dup( e_orig );
			}
		}

		(void)attr_delete( &e->e_attrs, needle.attr );

		rc = attr_merge( e, needle.attr, new_vals, NULL );
		ber_bvarray_free( new_vals );
		if ( rc != LDAP_SUCCESS ) {
			break;
		}
		if ( !stop ) {
			stop = needle.attr;
		}
	}

	if ( rc == LDAP_SUCCESS ) {
		rc = SLAP_CB_CONTINUE;
		if ( e && e != e_orig ) {
			rs_replace_entry( op, rs, on, e );
			rs->sr_flags &= ~REP_ENTRY_MASK;
			rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
		}
	} else if ( e && e != e_orig ) {
		entry_free( e );
	}

	return rc;
}

static int
datamorph_op_search( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	int rc = SLAP_CB_CONTINUE;

	/*
	 * 1. check all requested attributes -> register callback if one matches
	 * 2. check filter: parse filter, traverse, for configured attributes:
	 *    - presence -> do not touch
	 *    - ava -> replace assertion value with db value if possible, assertion with undefined otherwise 
	 *    - inequality -> ???
	 *    - anything else -> undefined
	 *    - might just check for equality and leave the rest to syntax?
	 * 3. unparse filter
	 */
	if ( datamorph_filter( op, ov, op->ors_filter ) ) {
		send_ldap_error(
				op, rs, LDAP_OTHER, "datamorph: failed to process filter" );
		return LDAP_OTHER;
	}

	return rc;
}

static int
datamorph_entry_release_rw( Operation *op, Entry *e, int rw )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	int rc = LDAP_SUCCESS;

	if ( on->on_next ) {
		rc = overlay_entry_release_ov( op, e, rw, on->on_next );
	} else if ( on->on_info->oi_orig->bi_entry_release_rw ) {
		/* FIXME: there should be a better way */
		rc = on->on_info->oi_orig->bi_entry_release_rw( op, e, rw );
	} else {
		entry_free( e );
	}

	return rc;
}

static int
datamorph_entry_get_rw(
		Operation *op,
		struct berval *ndn,
		ObjectClass *oc,
		AttributeDescription *at,
		int rw,
		Entry **ep )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;
	Entry *e_orig, *e = NULL;
	int rc;

	if ( on->on_next ) {
		rc = overlay_entry_get_ov( op, ndn, oc, at, rw, ep, on->on_next );
	} else {
		/* FIXME: there should be a better way */
		rc = on->on_info->oi_orig->bi_entry_get_rw( op, ndn, oc, at, rw, ep );
	}
	e_orig = *ep;

	if ( rc == LDAP_SUCCESS && e_orig ) {
		AttributeDescription *stop = NULL;
		Attribute *a;

		for ( a = e_orig->e_attrs; a; a = a->a_next ) {
			transformation_info *t, needle = { .attr = a->a_desc };
			BerVarray new_vals;

			t = ldap_avl_find(
					ov->transformations, &needle, transformation_info_cmp );
			if ( !t ) continue;

			rc = transform_from_db_format(
					op, t, a->a_vals, a->a_numvals, &new_vals );
			if ( rc != LDAP_SUCCESS ) {
				goto fail;
			}
			if ( !e ) {
				e = entry_dup( e_orig );
			}

			(void)attr_delete( &e->e_attrs, needle.attr );

			rc = attr_merge( e, needle.attr, new_vals, NULL );
			ber_bvarray_free( new_vals );
			if ( rc != LDAP_SUCCESS ) {
				goto fail;
			}
			if ( !stop ) {
				stop = needle.attr;
			}
		}
	}
	if ( e ) {
		datamorph_entry_release_rw( op, e_orig, rw );
		*ep = e;
	}

	return rc;

fail:
	if ( e ) {
		entry_free( e );
	}
	(void)datamorph_entry_release_rw( op, *ep, rw );
	return rc;
}

/* Schema */

static int
datamorphBlobValidate( Syntax *syntax, struct berval *in )
{
	/* any value allowed */
	return LDAP_SUCCESS;
}

int
datamorphBinarySignedOrderingMatch( int *matchp,
		slap_mask_t flags,
		Syntax *syntax,
		MatchingRule *mr,
		struct berval *value,
		void *assertedValue )
{
	struct berval *asserted = assertedValue;
	ber_len_t v_len = value->bv_len;
	ber_len_t av_len = asserted->bv_len;

	/* Ordering:
	 * 1. Negative always before non-negative
	 * 2. Shorter before longer
	 * 3. Rest ordered by memory contents (they are big-endian numbers)
	 */
	int match = ( *value->bv_val >= 0 ) - ( *asserted->bv_val >= 0 );

	if ( match == 0 ) match = (int)v_len - (int)av_len;

	if ( match == 0 ) match = memcmp( value->bv_val, asserted->bv_val, v_len );

	/* If used in extensible match filter, match if value < asserted */
	if ( flags & SLAP_MR_EXT ) match = ( match >= 0 );

	*matchp = match;
	return LDAP_SUCCESS;
}

/* Index generation function: Ordered index */
int
datamorphUnsignedIndexer( slap_mask_t use,
		slap_mask_t flags,
		Syntax *syntax,
		MatchingRule *mr,
		struct berval *prefix,
		BerVarray values,
		BerVarray *keysp,
		void *ctx )
{
	int i;
	BerVarray keys;

	for ( i = 0; values[i].bv_val != NULL; i++ ) {
		/* just count them */
	}

	/* we should have at least one value at this point */
	assert( i > 0 );

	keys = slap_sl_malloc( sizeof(struct berval) * ( i + 1 ), ctx );

	for ( i = 0; values[i].bv_val != NULL; i++ ) {
		ber_dupbv_x( &keys[i], &values[i], ctx );
	}

	BER_BVZERO( &keys[i] );

	*keysp = keys;

	return LDAP_SUCCESS;
}

/* Index generation function: Ordered index */
int
datamorphUnsignedFilter(
		slap_mask_t use,
		slap_mask_t flags,
		Syntax *syntax,
		MatchingRule *mr,
		struct berval *prefix,
		void *assertedValue,
		BerVarray *keysp,
		void *ctx )
{
	BerVarray keys;
	BerValue *value = assertedValue;

	keys = slap_sl_malloc( sizeof(struct berval) * 2, ctx );
	ber_dupbv_x( &keys[0], value, ctx );

	BER_BVZERO( &keys[1] );

	*keysp = keys;

	return LDAP_SUCCESS;
}

/* Index generation function: Ordered index */
int
datamorphSignedIndexer(
		slap_mask_t use,
		slap_mask_t flags,
		Syntax *syntax,
		MatchingRule *mr,
		struct berval *prefix,
		BerVarray values,
		BerVarray *keysp,
		void *ctx )
{
	int i;
	BerVarray keys;

	for ( i = 0; values[i].bv_val != NULL; i++ ) {
		/* just count them */
	}

	/* we should have at least one value at this point */
	assert( i > 0 );

	keys = slap_sl_malloc( sizeof(struct berval) * ( i + 1 ), ctx );

	for ( i = 0; values[i].bv_val != NULL; i++ ) {
		keys[i].bv_len = values[i].bv_len + 1;
		keys[i].bv_val = slap_sl_malloc( keys[i].bv_len, ctx );

		/* if positive (highest bit is not set), note that in the first byte */
		*keys[i].bv_val = ~( *values[i].bv_val & 0x80 );
		AC_MEMCPY( keys[i].bv_val + 1, values[i].bv_val, values[i].bv_len );
	}

	BER_BVZERO( &keys[i] );

	*keysp = keys;

	return LDAP_SUCCESS;
}

/* Index generation function: Ordered index */
int
datamorphSignedFilter(
		slap_mask_t use,
		slap_mask_t flags,
		Syntax *syntax,
		MatchingRule *mr,
		struct berval *prefix,
		void *assertedValue,
		BerVarray *keysp,
		void *ctx )
{
	BerVarray keys;
	BerValue *value = assertedValue;

	keys = slap_sl_malloc( sizeof(struct berval) * 2, ctx );

	keys[0].bv_len = value->bv_len + 1;
	keys[0].bv_val = slap_sl_malloc( keys[0].bv_len, ctx );

	/* if positive (highest bit is not set), note that in the first byte */
	*keys[0].bv_val = ~( *value->bv_val & 0x80 );
	AC_MEMCPY( keys[0].bv_val + 1, value->bv_val, value->bv_len );

	BER_BVZERO( &keys[1] );

	*keysp = keys;

	return LDAP_SUCCESS;
}

#define DATAMORPH_ARC "1.3.6.1.4.1.4203.666.11.12"

#define DATAMORPH_SYNTAXES DATAMORPH_ARC ".1"
#define DATAMORPH_SYNTAX_BASE DATAMORPH_SYNTAXES ".1"
#define DATAMORPH_SYNTAX_ENUM DATAMORPH_SYNTAXES ".2"
#define DATAMORPH_SYNTAX_INT DATAMORPH_SYNTAXES ".3"
#define DATAMORPH_SYNTAX_SIGNED_INT DATAMORPH_SYNTAXES ".4"

#define DATAMORPH_MATCHES DATAMORPH_ARC ".2"
#define DATAMORPH_MATCH_EQUALITY DATAMORPH_MATCHES ".1"
#define DATAMORPH_MATCH_SIGNED_EQUALITY DATAMORPH_MATCHES ".2"
#define DATAMORPH_MATCH_ORDERING DATAMORPH_MATCHES ".3"
#define DATAMORPH_MATCH_SIGNED_ORDERING DATAMORPH_MATCHES ".4"

static char *datamorph_sups[] = {
	DATAMORPH_SYNTAX_BASE,
	NULL
};

static char *datamorphSyntaxes[] = {
	DATAMORPH_SYNTAX_SIGNED_INT,
	DATAMORPH_SYNTAX_ENUM,
	DATAMORPH_SYNTAX_INT,

	NULL
};

static slap_syntax_defs_rec datamorph_syntax_defs[] = {
	{ "( " DATAMORPH_SYNTAX_BASE " DESC 'Fixed size value' )",
		0, NULL, NULL, NULL
	},
	{ "( " DATAMORPH_SYNTAX_ENUM " DESC 'Enumerated value' )",
		0, datamorph_sups, datamorphBlobValidate, NULL
	},
	{ "( " DATAMORPH_SYNTAX_INT " DESC 'Fixed-size integer' )",
		0, datamorph_sups, datamorphBlobValidate, NULL
	},
	{ "( " DATAMORPH_SYNTAX_SIGNED_INT " DESC 'Fixed-size signed integer' )",
		0, datamorph_sups, datamorphBlobValidate, NULL
	},

	{ NULL, 0, NULL, NULL, NULL }
};

static Syntax *datamorph_base_syntax;

static slap_mrule_defs_rec datamorph_mrule_defs[] = {
	{ "( " DATAMORPH_MATCH_EQUALITY
		" NAME 'fixedSizeIntegerMatch'"
		" SYNTAX " DATAMORPH_SYNTAX_INT " )",
		SLAP_MR_EQUALITY|SLAP_MR_EXT|SLAP_MR_ORDERED_INDEX,
		datamorphSyntaxes + 1,
		NULL, NULL, octetStringOrderingMatch,
		datamorphUnsignedIndexer, datamorphUnsignedFilter,
		NULL
	},

	{ "( " DATAMORPH_MATCH_SIGNED_EQUALITY
		" NAME 'fixedSizeSignedIntegerMatch'"
		" SYNTAX " DATAMORPH_SYNTAX_SIGNED_INT " )",
		SLAP_MR_EQUALITY|SLAP_MR_EXT|SLAP_MR_ORDERED_INDEX,
		NULL,
		NULL, NULL, datamorphBinarySignedOrderingMatch,
		datamorphSignedIndexer, datamorphSignedFilter,
		NULL
	},

	{ "( " DATAMORPH_MATCH_ORDERING
		" NAME 'fixedSizeIntegerOrderingMatch'"
		" SYNTAX " DATAMORPH_SYNTAX_INT " )",
		SLAP_MR_ORDERING|SLAP_MR_EXT|SLAP_MR_ORDERED_INDEX,
		datamorphSyntaxes + 1,
		NULL, NULL, octetStringOrderingMatch,
		datamorphUnsignedIndexer, datamorphUnsignedFilter,
		"octetStringMatch" },

	{ "( " DATAMORPH_MATCH_SIGNED_ORDERING
		" NAME 'fixedSizeSignedIntegerOrderingMatch'"
		" SYNTAX " DATAMORPH_SYNTAX_SIGNED_INT " )",
		SLAP_MR_ORDERING|SLAP_MR_EXT|SLAP_MR_ORDERED_INDEX,
		NULL,
		NULL, NULL, datamorphBinarySignedOrderingMatch,
		datamorphSignedIndexer, datamorphSignedFilter,
		"octetStringMatch" },

	{ NULL, SLAP_MR_NONE, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/* Configuration */

static ConfigLDAPadd datamorph_ldadd_enum;
static ConfigLDAPadd datamorph_ldadd_interval;
static ConfigLDAPadd datamorph_ldadd_mapping;

static ConfigDriver datamorph_set_attribute;
static ConfigDriver datamorph_set_size;
static ConfigDriver datamorph_set_signed;
static ConfigDriver datamorph_set_bounds;
static ConfigDriver datamorph_set_index;
static ConfigDriver datamorph_set_value;
static ConfigDriver datamorph_add_mapping;
static ConfigDriver datamorph_add_transformation;

static ConfigCfAdd datamorph_cfadd;

enum {
	DATAMORPH_INT_SIZE = 1,
	DATAMORPH_INT_SIGNED,
	DATAMORPH_INT_LOWER,
	DATAMORPH_INT_UPPER,

	DATAMORPH_INT_LAST,
};

static ConfigTable datamorph_cfg[] = {
	{ "datamorph_attribute", "attr", 2, 2, 0,
		ARG_STRING|ARG_QUOTE|ARG_MAGIC,
		datamorph_set_attribute,
		"( OLcfgCtAt:7.1 NAME 'olcDatamorphAttribute' "
			"DESC 'Attribute to transform' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "datamorph_size", "<1|2|4|8>", 2, 2, 0,
		ARG_INT|ARG_MAGIC|DATAMORPH_INT_SIZE,
		datamorph_set_size,
		"( OLcfgCtAt:7.2 NAME 'olcDatamorphIntegerBytes' "
			"DESC 'Integer size in bytes' "
			"EQUALITY integerMatch "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "datamorph_signed", "TRUE|FALSE", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|DATAMORPH_INT_SIGNED,
		datamorph_set_signed,
		"( OLcfgCtAt:7.3 NAME 'olcDatamorphIntegerSigned' "
			"DESC 'Whether integers maintain sign' "
			"EQUALITY booleanMatch "
			"SYNTAX OMsBoolean "
				"SINGLE-VALUE )",
			NULL, NULL
	},
	{ "datamorph_lower_bound", "int", 2, 2, 0,
		ARG_BERVAL|ARG_MAGIC|DATAMORPH_INT_LOWER,
		datamorph_set_bounds,
		"( OLcfgCtAt:7.4 NAME 'olcDatamorphIntegerLowerBound' "
			"DESC 'Lowest valid value for the attribute' "
			"EQUALITY integerMatch "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "datamorph_upper_bound", "int", 2, 2, 0,
		ARG_BERVAL|ARG_MAGIC|DATAMORPH_INT_UPPER,
		datamorph_set_bounds,
		"( OLcfgCtAt:7.5 NAME 'olcDatamorphIntegerUpperBound' "
			"DESC 'Highest valid value for the attribute' "
			"EQUALITY integerMatch "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL
	},

	/* These have no equivalent in slapd.conf */
	{ "", NULL, 2, 2, 0,
		ARG_INT|ARG_MAGIC,
		datamorph_set_index,
		"( OLcfgCtAt:7.6 NAME 'olcDatamorphIndex' "
			"DESC 'Internal DB value' "
			"EQUALITY integerMatch "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, NULL
	},
	{ "", NULL, 2, 2, 0,
		ARG_BERVAL|ARG_QUOTE|ARG_MAGIC,
		datamorph_set_value,
		"( OLcfgCtAt:7.7 NAME 'olcDatamorphValue' "
			"DESC 'Wire value' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},

	/* slapd.conf alternative for the two above */
	{ "datamorph_value", "int> <name", 3, 3, 0,
		ARG_QUOTE|ARG_MAGIC,
		datamorph_add_mapping,
		NULL, NULL, NULL
	},

	/* slapd.conf alternative for objectclasses below */
	{ "datamorph", "enum|int> <attr", 3, 3, 0,
		ARG_QUOTE|ARG_MAGIC,
		datamorph_add_transformation,
		NULL, NULL, NULL
	},

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs datamorph_ocs[] = {
	{ "( OLcfgCtOc:7.1 "
		"NAME 'olcDatamorphConfig' "
		"DESC 'Datamorph overlay configuration' "
		"SUP olcOverlayConfig )",
		Cft_Overlay, datamorph_cfg, NULL, datamorph_cfadd },
	{ "( OLcfgCtOc:7.2 "
		"NAME 'olcTransformation' "
		"DESC 'Transformation configuration' "
		"MUST ( olcDatamorphAttribute ) "
		"SUP top "
		"ABSTRACT )",
		Cft_Misc, datamorph_cfg, NULL },
	{ "( OLcfgCtOc:7.3 "
		"NAME 'olcDatamorphEnum' "
		"DESC 'Configuration for an enumerated attribute' "
		"SUP olcTransformation "
		"STRUCTURAL )",
		Cft_Misc, datamorph_cfg, datamorph_ldadd_enum },
	{ "( OLcfgCtOc:7.4 "
		"NAME 'olcDatamorphInteger' "
		"DESC 'Configuration for a compact integer attribute' "
		"MUST ( olcDatamorphIntegerBytes ) "
		"MAY ( olcDatamorphIntegerLowerBound $ "
			"olcDatamorphIntegerUpperBound $ "
			"olcDatamorphIntegerSigned "
		") "
		"SUP olcTransformation "
		"STRUCTURAL )",
		Cft_Misc, datamorph_cfg, datamorph_ldadd_interval },
	{ "( OLcfgCtOc:7.5 "
		"NAME 'olcDatamorphEnumValue' "
		"DESC 'Configuration for an enumerated attribute' "
		"MUST ( olcDatamorphIndex $ "
			"olcDatamorphValue "
		") "
		"STRUCTURAL )",
		Cft_Misc, datamorph_cfg, datamorph_ldadd_mapping },

	{ NULL, 0, NULL }
};

static void
datamorph_mapping_free( void *arg )
{
	datamorph_enum_mapping *mapping = arg;

	ch_free( mapping->wire_value.bv_val );
	ch_free( mapping );
}

static void
datamorph_info_free( void *arg )
{
	transformation_info *info = arg;

	if ( info->type == DATAMORPH_ENUM ) {
		ldap_avl_free( info->ti_enum.to_db, datamorph_mapping_free );
	}
	ch_free( info );
}

static int
datamorph_set_attribute( ConfigArgs *ca )
{
	transformation_info needle = {}, *info = ca->ca_private;
	slap_overinst *on = (slap_overinst *)ca->bi;
	datamorph_info *ov = on->on_bi.bi_private;
	char *s = ca->value_string;
	const char *text;
	int rc = LDAP_SUCCESS;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_string = info->attr->ad_cname.bv_val;
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		info = ldap_avl_delete( &ov->transformations, info,
				transformation_info_cmp );
		assert( info );

		info->attr = NULL;
		return LDAP_SUCCESS;
	}

	if ( *s == '{' ) {
		s = strchr( s, '}' );
		if ( !s ) {
			rc = LDAP_UNDEFINED_TYPE;
			goto done;
		}
		s += 1;
	}

	rc = slap_str2ad( s, &info->attr, &text );
	ch_free( ca->value_string );
	if ( rc ) {
		goto done;
	}

	/* The type has to be set appropriately */
	if ( !info->attr->ad_type->sat_syntax->ssyn_sups ||
			info->attr->ad_type->sat_syntax->ssyn_sups[0] !=
					datamorph_base_syntax ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"improper syntax for attribute %s",
				info->attr->ad_cname.bv_val );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;
	}

	needle.attr = info->attr;
	if ( ldap_avl_find( ov->transformations, &needle, transformation_info_cmp ) ) {
		rc = LDAP_CONSTRAINT_VIOLATION;
		goto done;
	}

done:
	if ( rc ) {
		ca->reply.err = rc;
	}
	return rc;
}

static int
datamorph_set_size( ConfigArgs *ca )
{
	transformation_info *info = ca->ca_private;

	if ( !info ) {
		slap_overinst *on = (slap_overinst *)ca->bi;
		datamorph_info *ov = on->on_bi.bi_private;
		info = ov->wip_transformation;
		assert( ca->op == SLAP_CONFIG_ADD );
	}

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_int = info->ti_int.size;
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		info->ti_int.size = 0;
		return LDAP_SUCCESS;
	}

	if ( ca->value_int != 1 &&
			ca->value_int != 2 &&
			ca->value_int != 4 &&
			ca->value_int != 8 ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg), "invalid size %d",
				ca->value_int );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	}
	info->ti_int.size = ca->value_int;

	return LDAP_SUCCESS;
}

static int
datamorph_set_signed( ConfigArgs *ca )
{
	transformation_info *info = ca->ca_private;

	if ( !info ) {
		slap_overinst *on = (slap_overinst *)ca->bi;
		datamorph_info *ov = on->on_bi.bi_private;
		info = ov->wip_transformation;
		assert( ca->op == SLAP_CONFIG_ADD );
	}

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_int = info->ti_int.flags & DATAMORPH_FLAG_SIGNED;
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		info->ti_int.flags &= ~DATAMORPH_FLAG_SIGNED;
		return LDAP_SUCCESS;
	}

	info->ti_int.flags &= ~DATAMORPH_FLAG_SIGNED;
	if ( ca->value_int ) {
		info->ti_int.flags |= DATAMORPH_FLAG_SIGNED;
	}

	return LDAP_SUCCESS;
}

static int
datamorph_set_bounds( ConfigArgs *ca )
{
	transformation_info *info = ca->ca_private;
	datamorph_interval_bound *bound;
	uint64_t unsigned_bound;
	int64_t signed_bound;
	char *ptr = ca->value_bv.bv_val + ca->value_bv.bv_len;
	int flag;

	if ( !info ) {
		slap_overinst *on = (slap_overinst *)ca->bi;
		datamorph_info *ov = on->on_bi.bi_private;
		info = ov->wip_transformation;
		assert( ca->op == SLAP_CONFIG_ADD );
	}

	switch ( ca->type ) {
		case DATAMORPH_INT_LOWER:
			bound = &info->ti_int.lower;
			flag = DATAMORPH_FLAG_LOWER;
			break;
		case DATAMORPH_INT_UPPER:
			bound = &info->ti_int.upper;
			flag = DATAMORPH_FLAG_UPPER;
			break;
		default:
			assert(0);
	}

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		char buf[24];
		struct berval bv = { .bv_val = buf };

		if ( !(info->ti_int.flags & flag) ) {
			/* Bound not set, do not emit */
			return LDAP_SUCCESS;
		}
		if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
			bv.bv_len = sprintf( buf, "%" PRId64, bound->i );
		} else {
			bv.bv_len = sprintf( buf, "%" PRIu64, bound->u );
		}
		ber_dupbv_x( &ca->value_bv, &bv, ca->ca_op->o_tmpmemctx );

		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		info->ti_int.flags &= ~flag;
		if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
			bound->i = (flag == DATAMORPH_FLAG_LOWER) ? INT64_MIN : INT64_MAX;
		} else {
			bound->u = (flag == DATAMORPH_FLAG_LOWER) ? 0 : UINT64_MAX;
		}
		return LDAP_SUCCESS;
	}

	/* FIXME: if attributes in the Add operation come in the wrong order
	 * (signed=true after the bound definition), we can't check the interval
	 * sanity. */
	/*
	if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
		signed_bound = strtoll( ca->value_bv.bv_val, &ptr, 10 );
	} else {
		unsigned_bound = strtoull( ca->value_bv.bv_val, &ptr, 10 );
	}
	*/
	/* Also, no idea what happens in the case of big-endian, hopefully,
	 * it behaves the same */
	unsigned_bound = strtoull( ca->value_bv.bv_val, &ptr, 10 );
	signed_bound = (int64_t)unsigned_bound;

	if ( *ca->value_bv.bv_val == '\0' || *ptr != '\0' ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"failed to parse '%s' as integer",
				ca->value_bv.bv_val );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	}
	ch_free( ca->value_bv.bv_val );

	info->ti_int.flags |= flag;
	switch ( info->ti_int.size ) {
		case 1:
			if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
				/* See FIXME above
				if ( signed_bound < INT8_MIN || signed_bound > INT8_MAX ) {
					goto fail;
				}
				*/
			} else {
				/* See FIXME above
				if ( unsigned_bound > UINT8_MAX ) {
					goto fail;
				}
				*/
			}
			break;
		case 2:
			if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
				/* See FIXME above
				if ( signed_bound < INT16_MIN || signed_bound > INT16_MAX ) {
					goto fail;
				}
				*/
			} else {
				/* See FIXME above
				if ( unsigned_bound > UINT16_MAX ) {
					goto fail;
				}
				*/
			}
			break;
		case 4:
			if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
				/* See FIXME above
				if ( signed_bound < INT32_MIN || signed_bound > INT32_MAX ) {
					goto fail;
				}
				*/
			} else {
				/* See FIXME above
				if ( unsigned_bound > UINT32_MAX ) {
					goto fail;
				}
				*/
			}
			break;
		case 8:
			break;
		default:
			/* Should only happen in these two cases:
			 * 1. datamorph_size not yet encountered for this one (when
			 *    processing slapd.conf)
			 * 2. When someone runs a fun modification on the config entry
			 *    messing with more attributes at once
			 *
			 * The error message is expected to be helpful only for the former,
			 * so use the slapd.conf name.
			 */
			snprintf( ca->cr_msg, sizeof(ca->cr_msg),
					"datamorph_size has to be set first!" );
			Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
			ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
			return ca->reply.err;
	}
	if ( info->ti_int.flags & DATAMORPH_FLAG_SIGNED ) {
		bound->i = signed_bound;
	} else {
		bound->u = unsigned_bound;
	}

	return LDAP_SUCCESS;
}

static int
datamorph_set_value( ConfigArgs *ca )
{
	datamorph_enum_mapping *mapping = ca->ca_private;
	char *s = ca->value_bv.bv_val;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		/* We generate the value as part of the RDN, don't add anything */
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		ch_free( mapping->wire_value.bv_val );
		BER_BVZERO( &mapping->wire_value );
		/* TODO: remove from info->ti_enum.to_db? */
		return LDAP_SUCCESS;
	}

	/* As long as this attribute can be in the RDN,
	 * we have to expect the '{n}' prefix */
	if ( *s == '{' ) {
		ber_len_t len;
		s = memchr( s, '}', ca->value_bv.bv_len );
		if ( !s ) {
			ca->reply.err = LDAP_UNDEFINED_TYPE;
			return ca->reply.err;
		}
		s += 1;

		len = ca->value_bv.bv_len - ( s - ca->value_bv.bv_val );
		ber_str2bv( s, len, 1, &mapping->wire_value );
		ch_free( ca->value_bv.bv_val );
	} else {
		mapping->wire_value = ca->value_bv;
	}

	return LDAP_SUCCESS;
}

static int
datamorph_set_index( ConfigArgs *ca )
{
	datamorph_enum_mapping *mapping = ca->ca_private;
	struct berval *from_db = mapping->transformation->ti_enum.from_db;

	if ( ca->op == SLAP_CONFIG_EMIT ) {
		ca->value_int = mapping->db_value;
		return LDAP_SUCCESS;
	} else if ( ca->op == LDAP_MOD_DELETE ) {
		BER_BVZERO( &from_db[mapping->db_value] );
		return LDAP_SUCCESS;
	}

	if ( ca->value_int < 0 || ca->value_int >= 256 ) {
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	} else if ( !BER_BVISNULL( &from_db[ca->value_int] ) ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg), "duplicate index %d",
				ca->value_int );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	}
	mapping->db_value = ca->value_int;
	from_db[ca->value_int] = mapping->wire_value;

	return LDAP_SUCCESS;
}

/* Called when processing slapd.conf only,
 * cn=config uses the objectclass to decide which type we're dealing with.
 */
static int
datamorph_add_transformation( ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	datamorph_info *ov = on->on_bi.bi_private;
	transformation_info *info;

	if ( ov->wip_transformation ) {
		/* We checked everything as were processing the lines */
		int rc = ldap_avl_insert( &ov->transformations, ov->wip_transformation,
				transformation_info_cmp, ldap_avl_dup_error );
		assert( rc == LDAP_SUCCESS );
	}

	info = ch_calloc( 1, sizeof(transformation_info) );
	ov->wip_transformation = ca->ca_private = info;

	if ( !strcasecmp( ca->argv[1], "enum" ) ) {
		info->type = DATAMORPH_ENUM;
	} else if ( !strcasecmp( ca->argv[1], "int" ) ) {
		info->type = DATAMORPH_INT;
	} else {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg),
				"unknown transformation type '%s'", ca->argv[1] );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		ca->reply.err = LDAP_CONSTRAINT_VIOLATION;
		return ca->reply.err;
	}

	ca->value_string = strdup( ca->argv[2] );

	return datamorph_set_attribute( ca );
}

static int
datamorph_add_mapping( ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	datamorph_info *ov = on->on_bi.bi_private;
	transformation_info *info = ov->wip_transformation;
	datamorph_enum_mapping *mapping;
	int rc = LDAP_CONSTRAINT_VIOLATION;

	if ( !info ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg), "no attribute configured" );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		goto done;
	}

	mapping = ch_calloc( 1, sizeof(datamorph_enum_mapping) );
	mapping->transformation = info;
	ca->ca_private = mapping;

	ber_str2bv( ca->argv[2], 0, 1, &ca->value_bv );
	rc = datamorph_set_value( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	rc = lutil_atoix( &ca->value_int, ca->argv[1], 0 );
	if ( rc != LDAP_SUCCESS ) {
		snprintf( ca->cr_msg, sizeof(ca->cr_msg), "invalid integer %s",
				ca->argv[1] );
		Debug( LDAP_DEBUG_ANY, "%s: %s\n", ca->log, ca->cr_msg );
		goto done;
	}

	rc = datamorph_set_index( ca );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

done:
	if ( rc == LDAP_SUCCESS ) {
		rc = ldap_avl_insert( &info->ti_enum.to_db, mapping,
				transformation_mapping_cmp, ldap_avl_dup_error );
	}
	if ( rc ) {
		ca->reply.err = rc;
	}

	return rc;
}

static int
datamorph_ldadd_info_cleanup( ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	datamorph_info *ov = on->on_bi.bi_private;
	transformation_info *info = ca->ca_private;

	if ( ca->reply.err != LDAP_SUCCESS ) {
		/* Not reached since cleanup is only called on success */
fail:
		ch_free( info );
		return LDAP_SUCCESS;
	}

	if ( ldap_avl_insert( &ov->transformations, info, transformation_info_cmp,
			ldap_avl_dup_error ) ) {
		goto fail;
	}
	return LDAP_SUCCESS;
}

static int
datamorph_ldadd_transformation(
		CfEntryInfo *cei,
		Entry *e,
		ConfigArgs *ca,
		datamorph_type type )
{
	transformation_info *info;

	if ( cei->ce_type != Cft_Overlay || !cei->ce_bi ||
			cei->ce_bi->bi_cf_ocs != datamorph_ocs )
		return LDAP_CONSTRAINT_VIOLATION;

	info = ch_calloc( 1, sizeof(transformation_info) );
	info->type = type;

	ca->bi = cei->ce_bi;
	ca->ca_private = info;
	config_push_cleanup( ca, datamorph_ldadd_info_cleanup );
	/* config_push_cleanup is only run in the case of online config but we use it to
	 * enable the new config when done with the entry */
	ca->lineno = 0;

	return LDAP_SUCCESS;
}

static int
datamorph_ldadd_enum( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	return datamorph_ldadd_transformation( cei, e, ca, DATAMORPH_ENUM );
}

static int
datamorph_ldadd_interval( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	return datamorph_ldadd_transformation( cei, e, ca, DATAMORPH_INT );
}

static int
datamorph_ldadd_mapping_cleanup( ConfigArgs *ca )
{
	datamorph_enum_mapping *mapping = ca->ca_private;
	transformation_info *info = mapping->transformation;

	if ( ca->reply.err != LDAP_SUCCESS ) {
		/* Not reached since cleanup is only called on success */
fail:
		datamorph_mapping_free( mapping );
		return LDAP_SUCCESS;
	}

	if ( ldap_avl_insert( &info->ti_enum.to_db, mapping, transformation_mapping_cmp,
			ldap_avl_dup_error ) ) {
		goto fail;
	}
	info->ti_enum.from_db[mapping->db_value] = mapping->wire_value;

	return LDAP_SUCCESS;
}

static int
datamorph_ldadd_mapping( CfEntryInfo *cei, Entry *e, ConfigArgs *ca )
{
	transformation_info *info;
	datamorph_enum_mapping *mapping;
	CfEntryInfo *parent = cei->ce_parent;

	if ( cei->ce_type != Cft_Misc || !parent || !parent->ce_bi ||
			parent->ce_bi->bi_cf_ocs != datamorph_ocs )
		return LDAP_CONSTRAINT_VIOLATION;

	info = cei->ce_private;

	mapping = ch_calloc( 1, sizeof(datamorph_enum_mapping) );
	mapping->transformation = info;

	ca->ca_private = mapping;
	config_push_cleanup( ca, datamorph_ldadd_mapping_cleanup );
	/* config_push_cleanup is only run in the case of online config but we use it to
	 * enable the new config when done with the entry */
	ca->lineno = 0;

	return LDAP_SUCCESS;
}

struct datamorph_cfadd_args {
	Operation *op;
	SlapReply *rs;
	Entry *p;
	ConfigArgs *ca;
	int index;
};

static int
datamorph_config_build_enum( void *item, void *arg )
{
	datamorph_enum_mapping *mapping = item;
	struct datamorph_cfadd_args *args = arg;
	struct berval rdn;
	Entry *e;
	char *p;
	ber_len_t index;

	rdn.bv_len = snprintf( args->ca->cr_msg, sizeof(args->ca->cr_msg),
			"olcDatamorphValue={%d}", args->index++ );
	rdn.bv_val = args->ca->cr_msg;
	p = rdn.bv_val + rdn.bv_len;

	rdn.bv_len += mapping->wire_value.bv_len;
	for ( index = 0; index < mapping->wire_value.bv_len; index++ ) {
		if ( RDN_NEEDSESCAPE(mapping->wire_value.bv_val[index]) ) {
			rdn.bv_len++;
			*p++ = '\\';
		}
		*p++ = mapping->wire_value.bv_val[index];
	}
	*p = '\0';

	args->ca->ca_private = mapping;
	args->ca->ca_op = args->op;
	e = config_build_entry( args->op, args->rs, args->p->e_private, args->ca,
			&rdn, &datamorph_ocs[4], NULL );
	assert( e );

	return LDAP_SUCCESS;
}

static int
datamorph_config_build_attr( void *item, void *arg )
{
	transformation_info *info = item;
	struct datamorph_cfadd_args *args = arg;
	struct berval rdn;
	ConfigOCs *oc;
	Entry *e;

	rdn.bv_len = snprintf( args->ca->cr_msg, sizeof(args->ca->cr_msg),
			"olcDatamorphAttribute={%d}%s", args->index++,
			info->attr->ad_cname.bv_val );
	rdn.bv_val = args->ca->cr_msg;

	switch ( info->type ) {
		case DATAMORPH_ENUM:
			oc = &datamorph_ocs[2];
			break;
		case DATAMORPH_INT:
			oc = &datamorph_ocs[3];
			break;
		default:
			assert(0);
			break;
	}

	args->ca->ca_private = info;
	args->ca->ca_op = args->op;
	e = config_build_entry(
			args->op, args->rs, args->p->e_private, args->ca, &rdn, oc, NULL );
	assert( e );

	if ( info->type == DATAMORPH_ENUM ) {
		struct datamorph_cfadd_args new_args = *args;
		new_args.p = e;
		new_args.index = 0;

		return ldap_avl_apply( info->ti_enum.to_db, datamorph_config_build_enum,
				&new_args, 1, AVL_PREORDER );
	}

	return LDAP_SUCCESS;
}

static int
datamorph_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *ca )
{
	slap_overinst *on = (slap_overinst *)ca->bi;
	datamorph_info *ov = on->on_bi.bi_private;
	struct datamorph_cfadd_args args = {
			.op = op,
			.rs = rs,
			.p = p,
			.ca = ca,
			.index = 0,
	};

	if ( ov->wip_transformation ) {
		/* There is one last item that is unfinished */
		int rc = ldap_avl_insert( &ov->transformations, ov->wip_transformation,
				transformation_info_cmp, ldap_avl_dup_error );
		assert( rc == LDAP_SUCCESS );
	}

	return ldap_avl_apply( ov->transformations, &datamorph_config_build_attr, &args,
			1, AVL_PREORDER );
}

static slap_overinst datamorph;

static int
datamorph_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	datamorph_info *ov;

	/* TODO: can this be global? */
	if ( SLAP_ISGLOBALOVERLAY(be) ) {
		Debug( LDAP_DEBUG_ANY, "datamorph overlay must be instantiated "
				"within a database.\n" );
		return 1;
	}

	ov = ch_calloc( 1, sizeof(datamorph_info) );
	on->on_bi.bi_private = ov;

	return LDAP_SUCCESS;
}

static int
datamorph_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	datamorph_info *ov = on->on_bi.bi_private;

	if ( ov ) {
		ldap_avl_free( ov->transformations, datamorph_info_free );
	}
	ch_free( ov );

	return LDAP_SUCCESS;
}

int
datamorph_initialize()
{
	int rc, i;

	datamorph.on_bi.bi_type = "datamorph";
	datamorph.on_bi.bi_db_init = datamorph_db_init;
	datamorph.on_bi.bi_db_destroy = datamorph_db_destroy;

	datamorph.on_bi.bi_op_add = datamorph_op_add;
	datamorph.on_bi.bi_op_compare = datamorph_op_compare;
	datamorph.on_bi.bi_op_modify = datamorph_op_mod;
	datamorph.on_bi.bi_op_modrdn = datamorph_op_modrdn;
	datamorph.on_bi.bi_op_search = datamorph_op_search;
	datamorph.on_response = datamorph_response;

	datamorph.on_bi.bi_entry_release_rw = datamorph_entry_release_rw;
	datamorph.on_bi.bi_entry_get_rw = datamorph_entry_get_rw;

	datamorph.on_bi.bi_cf_ocs = datamorph_ocs;

	for ( i = 0; datamorph_syntax_defs[i].sd_desc != NULL; i++ ) {
		rc = register_syntax( &datamorph_syntax_defs[i] );

		if ( rc ) {
			Debug( LDAP_DEBUG_ANY, "datamorph_initialize: "
					"error registering syntax %s\n",
					datamorph_syntax_defs[i].sd_desc );
			return rc;
		}
	}

	datamorph_base_syntax = syn_find( DATAMORPH_SYNTAX_BASE );
	assert( datamorph_base_syntax );

	for ( i = 0; datamorph_mrule_defs[i].mrd_desc != NULL; i++ ) {
		rc = register_matching_rule( &datamorph_mrule_defs[i] );

		if ( rc ) {
			Debug( LDAP_DEBUG_ANY, "datamorph_initialize: "
					"error registering matching rule %s\n",
					datamorph_mrule_defs[i].mrd_desc );
			return rc;
		}
	}

	rc = config_register_schema( datamorph_cfg, datamorph_ocs );
	if ( rc ) return rc;

	return overlay_register( &datamorph );
}

#if SLAPD_OVER_DATAMORPH == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return datamorph_initialize();
}
#endif

#endif /* SLAPD_OVER_DATAMORPH */
