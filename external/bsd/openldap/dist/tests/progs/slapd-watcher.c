/*	$NetBSD: slapd-watcher.c,v 1.2 2021/08/14 16:15:03 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2021 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: slapd-watcher.c,v 1.2 2021/08/14 16:15:03 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include "ac/signal.h"
#include "ac/stdlib.h"
#include "ac/time.h"

#include "ac/ctype.h"
#include "ac/param.h"
#include "ac/socket.h"
#include "ac/string.h"
#include "ac/unistd.h"
#include "ac/wait.h"
#include "ac/time.h"

#include "ldap.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "lber_pvt.h"
#include "ldap_pvt.h"

#include "slapd-common.h"

#define SLAP_SYNC_SID_MAX	4095

#define	HAS_MONITOR	1
#define	HAS_BASE	2
#define	HAS_ENTRIES	4
#define	HAS_SREPL	8
#define HAS_ALL (HAS_MONITOR|HAS_BASE|HAS_ENTRIES|HAS_SREPL)


#define WAS_LATE	0x100
#define WAS_DOWN	0x200

#define	MONFILTER	"(objectClass=monitorOperation)"

static const char *default_monfilter = MONFILTER;

typedef enum {
    SLAP_OP_BIND = 0,
    SLAP_OP_UNBIND,
    SLAP_OP_SEARCH,
    SLAP_OP_COMPARE,
    SLAP_OP_MODIFY,
    SLAP_OP_MODRDN,
    SLAP_OP_ADD,
    SLAP_OP_DELETE,
    SLAP_OP_ABANDON,
    SLAP_OP_EXTENDED,
    SLAP_OP_LAST
} slap_op_t;

struct opname {
	struct berval rdn;
	char *display;
} opnames[] = {
	{ BER_BVC("cn=Bind"),		"Bind" },
	{ BER_BVC("cn=Unbind"),		"Unbind" },
	{ BER_BVC("cn=Search"),		"Search" },
	{ BER_BVC("cn=Compare"),	"Compare" },
	{ BER_BVC("cn=Modify"),		"Modify" },
	{ BER_BVC("cn=Modrdn"),		"ModDN" },
	{ BER_BVC("cn=Add"),		"Add" },
	{ BER_BVC("cn=Delete"),		"Delete" },
	{ BER_BVC("cn=Abandon"),	"Abandon" },
	{ BER_BVC("cn=Extended"),	"Extended" },
	{ BER_BVNULL, NULL }
};

typedef struct counters {
	struct timeval time;
	unsigned long entries;
	unsigned long ops[SLAP_OP_LAST];
} counters;

typedef struct csns {
	struct berval *vals;
	struct timeval *tvs;
} csns;

typedef struct activity {
	time_t active;
	time_t idle;
	time_t maxlag;
	time_t lag;
} activity;

typedef struct server {
	char *url;
	LDAP *ld;
	int flags;
	int sid;
	struct berval monitorbase;
	char *monitorfilter;
	time_t late;
	time_t down;
	counters c_prev;
	counters c_curr;
	csns csn_prev;
	csns csn_curr;
	activity *times;
} server;

static void
usage( char *name, char opt )
{
	if ( opt ) {
		fprintf( stderr, "%s: unable to handle option \'%c\'\n\n",
			name, opt );
	}

	fprintf( stderr, "usage: %s "
		"[-D <dn> [ -w <passwd> ]] "
		"[-d <level>] "
		"[-O <SASL secprops>] "
		"[-R <SASL realm>] "
		"[-U <SASL authcid> [-X <SASL authzid>]] "
		"[-x | -Y <SASL mech>] "
		"[-i <interval>] "
		"[-s <sids>] "
		"[-b <baseDN> ] URI[...]\n",
		name );
	exit( EXIT_FAILURE );
}

struct berval base;
int interval = 10;
int numservers;
server *servers;
char *monfilter;

struct berval at_namingContexts = BER_BVC("namingContexts");
struct berval at_monitorOpCompleted = BER_BVC("monitorOpCompleted");
struct berval at_olmMDBEntries = BER_BVC("olmMDBEntries");
struct berval at_contextCSN = BER_BVC("contextCSN");

void timestamp(time_t *tt)
{
	struct tm *tm = gmtime(tt);
	printf("%d-%02d-%02d %02d:%02d:%02d",
		tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void deltat(time_t *tt)
{
	struct tm *tm = gmtime(tt);
	if (tm->tm_mday-1)
		printf("%02d+", tm->tm_mday-1);
	printf("%02d:%02d:%02d",
		tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static char *clearscreen = "\033[H\033[2J";

void rotate_stats( server *sv )
{
	if ( sv->flags & HAS_MONITOR )
		sv->c_prev = sv->c_curr;
	if ( sv->flags & HAS_BASE ) {
		int i;

		for (i=0; i<numservers; i++) {
			if ( sv->csn_curr.vals[i].bv_len ) {
				ber_bvreplace(&sv->csn_prev.vals[i],
					&sv->csn_curr.vals[i]);
				sv->csn_prev.tvs[i] = sv->csn_curr.tvs[i];
			} else {
				if ( sv->csn_prev.vals[i].bv_val )
					sv->csn_prev.vals[i].bv_val[0] = '\0';
			}
		}
	}
}

void display()
{
	int i, j;
	struct timeval now;
	time_t now_t;

	gettimeofday(&now, NULL);
	now_t = now.tv_sec;
	printf("%s", clearscreen);
	timestamp(&now_t);
	printf("\n");

	for (i=0; i<numservers; i++) {
		printf("\n%s", servers[i].url );
		if ( servers[i].flags & WAS_DOWN ) {
			printf(", down@");
			timestamp( &servers[i].down );
		}
		if ( servers[i].flags & WAS_LATE ) {
			printf(", late@");
			timestamp( &servers[i].late );
		}
		printf("\n");
		if ( servers[i].flags & HAS_MONITOR ) {
			struct timeval tv;
			double rate, duration;
			long delta;
			printf("      ");
			if ( servers[i].flags & HAS_ENTRIES )
				printf("  Entries  ");
			for ( j = 0; j<SLAP_OP_LAST; j++ )
				printf(" %9s ", opnames[j].display);
			printf("\n");
			printf("Num   ");
			if ( servers[i].flags & HAS_ENTRIES )
				printf("%10lu ", servers[i].c_curr.entries);
			for ( j = 0; j<SLAP_OP_LAST; j++ )
				printf("%10lu ", servers[i].c_curr.ops[j]);
			printf("\n");
			printf("Num/s ");
			tv.tv_usec = now.tv_usec - servers[i].c_prev.time.tv_usec;
			tv.tv_sec = now.tv_sec - servers[i].c_prev.time.tv_sec;
			if ( tv.tv_usec < 0 ) {
				tv.tv_usec += 1000000;
				tv.tv_sec--;
			}
			duration = tv.tv_sec + (tv.tv_usec / (double)1000000);
			if ( servers[i].flags & HAS_ENTRIES ) {
				delta = servers[i].c_curr.entries - servers[i].c_prev.entries;
				rate = delta / duration;
				printf("%10.2f ", rate);
			}
			for ( j = 0; j<SLAP_OP_LAST; j++ ) {
				delta = servers[i].c_curr.ops[j] - servers[i].c_prev.ops[j];
				rate = delta / duration;
				printf("%10.2f ", rate);
			}
			printf("\n");
		}
		if ( servers[i].flags & HAS_BASE ) {
			for (j=0; j<numservers; j++) {
				/* skip empty CSNs */
				if (!servers[i].csn_curr.vals[j].bv_len ||
					!servers[i].csn_curr.vals[j].bv_val[0])
					continue;
				printf("contextCSN: %s", servers[i].csn_curr.vals[j].bv_val );
				if (ber_bvcmp(&servers[i].csn_curr.vals[j],
							&servers[i].csn_prev.vals[j])) {
					/* a difference */
					if (servers[i].times[j].idle) {
						servers[i].times[j].idle = 0;
						servers[i].times[j].active = 0;
						servers[i].times[j].maxlag = 0;
						servers[i].times[j].lag = 0;
					}
active:
					if (!servers[i].times[j].active)
						servers[i].times[j].active = now_t;
					printf(" actv@");
					timestamp(&servers[i].times[j].active);
				} else if ( servers[i].times[j].lag || ( servers[i].flags & WAS_LATE )) {
					goto active;
				} else {
					if (servers[i].times[j].active && !servers[i].times[j].idle)
						servers[i].times[j].idle = now_t;
					if (servers[i].times[j].active) {
						printf(" actv@");
						timestamp(&servers[i].times[j].active);
						printf(", idle@");
						timestamp(&servers[i].times[j].idle);
					} else {
						printf(" idle");
					}
				}
				if (i != j) {
					if (ber_bvcmp(&servers[i].csn_curr.vals[j],
						&servers[j].csn_curr.vals[j])) {
						struct timeval delta;
						int ahead = 0;
						time_t deltatt;
						delta.tv_sec = servers[j].csn_curr.tvs[j].tv_sec -
							servers[i].csn_curr.tvs[j].tv_sec;
						delta.tv_usec = servers[j].csn_curr.tvs[j].tv_usec -
							servers[i].csn_curr.tvs[j].tv_usec;
						if (delta.tv_usec < 0) {
							delta.tv_usec += 1000000;
							delta.tv_sec--;
						}
						if (delta.tv_sec < 0) {
							delta.tv_sec = -delta.tv_sec;
							ahead = 1;
						}
						deltatt = delta.tv_sec;
						if (ahead)
							printf(", ahead ");
						else
							printf(", behind ");
						deltat( &deltatt );
						servers[i].times[j].lag = deltatt;
						if (deltatt > servers[i].times[j].maxlag)
							servers[i].times[j].maxlag = deltatt;
					} else {
						servers[i].times[j].lag = 0;
						printf(", sync'd");
					}
					if (servers[i].times[j].maxlag) {
						printf(", max delta ");
						deltat( &servers[i].times[j].maxlag );
					}
				}
				printf("\n");
			}
		}
		if ( !( servers[i].flags & WAS_LATE ))
			rotate_stats( &servers[i] );
	}
}

