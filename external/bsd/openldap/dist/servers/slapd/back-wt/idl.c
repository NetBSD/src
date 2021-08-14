/*	$NetBSD: idl.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

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
__RCSID("$NetBSD: idl.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "back-wt.h"
#include "idl.h"

#define IDL_MAX(x,y)	( (x) > (y) ? (x) : (y) )
#define IDL_MIN(x,y)	( (x) < (y) ? (x) : (y) )
#define IDL_CMP(x,y)	( (x) < (y) ? -1 : (x) > (y) )

#if IDL_DEBUG > 0
static void idl_check( ID *ids )
{
	if( WT_IDL_IS_RANGE( ids ) ) {
		assert( WT_IDL_RANGE_FIRST(ids) <= WT_IDL_RANGE_LAST(ids) );
	} else {
		ID i;
		for( i=1; i < ids[0]; i++ ) {
			assert( ids[i+1] > ids[i] );
		}
	}
}

#if IDL_DEBUG > 1
static void idl_dump( ID *ids )
{
	if( WT_IDL_IS_RANGE( ids ) ) {
		Debug( LDAP_DEBUG_ANY,
               "IDL: range ( %ld - %ld )\n",
               (long) WT_IDL_RANGE_FIRST( ids ),
               (long) WT_IDL_RANGE_LAST( ids ) );

	} else {
		ID i;
		Debug( LDAP_DEBUG_ANY, "IDL: size %ld", (long) ids[0] );

		for( i=1; i<=ids[0]; i++ ) {
			if( i % 16 == 1 ) {
				Debug( LDAP_DEBUG_ANY, "\n" );
			}
			Debug( LDAP_DEBUG_ANY, "  %02lx", (long) ids[i] );
		}

		Debug( LDAP_DEBUG_ANY, "\n" );
	}

	idl_check( ids );
}
#endif /* IDL_DEBUG > 1 */
#endif /* IDL_DEBUG > 0 */

unsigned wt_idl_search( ID *ids, ID id )
{
#define IDL_BINARY_SEARCH 1
#ifdef IDL_BINARY_SEARCH
	/*
	 * binary search of id in ids
	 * if found, returns position of id
	 * if not found, returns first position greater than id
	 */
	unsigned base = 0;
	unsigned cursor = 1;
	int val = 0;
	unsigned n = ids[0];

#if IDL_DEBUG > 0
	idl_check( ids );
#endif

	while( 0 < n ) {
		unsigned pivot = n >> 1;
		cursor = base + pivot + 1;
		val = IDL_CMP( id, ids[cursor] );

		if( val < 0 ) {
			n = pivot;

		} else if ( val > 0 ) {
			base = cursor;
			n -= pivot + 1;

		} else {
			return cursor;
		}
	}

	if( val > 0 ) {
		++cursor;
	}
	return cursor;

#else
	/* (reverse) linear search */
	int i;

#if IDL_DEBUG > 0
	idl_check( ids );
#endif

	for( i=ids[0]; i; i-- ) {
		if( id > ids[i] ) {
			break;
		}
	}

	return i+1;
#endif
}

int wt_idl_insert( ID *ids, ID id )
{
	unsigned x;

#if IDL_DEBUG > 1
	Debug( LDAP_DEBUG_ANY, "insert: %04lx at %d\n", (long) id, x );
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	if (WT_IDL_IS_RANGE( ids )) {
		/* if already in range, treat as a dup */
		if (id >= WT_IDL_RANGE_FIRST(ids) && id <= WT_IDL_RANGE_LAST(ids))
			return -1;
		if (id < WT_IDL_RANGE_FIRST(ids))
			ids[1] = id;
		else if (id > WT_IDL_RANGE_LAST(ids))
			ids[2] = id;
		return 0;
	}

	x = wt_idl_search( ids, id );
	assert( x > 0 );

	if( x < 1 ) {
		/* internal error */
		return -2;
	}

	if ( x <= ids[0] && ids[x] == id ) {
		/* duplicate */
		return -1;
	}

	if ( ++ids[0] >= WT_IDL_DB_MAX ) {
		if( id < ids[1] ) {
			ids[1] = id;
			ids[2] = ids[ids[0]-1];
		} else if ( ids[ids[0]-1] < id ) {
			ids[2] = id;
		} else {
			ids[2] = ids[ids[0]-1];
		}
		ids[0] = NOID;

	} else {
		/* insert id */
		AC_MEMCPY( &ids[x+1], &ids[x], (ids[0]-x) * sizeof(ID) );
		ids[x] = id;
	}

#if IDL_DEBUG > 1
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	return 0;
}

