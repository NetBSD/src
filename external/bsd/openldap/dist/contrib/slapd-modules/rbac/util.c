/*	$NetBSD: util.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/* util.c - RBAC utility */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 *
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: util.c,v 1.2 2021/08/14 16:14:53 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/string.h>

#include "slap.h"
#include "slap-config.h"
#include "lutil.h"

#include "rbac.h"

#define DELIMITER '$'

#define SUNDAY 0x01
#define MONDAY 0x02
#define TUESDAY 0x04
#define WEDNESDAY 0x08
#define THURSDAY 0x10
#define FRIDAY 0x20
#define SATURDAY 0x40

#define ALL_WEEK "all"

void
rbac_free_constraint( rbac_constraint_t *cp )
{
	if ( !cp ) return;

	if ( !BER_BVISNULL( &cp->name ) ) {
		ch_free( cp->name.bv_val );
	}

	ch_free( cp );
}

void
rbac_free_constraints( rbac_constraint_t *constraints )
{
	rbac_constraint_t *cp, *tmp;

	if ( !constraints ) return;

	tmp = constraints;
	while ( tmp ) {
		cp = tmp->next;
		rbac_free_constraint( tmp );
		tmp = cp;
	}

	return;
}

rbac_constraint_t *
rbac_alloc_constraint()
{
	rbac_constraint_t *cp = NULL;

	cp = ch_calloc( 1, sizeof(rbac_constraint_t) );
	return cp;
}

static int
is_well_formed_constraint( struct berval *bv )
{
	int rc = LDAP_SUCCESS;

	/* assume well-formed role/user-constraints, for the moment */

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "is_well_formed_constraint: "
				"rbac role/user constraint not well-formed: %s\n",
				bv->bv_val );
	}

	return rc;
}

/* input contains 4 digits, representing time */
/* in hhmm format */
static int
constraint_parse_time( char *input )
{
	int btime;
	char *ptr = input;

	btime = ( *ptr++ - '0' ) * 12;
	btime += ( *ptr++ - '0' );
	btime *= 60; /* turning into mins */
	btime += ( *ptr++ - '0' ) * 10;
	btime += ( *ptr++ - '0' );
	btime *= 60; /* turning into secs */

	return btime;
}

/* input contains 4 digits, representing year */
/* in yyyy format */
static int
constraint_parse_year( char *input )
{
	int i;
	int year = 0;
	char *ptr = input;

	for ( i = 0; i <= 3; i++, ptr++ ) {
		year = year * 10 + *ptr - '0';
	}

	return year;
}

/* input contains 2 digits, representing month */
/* in mm format */
static int
constraint_parse_month( char *input )
{
	int i;
	int month = 0;
	char *ptr = input;

	for ( i = 0; i < 2; i++, ptr++ ) {
		month = month * 10 + *ptr - '0';
	}

	return month;
}

/* input contains 2 digits, representing day in month */
/* in dd format */
static int
constraint_parse_day_in_month( char *input )
{
	int i;
	int day_in_month = 0;
	char *ptr = input;

	for ( i = 0; i < 2; i++, ptr++ ) {
		day_in_month = day_in_month * 10 + *ptr - '0';
	}

	return day_in_month;
}