void get_counters(
	LDAP *ld,
	LDAPMessage *e,
	BerElement *ber,
	counters *c )
{
	int rc;
	slap_op_t op = SLAP_OP_BIND;
	struct berval dn, bv, *bvals, **bvp = &bvals;

	do {
		int done = 0;
		for ( rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp );
			rc == LDAP_SUCCESS;
			rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp )) {

			if ( bv.bv_val == NULL ) break;
			if ( !ber_bvcmp( &bv, &at_monitorOpCompleted ) && bvals ) {
				c->ops[op] = strtoul( bvals[0].bv_val, NULL, 0 );
				done = 1;
			}
			if ( bvals ) {
				ber_memfree( bvals );
				bvals = NULL;
			}
			if ( done )
				break;
		}
		ber_free( ber, 0 );
		e = ldap_next_entry( ld, e );
		if ( !e )
			break;
		ldap_get_dn_ber( ld, e, &ber, &dn );
		op++;
	} while ( op < SLAP_OP_LAST );
}

int
slap_parse_csn_sid( struct berval *csnp )
{
	char *p, *q;
	struct berval csn = *csnp;
	int i;

	p = ber_bvchr( &csn, '#' );
	if ( !p )
		return -1;
	p++;
	csn.bv_len -= p - csn.bv_val;
	csn.bv_val = p;

	p = ber_bvchr( &csn, '#' );
	if ( !p )
		return -1;
	p++;
	csn.bv_len -= p - csn.bv_val;
	csn.bv_val = p;

	q = ber_bvchr( &csn, '#' );
	if ( !q )
		return -1;

	csn.bv_len = q - p;

	i = strtol( p, &q, 16 );
	if ( p == q || q != p + csn.bv_len || i < 0 || i > SLAP_SYNC_SID_MAX ) {
		i = -1;
	}

	return i;
}