static int wt_idl_delete( ID *ids, ID id )
{
	unsigned x;

#if IDL_DEBUG > 1
	Debug( LDAP_DEBUG_ANY, "delete: %04lx at %d\n", (long) id, x );
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	if (WT_IDL_IS_RANGE( ids )) {
		/* If deleting a range boundary, adjust */
		if ( ids[1] == id )
			ids[1]++;
		else if ( ids[2] == id )
			ids[2]--;
		/* deleting from inside a range is a no-op */

		/* If the range has collapsed, re-adjust */
		if ( ids[1] > ids[2] )
			ids[0] = 0;
		else if ( ids[1] == ids[2] )
			ids[1] = 1;
		return 0;
	}

	x = wt_idl_search( ids, id );
	assert( x > 0 );

	if( x <= 0 ) {
		/* internal error */
		return -2;
	}

	if( x > ids[0] || ids[x] != id ) {
		/* not found */
		return -1;

	} else if ( --ids[0] == 0 ) {
		if( x != 1 ) {
			return -3;
		}

	} else {
		AC_MEMCPY( &ids[x], &ids[x+1], (1+ids[0]-x) * sizeof(ID) );
	}

#if IDL_DEBUG > 1
	idl_dump( ids );
#elif IDL_DEBUG > 0
	idl_check( ids );
#endif

	return 0;
}

static char *
wt_show_key(
	char		*buf,
	void		*val,
	size_t		len )
{
	if ( len == 4 /* LUTIL_HASH_BYTES */ ) {
		unsigned char *c = val;
		sprintf( buf, "[%02x%02x%02x%02x]", c[0], c[1], c[2], c[3] );
		return buf;
	} else {
		return val;
	}
}

/*
 * idl_intersection - return a = a intersection b
 */
int
wt_idl_intersection(
	ID *a,
	ID *b )
{
	ID ida, idb;
	ID idmax, idmin;
	ID cursora = 0, cursorb = 0, cursorc;
	int swap = 0;

	if ( WT_IDL_IS_ZERO( a ) || WT_IDL_IS_ZERO( b ) ) {
		a[0] = 0;
		return 0;
	}

	idmin = IDL_MAX( WT_IDL_FIRST(a), WT_IDL_FIRST(b) );
	idmax = IDL_MIN( WT_IDL_LAST(a), WT_IDL_LAST(b) );
	if ( idmin > idmax ) {
		a[0] = 0;
		return 0;
	} else if ( idmin == idmax ) {
		a[0] = 1;
		a[1] = idmin;
		return 0;
	}

	if ( WT_IDL_IS_RANGE( a ) ) {
		if ( WT_IDL_IS_RANGE(b) ) {
		/* If both are ranges, just shrink the boundaries */
			a[1] = idmin;
			a[2] = idmax;
			return 0;
		} else {
		/* Else swap so that b is the range, a is a list */
			ID *tmp = a;
			a = b;
			b = tmp;
			swap = 1;
		}
	}

	/* If a range completely covers the list, the result is
	 * just the list. If idmin to idmax is contiguous, just
	 * turn it into a range.
	 */
	if ( WT_IDL_IS_RANGE( b )
		&& WT_IDL_RANGE_FIRST( b ) <= WT_IDL_FIRST( a )
		&& WT_IDL_RANGE_LAST( b ) >= WT_IDL_LLAST( a ) ) {
		if (idmax - idmin + 1 == a[0])
		{
			a[0] = NOID;
			a[1] = idmin;
			a[2] = idmax;
		}
		goto done;
	}

	/* Fine, do the intersection one element at a time.
	 * First advance to idmin in both IDLs.
	 */
	cursora = cursorb = idmin;
	ida = wt_idl_first( a, &cursora );
	idb = wt_idl_first( b, &cursorb );
	cursorc = 0;

	while( ida <= idmax || idb <= idmax ) {
		if( ida == idb ) {
			a[++cursorc] = ida;
			ida = wt_idl_next( a, &cursora );
			idb = wt_idl_next( b, &cursorb );
		} else if ( ida < idb ) {
			ida = wt_idl_next( a, &cursora );
		} else {
			idb = wt_idl_next( b, &cursorb );
		}
	}
	a[0] = cursorc;
done:
	if (swap)
		WT_IDL_CPY( b, a );

	return 0;
}