rbac_constraint_t *
rbac_bv2constraint( struct berval *bv )
{
	rbac_constraint_t *cp = NULL;
	int rc = LDAP_SUCCESS;
	char *ptr, *endp = NULL;
	int len = 0;
	int year, month, mday;

	if ( !bv || BER_BVISNULL( bv ) ) goto done;

	rc = is_well_formed_constraint( bv );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	cp = rbac_alloc_constraint();
	if ( !cp ) {
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* constraint name */
	ptr = bv->bv_val;
	endp = ptr;
	while ( *endp != DELIMITER ) {
		endp++;
		len++;
	}

	if ( len > 0 ) {
		cp->name.bv_val = ch_malloc( len + 1 );
		strncpy( cp->name.bv_val, ptr, len );
		cp->name.bv_val[len] = '\0';
		cp->name.bv_len = len;
	} else {
		rc = LDAP_OTHER;
		goto done;
	}

	/* allowed inactivity period */
	ptr = endp;
	endp++;
	if ( isdigit( *endp ) ) {
		int secs = 0;
		while ( isdigit( *endp ) ) {
			secs = secs * 10 + *endp - '0';
			endp++;
		}
		cp->allowed_inactivity = secs;
	} else if ( *endp != DELIMITER ) {
		rc = LDAP_OTHER;
		goto done;
	}

	ptr = endp;
	endp = ptr + 1;

	/* begin time */
	if ( isdigit( *endp ) ) {
		cp->begin_time = constraint_parse_time( endp );
		while ( isdigit( *endp ) )
			endp++;
	}

	ptr = endp;
	while ( *ptr != DELIMITER )
		ptr++;
	endp = ptr + 1;

	/* end time */
	if ( isdigit( *endp ) ) {
		cp->end_time = constraint_parse_time( endp );
		while ( isdigit( *endp ) )
			endp++;
	}

	ptr = endp;
	while ( *ptr != DELIMITER )
		ptr++;
	endp = ptr + 1;

	/* begin year/month/day_in_month */
	if ( isdigit( *endp ) ) {
		lutil_tm tm;
		year = constraint_parse_year( endp );
		endp += 4;
		month = constraint_parse_month( endp );
		endp += 2;
		mday = constraint_parse_day_in_month( endp );
		endp += 2;

		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = mday;
		tm.tm_sec = 0;
		tm.tm_min = 0;
		tm.tm_hour = 0;

		lutil_tm2time( &tm, &cp->begin_date );
	}

	ptr = endp;
	while ( *ptr != DELIMITER )
		ptr++;
	endp = ptr + 1;

	/* end year/month/day_in_month */
	if ( isdigit( *endp ) ) {
		lutil_tm tm;
		year = constraint_parse_year( endp );
		endp += 4;
		month = constraint_parse_month( endp );
		endp += 2;
		mday = constraint_parse_day_in_month( endp );
		endp += 2;

		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = mday;
		tm.tm_sec = 0;
		tm.tm_min = 0;
		tm.tm_hour = 0;

		lutil_tm2time( &tm, &cp->end_date );
	}

	ptr = endp;
	while ( *ptr != DELIMITER )
		ptr++;
	endp = ptr + 1;

	/* begin lock year/month/day_in_month */
	if ( isdigit( *endp ) ) {
		lutil_tm tm;
		year = constraint_parse_year( endp );
		endp += 4;
		month = constraint_parse_month( endp );
		endp += 2;
		mday = constraint_parse_day_in_month( endp );
		endp += 2;

		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = mday;
		tm.tm_sec = 0;
		tm.tm_min = 0;
		tm.tm_hour = 0;

		lutil_tm2time( &tm, &cp->begin_lock_date );
	}

	ptr = endp;
	while ( *ptr != DELIMITER )
		ptr++;
	endp = ptr + 1;

	/* end lock year/month/day_in_month */
	if ( isdigit( *endp ) ) {
		lutil_tm tm;

		year = constraint_parse_year( endp );
		endp += 4;
		month = constraint_parse_month( endp );
		endp += 2;
		mday = constraint_parse_day_in_month( endp );
		endp += 2;

		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = mday;
		tm.tm_sec = 0;
		tm.tm_min = 0;
		tm.tm_hour = 0;

		lutil_tm2time( &tm, &cp->end_lock_date );
	}

	ptr = endp;
	while ( *ptr != DELIMITER )
		ptr++;
	endp = ptr + 1;

	/* dayMask */

	/* allow "all" to mean the entire week */
	if ( strncasecmp( endp, ALL_WEEK, strlen( ALL_WEEK ) ) == 0 ) {
		cp->day_mask = SUNDAY | MONDAY | TUESDAY | WEDNESDAY | THURSDAY |
				FRIDAY | SATURDAY;
	}

	while ( *endp && isdigit( *endp ) ) {
		switch ( *endp - '0' ) {
			case 1:
				cp->day_mask |= SUNDAY;
				break;
			case 2:
				cp->day_mask |= MONDAY;
				break;
			case 3:
				cp->day_mask |= TUESDAY;
				break;
			case 4:
				cp->day_mask |= WEDNESDAY;
				break;
			case 5:
				cp->day_mask |= THURSDAY;
				break;
			case 6:
				cp->day_mask |= FRIDAY;
				break;
			case 7:
				cp->day_mask |= SATURDAY;
				break;
			default:
				/* should not be here */
				rc = LDAP_OTHER;
				goto done;
		}
		endp++;
	}

done:;
	if ( rc != LDAP_SUCCESS ) {
		rbac_free_constraint( cp );
		cp = NULL;
	}

	return cp;
}

static int
constraint_day_of_week( rbac_constraint_t *cp, int wday )
{
	int rc = LDAP_UNWILLING_TO_PERFORM;

	/* assumption: Monday is 1st day of a week */
	switch ( wday ) {
		case 1:
			if ( !(cp->day_mask & MONDAY) ) goto done;
			break;
		case 2:
			if ( !(cp->day_mask & TUESDAY) ) goto done;
			break;
		case 3:
			if ( !(cp->day_mask & WEDNESDAY) ) goto done;
			break;
		case 4:
			if ( !(cp->day_mask & THURSDAY) ) goto done;
			break;
		case 5:
			if ( !(cp->day_mask & FRIDAY) ) goto done;
			break;
		case 6:
			if ( !(cp->day_mask & SATURDAY) ) goto done;
			break;
		case 0:
		case 7:
			if ( !(cp->day_mask & SUNDAY) ) goto done;
			break;
		default:
			/* should not be here */
			goto done;
	}

	rc = LDAP_SUCCESS;

done:;
	return rc;
}

int
rbac_check_time_constraint( rbac_constraint_t *cp )
{
	int rc = LDAP_UNWILLING_TO_PERFORM;
	time_t now;
	struct tm result, *resultp;

	now = slap_get_time();

	/*
	 * does slapd support day-of-week (wday)?
	 * using native routine for now.
	 * Win32's gmtime call is already thread-safe, to the _r
	 * decorator is unneeded.
	 */
#ifdef _WIN32
	resultp = gmtime( &now );
#else
	resultp = gmtime_r( &now, &result );
#endif
	if ( !resultp ) goto done;
#if 0
	timestamp.bv_val = timebuf;
	timestamp.bv_len = sizeof(timebuf);
	slap_timestamp(&now, &timestamp);
	lutil_parsetime(timestamp.bv_val, &now_tm);
	lutil_tm2time(&now_tm, &now_tt);
#endif

	if ( ( cp->begin_date.tt_sec > 0 && cp->begin_date.tt_sec > now ) ||
			( cp->end_date.tt_sec > 0 && cp->end_date.tt_sec < now ) ) {
		/* not within allowed time period */
		goto done;
	}

	/* allowed time period during a day */
	if ( cp->begin_time > 0 && cp->end_time > 0 ) {
		int timeofday = ( resultp->tm_hour * 60 + resultp->tm_min ) * 60 +
				resultp->tm_sec;
		if ( timeofday < cp->begin_time || timeofday > cp->end_time ) {
			/* not within allowed time period in a day */
			goto done;
		}
	}

	/* allowed day in a week */
	if ( cp->day_mask > 0 ) {
		rc = constraint_day_of_week( cp, resultp->tm_wday );
		if ( rc != LDAP_SUCCESS ) goto done;
	}

	/* during lock-out period? */
	if ( ( cp->begin_lock_date.tt_sec > 0 &&
				 cp->begin_lock_date.tt_sec < now ) &&
			( cp->end_lock_date.tt_sec > 0 &&
					cp->end_lock_date.tt_sec > now ) ) {
		/* within locked out period */
		rc = LDAP_UNWILLING_TO_PERFORM;
		goto done;
	}

	/* passed all tests */
	rc = LDAP_SUCCESS;

done:;
	return rc;
}

rbac_constraint_t *
rbac_role2constraint( struct berval *role, rbac_constraint_t *role_constraints )
{
	rbac_constraint_t *cp = NULL;

	if ( !role_constraints || !role ) goto done;

	cp = role_constraints;
	while ( cp ) {
		if ( ber_bvstrcasecmp( role, &cp->name ) == 0 ) {
			/* found the role constraint */
			goto done;
		}
		cp = cp->next;
	}

done:;
	return cp;
}

void
rbac_to_lower( struct berval *bv )
{
	// convert the berval to lower case:
	int i;
	for ( i = 0; i < bv->bv_len; i++ ) {
		bv->bv_val[i] = tolower( bv->bv_val[i] );
	}
}