void get_csns(
	csns *c,
	struct berval *bvs
)
{
	int i, j;

	/* clear old values if any */
	for (i=0; i<numservers; i++)
		if ( c->vals[i].bv_val )
			c->vals[i].bv_val[0] = '\0';

	for (i=0; bvs[i].bv_val; i++) {
		struct lutil_tm tm;
		struct lutil_timet tt;
		int sid = slap_parse_csn_sid( &bvs[i] );
		for (j=0; j<numservers; j++)
			if (sid == servers[j].sid) break;
		if (j < numservers) {
			ber_bvreplace( &c->vals[j], &bvs[i] );
			lutil_parsetime(bvs[i].bv_val, &tm);
			c->tvs[j].tv_usec = tm.tm_nsec / 1000;
			lutil_tm2time( &tm, &tt );
			c->tvs[j].tv_sec = tt.tt_sec;
		}
	}
}

int
setup_server( struct tester_conn_args *config, server *sv, int first )
{
	config->uri = sv->url;
	tester_init_ld( &sv->ld, config, first ? 0 : TESTER_INIT_NOEXIT );
	if ( !sv->ld )
		return -1;

	sv->flags &= ~HAS_ALL;
	{
		char *attrs[] = { at_namingContexts.bv_val, at_monitorOpCompleted.bv_val,
			at_olmMDBEntries.bv_val, NULL };
		LDAPMessage *res = NULL, *e = NULL;
		BerElement *ber = NULL;
		LDAP *ld = sv->ld;
		struct berval dn, bv, *bvals, **bvp = &bvals;
		int j, rc;

		rc = ldap_search_ext_s( ld, "cn=monitor", LDAP_SCOPE_SUBTREE, monfilter,
			attrs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &res );
		switch(rc) {
		case LDAP_SIZELIMIT_EXCEEDED:
		case LDAP_TIMELIMIT_EXCEEDED:
		case LDAP_SUCCESS:
			gettimeofday( &sv->c_curr.time, 0 );
			sv->flags |= HAS_MONITOR;
			for ( e = ldap_first_entry( ld, res ); e; e = ldap_next_entry( ld, e )) {
				ldap_get_dn_ber( ld, e, &ber, &dn );
				if ( !strncasecmp( dn.bv_val, "cn=Database", sizeof("cn=Database")-1 ) ||
					!strncasecmp( dn.bv_val, "cn=Frontend", sizeof("cn=Frontend")-1 )) {
					int matched = 0;
					for ( rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp );
						rc == LDAP_SUCCESS;
						rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp )) {
						if ( bv.bv_val == NULL ) break;
						if (!ber_bvcmp( &bv, &at_namingContexts ) && bvals ) {
							for (j=0; bvals[j].bv_val; j++) {
								if ( !ber_bvstrcasecmp( &base, &bvals[j] )) {
									matched = 1;
									break;
								}
							}
							if (!matched) {
								ber_memfree( bvals );
								bvals = NULL;
								break;
							}
						}
						if (!ber_bvcmp( &bv, &at_olmMDBEntries )) {
							ber_bvreplace( &sv->monitorbase, &dn );
							sv->flags |= HAS_ENTRIES;
							sv->c_curr.entries = strtoul( bvals[0].bv_val, NULL, 0 );
						}
						ber_memfree( bvals );
						bvals = NULL;
					}
				} else if (!strncasecmp( dn.bv_val, opnames[0].rdn.bv_val,
					opnames[0].rdn.bv_len )) {
					get_counters( ld, e, ber, &sv->c_curr );
					break;
				}
				if ( ber )
					ber_free( ber, 0 );
			}
			break;

		case LDAP_NO_SUCH_OBJECT:
			/* no cn=monitor */
			break;

		default:
			tester_ldap_error( ld, "ldap_search_ext_s(cn=Monitor)", sv->url );
			if ( first )
				exit( EXIT_FAILURE );
		}
		ldap_msgfree( res );

		if ( base.bv_val ) {
			char *attr2[] = { at_contextCSN.bv_val, NULL };
			rc = ldap_search_ext_s( ld, base.bv_val, LDAP_SCOPE_BASE, "(objectClass=*)",
				attr2, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &res );
			switch(rc) {
			case LDAP_SUCCESS:
				e = ldap_first_entry( ld, res );
				if ( e ) {
					sv->flags |= HAS_BASE;
					ldap_get_dn_ber( ld, e, &ber, &dn );
					for ( rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp );
						rc == LDAP_SUCCESS;
						rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp )) {
						int done = 0;
						if ( bv.bv_val == NULL ) break;
						if ( bvals ) {
							if ( !ber_bvcmp( &bv, &at_contextCSN )) {
								get_csns( &sv->csn_curr, bvals );
								done = 1;
							}
							ber_memfree( bvals );
							bvals = NULL;
							if ( done )
								break;
						}
					}
				}
				ldap_msgfree( res );
				break;

			default:
				tester_ldap_error( ld, "ldap_search_ext_s(baseDN)", sv->url );
				if ( first )
					exit( EXIT_FAILURE );
			}
		}
	}

	if ( sv->monitorfilter != default_monfilter )
		free( sv->monitorfilter );
	if ( sv->flags & HAS_ENTRIES ) {
		int len = sv->monitorbase.bv_len + sizeof("(|(entryDN=)" MONFILTER ")");
		char *ptr = malloc(len);
		sprintf(ptr, "(|(entryDN=%s)" MONFILTER ")", sv->monitorbase.bv_val );
		sv->monitorfilter = ptr;
	} else if ( sv->flags & HAS_MONITOR ) {
		sv->monitorfilter = (char *)default_monfilter;
	}
	if ( first )
		rotate_stats( sv );
	return 0;
}