/*
 * idl_union - return a = a union b
 */
int
wt_idl_union(
	ID	*a,
	ID	*b )
{
	ID ida, idb;
	ID cursora = 0, cursorb = 0, cursorc;

	if ( WT_IDL_IS_ZERO( b ) ) {
		return 0;
	}

	if ( WT_IDL_IS_ZERO( a ) ) {
		WT_IDL_CPY( a, b );
		return 0;
	}

	if ( WT_IDL_IS_RANGE( a ) || WT_IDL_IS_RANGE(b) ) {
over:		ida = IDL_MIN( WT_IDL_FIRST(a), WT_IDL_FIRST(b) );
		idb = IDL_MAX( WT_IDL_LAST(a), WT_IDL_LAST(b) );
		a[0] = NOID;
		a[1] = ida;
		a[2] = idb;
		return 0;
	}

	ida = wt_idl_first( a, &cursora );
	idb = wt_idl_first( b, &cursorb );

	cursorc = b[0];

	/* The distinct elements of a are cat'd to b */
	while( ida != NOID || idb != NOID ) {
		if ( ida < idb ) {
			if( ++cursorc > WT_IDL_UM_MAX ) {
				goto over;
			}
			b[cursorc] = ida;
			ida = wt_idl_next( a, &cursora );

		} else {
			if ( ida == idb )
				ida = wt_idl_next( a, &cursora );
			idb = wt_idl_next( b, &cursorb );
		}
	}

	/* b is copied back to a in sorted order */
	a[0] = cursorc;
	cursora = 1;
	cursorb = 1;
	cursorc = b[0]+1;
	while (cursorb <= b[0] || cursorc <= a[0]) {
		if (cursorc > a[0])
			idb = NOID;
		else
			idb = b[cursorc];
		if (cursorb <= b[0] && b[cursorb] < idb)
			a[cursora++] = b[cursorb++];
		else {
			a[cursora++] = idb;
			cursorc++;
		}
	}

	return 0;
}


#if 0
/*
 * wt_idl_notin - return a intersection ~b (or a minus b)
 */
int
wt_idl_notin(
	ID	*a,
	ID	*b,
	ID *ids )
{
	ID ida, idb;
	ID cursora = 0, cursorb = 0;

	if( WT_IDL_IS_ZERO( a ) ||
		WT_IDL_IS_ZERO( b ) ||
		WT_IDL_IS_RANGE( b ) )
	{
		WT_IDL_CPY( ids, a );
		return 0;
	}

	if( WT_IDL_IS_RANGE( a ) ) {
		WT_IDL_CPY( ids, a );
		return 0;
	}

	ida = wt_idl_first( a, &cursora ),
	idb = wt_idl_first( b, &cursorb );

	ids[0] = 0;

	while( ida != NOID ) {
		if ( idb == NOID ) {
			/* we could shortcut this */
			ids[++ids[0]] = ida;
			ida = wt_idl_next( a, &cursora );

		} else if ( ida < idb ) {
			ids[++ids[0]] = ida;
			ida = wt_idl_next( a, &cursora );

		} else if ( ida > idb ) {
			idb = wt_idl_next( b, &cursorb );

		} else {
			ida = wt_idl_next( a, &cursora );
			idb = wt_idl_next( b, &cursorb );
		}
	}

	return 0;
}
#endif

ID wt_idl_first( ID *ids, ID *cursor )
{
	ID pos;

	if ( ids[0] == 0 ) {
		*cursor = NOID;
		return NOID;
	}

	if ( WT_IDL_IS_RANGE( ids ) ) {
		if( *cursor < ids[1] ) {
			*cursor = ids[1];
		}
		return *cursor;
	}

	if ( *cursor == 0 )
		pos = 1;
	else
		pos = wt_idl_search( ids, *cursor );

	if( pos > ids[0] ) {
		return NOID;
	}

	*cursor = pos;
	return ids[pos];
}

ID wt_idl_next( ID *ids, ID *cursor )
{
	if ( WT_IDL_IS_RANGE( ids ) ) {
		if( ids[2] < ++(*cursor) ) {
			return NOID;
		}
		return *cursor;
	}

	if ( ++(*cursor) <= ids[0] ) {
		return ids[*cursor];
	}

	return NOID;
}

/* Add one ID to an unsorted list. We ensure that the first element is the
 * minimum and the last element is the maximum, for fast range compaction.
 *   this means IDLs up to length 3 are always sorted...
 */
int wt_idl_append_one( ID *ids, ID id )
{
	if (WT_IDL_IS_RANGE( ids )) {
		/* if already in range, treat as a dup */
		if (id >= WT_IDL_RANGE_FIRST(ids) && id <= WT_IDL_RANGE_LAST(ids))
			return -1;
		if (id < WT_IDL_RANGE_FIRST(ids))
			ids[1] = id;
		else if (id > WT_IDL_RANGE_LAST(ids))
			ids[2] = id;
		return 0;
	}
	if ( ids[0] ) {
		ID tmp;

		if (id < ids[1]) {
			tmp = ids[1];
			ids[1] = id;
			id = tmp;
		}
		if ( ids[0] > 1 && id < ids[ids[0]] ) {
			tmp = ids[ids[0]];
			ids[ids[0]] = id;
			id = tmp;
		}
	}
	ids[0]++;
	if ( ids[0] >= WT_IDL_UM_MAX ) {
		ids[0] = NOID;
		ids[2] = id;
	} else {
		ids[ids[0]] = id;
	}
	return 0;
}

/* Append sorted list b to sorted list a. The result is unsorted but
 * a[1] is the min of the result and a[a[0]] is the max.
 */
int wt_idl_append( ID *a, ID *b )
{
	ID ida, idb, tmp, swap = 0;

	if ( WT_IDL_IS_ZERO( b ) ) {
		return 0;
	}

	if ( WT_IDL_IS_ZERO( a ) ) {
		WT_IDL_CPY( a, b );
		return 0;
	}

	ida = WT_IDL_LAST( a );
	idb = WT_IDL_LAST( b );
	if ( WT_IDL_IS_RANGE( a ) || WT_IDL_IS_RANGE(b) ||
		a[0] + b[0] >= WT_IDL_UM_MAX ) {
		a[2] = IDL_MAX( ida, idb );
		a[1] = IDL_MIN( a[1], b[1] );
		a[0] = NOID;
		return 0;
	}

	if ( b[0] > 1 && ida > idb ) {
		swap = idb;
		a[a[0]] = idb;
		b[b[0]] = ida;
	}

	if ( b[1] < a[1] ) {
		tmp = a[1];
		a[1] = b[1];
	} else {
		tmp = b[1];
	}
	a[0]++;
	a[a[0]] = tmp;

	if ( b[0] > 1 ) {
		int i = b[0] - 1;
		AC_MEMCPY(a+a[0]+1, b+2, i * sizeof(ID));
		a[0] += i;
	}
	if ( swap ) {
		b[b[0]] = swap;
	}
	return 0;
}

#if 1

/* Quicksort + Insertion sort for small arrays */

#define SMALL	8
#define	SWAP(a,b)	itmp=(a);(a)=(b);(b)=itmp