int
main( int argc, char **argv )
{
	int		i, rc, *msg1, *msg2;
	char **sids = NULL;
	struct tester_conn_args *config;
	int first = 1;

	config = tester_init( "slapd-watcher", TESTER_TESTER );
	config->authmethod = LDAP_AUTH_SIMPLE;

	while ( ( i = getopt( argc, argv, "D:O:R:U:X:Y:b:d:i:s:w:x" ) ) != EOF )
	{
		switch ( i ) {
		case 'b':		/* base DN for contextCSN lookups */
			ber_str2bv( optarg, 0, 0, &base );
			break;

		case 'i':
			interval = atoi(optarg);
			break;

		case 's':
			sids = ldap_str2charray( optarg, "," );
			break;

		default:
			if ( tester_config_opt( config, i, optarg ) == LDAP_SUCCESS )
				break;

			usage( argv[0], i );
			break;
		}
	}

	tester_config_finish( config );
#ifdef SIGPIPE
	(void) SIGNAL(SIGPIPE, SIG_IGN);
#endif

	/* don't clear the screen if debug is enabled */
	if (debug)
		clearscreen = "\n\n";

	numservers = argc - optind;
	if ( !numservers )
		usage( argv[0], 0 );

	if ( sids ) {
		for (i=0; sids[i]; i++ );
		if ( i != numservers ) {
			fprintf(stderr, "Number of sids doesn't equal number of server URLs\n");
			exit( EXIT_FAILURE );
		}
	}

	argv += optind;
	argc -= optind;
	servers = calloc( numservers, sizeof(server));

	if ( base.bv_val ) {
		monfilter = "(|(entryDN:dnOneLevelMatch:=cn=Databases,cn=Monitor)" MONFILTER ")";
	} else {
		monfilter = MONFILTER;
	}

	if ( numservers > 1 ) {
		for ( i=0; i<numservers; i++ )
			if ( sids )
				servers[i].sid = atoi(sids[i]);
			else
				servers[i].sid = i+1;
	}

	for ( i = 0; i < numservers; i++ ) {
		servers[i].url = argv[i];
		servers[i].times = calloc( numservers, sizeof(activity));
		servers[i].csn_curr.vals = calloc( numservers, sizeof(struct berval));
		servers[i].csn_prev.vals = calloc( numservers, sizeof(struct berval));
		servers[i].csn_curr.tvs = calloc( numservers, sizeof(struct timeval));
		servers[i].csn_prev.tvs = calloc( numservers, sizeof(struct timeval));
	}

	msg1 = malloc( numservers * 2 * sizeof(int));
	msg2 = msg1 + numservers;

	for (;;) {
		LDAPMessage *res = NULL, *e = NULL;
		BerElement *ber = NULL;
		struct berval dn, bv, *bvals, **bvp = &bvals;
		struct timeval tv;
		LDAP *ld;

		for (i=0; i<numservers; i++) {
			if ( !servers[i].ld || !(servers[i].flags & WAS_LATE )) {
				msg1[i] = 0;
				msg2[i] = 0;
			}
			if ( !servers[i].ld ) {
				setup_server( config, &servers[i], first );
			} else {
				ld = servers[i].ld;
				rc = -1;
				if ( servers[i].flags & WAS_DOWN )
					servers[i].flags ^= WAS_DOWN;
				if (( servers[i].flags & HAS_MONITOR ) && !msg1[i] ) {
					char *attrs[3] = { at_monitorOpCompleted.bv_val };
					if ( servers[i].flags & HAS_ENTRIES )
						attrs[1] = at_olmMDBEntries.bv_val;
					rc = ldap_search_ext( ld, "cn=monitor",
						LDAP_SCOPE_SUBTREE, servers[i].monitorfilter,
						attrs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &msg1[i] );
					if ( rc != LDAP_SUCCESS ) {
						tester_ldap_error( ld, "ldap_search_ext(cn=Monitor)", servers[i].url );
						if ( first )
							exit( EXIT_FAILURE );
						else {
server_down1:
							ldap_unbind_ext( ld, NULL, NULL );
							servers[i].flags |= WAS_DOWN;
							servers[i].ld = NULL;
							gettimeofday( &tv, NULL );
							servers[i].down = tv.tv_sec;
							msg1[i] = 0;
							msg2[i] = 0;
							continue;
						}
					}
				}
				if (( servers[i].flags & HAS_BASE ) && !msg2[i] ) {
					char *attrs[2] = { at_contextCSN.bv_val };
					rc = ldap_search_ext( ld, base.bv_val,
						LDAP_SCOPE_BASE, "(objectClass=*)",
						attrs, 0, NULL, NULL, NULL, LDAP_NO_LIMIT, &msg2[i] );
					if ( rc != LDAP_SUCCESS ) {
						tester_ldap_error( ld, "ldap_search_ext(baseDN)", servers[i].url );
						if ( first )
							exit( EXIT_FAILURE );
						else
							goto server_down1;
					}
				}
				if ( rc != -1 )
					gettimeofday( &servers[i].c_curr.time, 0 );
			}
		}

		for (i=0; i<numservers; i++) {
			ld = servers[i].ld;
			if ( msg1[i] ) {
				tv.tv_sec = 0;
				tv.tv_usec = 250000;
				rc = ldap_result( ld, msg1[i], LDAP_MSG_ALL, &tv, &res );
				if ( rc < 0 ) {
					tester_ldap_error( ld, "ldap_result(cn=Monitor)", servers[i].url );
					if ( first )
						exit( EXIT_FAILURE );
					else {
server_down2:
						ldap_unbind_ext( ld, NULL, NULL );
						servers[i].flags |= WAS_DOWN;
						servers[i].ld = NULL;
						servers[i].down = servers[i].c_curr.time.tv_sec;
						msg1[i] = 0;
						msg2[i] = 0;
						continue;
					}
				}
				if ( rc == 0 ) {
					if ( !( servers[i].flags & WAS_LATE ))
						servers[i].late = servers[i].c_curr.time.tv_sec;
					servers[i].flags |= WAS_LATE;
					continue;
				}
				if ( servers[i].flags & WAS_LATE )
					servers[i].flags ^= WAS_LATE;
				for ( e = ldap_first_entry( ld, res ); e; e = ldap_next_entry( ld, e )) {
					ldap_get_dn_ber( ld, e, &ber, &dn );
					if ( !strncasecmp( dn.bv_val, "cn=Database", sizeof("cn=Database")-1 ) ||
						!strncasecmp( dn.bv_val, "cn=Frontend", sizeof("cn=Frontend")-1 )) {
						for ( rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp );
							rc == LDAP_SUCCESS;
							rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp )) {
							if ( bv.bv_val == NULL ) break;
							if ( !ber_bvcmp( &bv, &at_olmMDBEntries )) {
								if ( !BER_BVISNULL( &servers[i].monitorbase )) {
									servers[i].c_curr.entries = strtoul( bvals[0].bv_val, NULL, 0 );
								}
							}
							ber_memfree( bvals );
							bvals = NULL;
						}
					} else if (!strncasecmp( dn.bv_val, opnames[0].rdn.bv_val,
						opnames[0].rdn.bv_len )) {
						get_counters( ld, e, ber, &servers[i].c_curr );
						break;
					}
					if ( ber )
						ber_free( ber, 0 );
				}
				ldap_msgfree( res );
			}
			if ( msg2[i] ) {
				tv.tv_sec = 0;
				tv.tv_usec = 250000;
				rc = ldap_result( ld, msg2[i], LDAP_MSG_ALL, &tv, &res );
				if ( rc < 0 ) {
					tester_ldap_error( ld, "ldap_result(baseDN)", servers[i].url );
					if ( first )
						exit( EXIT_FAILURE );
					else
						goto server_down2;
				}
				if ( rc == 0 ) {
					if ( !( servers[i].flags & WAS_LATE ))
						servers[i].late = servers[i].c_curr.time.tv_sec;
					servers[i].flags |= WAS_LATE;
					continue;
				}
				if ( servers[i].flags & WAS_LATE )
					servers[i].flags ^= WAS_LATE;
				e = ldap_first_entry( ld, res );
				if ( e ) {
					ldap_get_dn_ber( ld, e, &ber, &dn );
					for ( rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp );
						rc == LDAP_SUCCESS;
						rc = ldap_get_attribute_ber( ld, e, ber, &bv, bvp )) {
						int done = 0;
						if ( bv.bv_val == NULL ) break;
						if ( bvals ) {
							if ( !ber_bvcmp( &bv, &at_contextCSN )) {
								get_csns( &servers[i].csn_curr, bvals );
								done = 1;
							}
							ber_memfree( bvals );
							bvals = NULL;
							if ( done )
								break;
						}
					}
				}
				ldap_msgfree( res );
			}
		}
		display();
		sleep(interval);
		first = 0;
	}

	exit( EXIT_SUCCESS );
}