void
wt_idl_sort( ID *ids, ID *tmp )
{
	int *istack = (int *)tmp; /* Private stack, not used by caller */
	int i,j,k,l,ir,jstack;
	ID a, itmp;

	if ( WT_IDL_IS_RANGE( ids ))
		return;

	ir = ids[0];
	l = 1;
	jstack = 0;
	for(;;) {
		if (ir - l < SMALL) {	/* Insertion sort */
			for (j=l+1;j<=ir;j++) {
				a = ids[j];
				for (i=j-1;i>=1;i--) {
					if (ids[i] <= a) break;
					ids[i+1] = ids[i];
				}
				ids[i+1] = a;
			}
			if (jstack == 0) break;
			ir = istack[jstack--];
			l = istack[jstack--];
		} else {
			k = (l + ir) >> 1;	/* Choose median of left, center, right */
			SWAP(ids[k], ids[l+1]);
			if (ids[l] > ids[ir]) {
				SWAP(ids[l], ids[ir]);
			}
			if (ids[l+1] > ids[ir]) {
				SWAP(ids[l+1], ids[ir]);
			}
			if (ids[l] > ids[l+1]) {
				SWAP(ids[l], ids[l+1]);
			}
			i = l+1;
			j = ir;
			a = ids[l+1];
			for(;;) {
				do i++; while(ids[i] < a);
				do j--; while(ids[j] > a);
				if (j < i) break;
				SWAP(ids[i],ids[j]);
			}
			ids[l+1] = ids[j];
			ids[j] = a;
			jstack += 2;
			if (ir-i+1 >= j-l) {
				istack[jstack] = ir;
				istack[jstack-1] = i;
				ir = j-1;
			} else {
				istack[jstack] = j-1;
				istack[jstack-1] = l;
				l = i;
			}
		}
	}
}

#else

/* 8 bit Radix sort + insertion sort
 *
 * based on code from http://www.cubic.org/docs/radix.htm
 * with improvements by ebackes@symas.com and hyc@symas.com
 *
 * This code is O(n) but has a relatively high constant factor. For lists
 * up to ~50 Quicksort is slightly faster; up to ~100 they are even.
 * Much faster than quicksort for lists longer than ~100. Insertion
 * sort is actually superior for lists <50.
 */

#define BUCKETS	(1<<8)
#define SMALL	50

void
wt_idl_sort( ID *ids, ID *tmp )
{
	int count, soft_limit, phase = 0, size = ids[0];
	ID *idls[2];
	unsigned char *maxv = (unsigned char *)&ids[size];

	if ( WT_IDL_IS_RANGE( ids ))
		return;

	/* Use insertion sort for small lists */
	if ( size <= SMALL ) {
		int i,j;
		ID a;

		for (j=1;j<=size;j++) {
			a = ids[j];
			for (i=j-1;i>=1;i--) {
				if (ids[i] <= a) break;
				ids[i+1] = ids[i];
			}
			ids[i+1] = a;
		}
		return;
	}

	tmp[0] = size;
	idls[0] = ids;
	idls[1] = tmp;

#if BYTE_ORDER == BIG_ENDIAN
    for (soft_limit = 0; !maxv[soft_limit]; soft_limit++);
#else
    for (soft_limit = sizeof(ID)-1; !maxv[soft_limit]; soft_limit--);
#endif

	for (
#if BYTE_ORDER == BIG_ENDIAN
	count = sizeof(ID)-1; count >= soft_limit; --count
#else
	count = 0; count <= soft_limit; ++count
#endif
	) {
		unsigned int num[BUCKETS], * np, n, sum;
		int i;
        ID *sp, *source, *dest;
        unsigned char *bp, *source_start;

		source = idls[phase]+1;
		dest = idls[phase^1]+1;
		source_start =  ((unsigned char *) source) + count;

        np = num;
        for ( i = BUCKETS; i > 0; --i ) *np++ = 0;

		/* count occurrences of every byte value */
		bp = source_start;
        for ( i = size; i > 0; --i, bp += sizeof(ID) )
				num[*bp]++;

		/* transform count into index by summing elements and storing
		 * into same array
		 */
        sum = 0;
        np = num;
        for ( i = BUCKETS; i > 0; --i ) {
                n = *np;
                *np++ = sum;
                sum += n;
        }

		/* fill dest with the right values in the right place */
		bp = source_start;
        sp = source;
        for ( i = size; i > 0; --i, bp += sizeof(ID) ) {
                np = num + *bp;
                dest[*np] = *sp++;
                ++(*np);
        }
		phase ^= 1;
	}

	/* copy back from temp if needed */
	if ( phase ) {
		ids++; tmp++;
		for ( count = 0; count < size; ++count )
			*ids++ = *tmp++;
	}
}
#endif	/* Quick vs Radix */

