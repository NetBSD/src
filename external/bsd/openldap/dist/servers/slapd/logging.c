/*	$NetBSD: logging.c,v 1.2 2025/09/05 21:16:25 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2021-2024 The OpenLDAP Foundation.
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
__RCSID("$NetBSD: logging.c,v 1.2 2025/09/05 21:16:25 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/param.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/ctype.h>

#include <sys/stat.h>
#ifndef _WIN32
#include <sys/uio.h>
#endif
#include <fcntl.h>

#include "slap.h"
#include "ldif.h"

#include "slap-config.h"
#include "slap-cfglog.h"

static int config_syslog, active_syslog;

static char logfile_suffix[sizeof(".xx.gz")];
static char logfile_path[MAXPATHLEN - sizeof(logfile_suffix) -1];
static long logfile_fslimit;
static int logfile_age, logfile_only, logfile_max;
static char *syslog_prefix;
static int splen;
static int logfile_rotfail, logfile_openfail;

typedef enum { LFMT_DEBUG, LFMT_SYSLOG, LFMT_RFC3339 } LogFormat;
static LogFormat logfile_format;

#define LFMT_LOCALTIME 0x80
#define LFMT_DEFAULT	LFMT_DEBUG
#define LFMT_SYSLOG_LOCAL	(LFMT_SYSLOG|LFMT_LOCALTIME)
#define LFMT_SYSLOG_UTC	(LFMT_SYSLOG)
#define LFMT_RFC3339_UTC	(LFMT_RFC3339)

static slap_verbmasks logformat_key[] = {
	{ BER_BVC("default"),		LFMT_DEFAULT },
	{ BER_BVC("debug"),			LFMT_DEBUG },
	{ BER_BVC("syslog-utc"),	LFMT_SYSLOG_UTC },
	{ BER_BVC("syslog-localtime"),		LFMT_SYSLOG_LOCAL },
	{ BER_BVC("rfc3339-utc"),		LFMT_RFC3339_UTC },
	{ BER_BVNULL, 0 }
};

char *serverName;
int slap_debug_orig;

ldap_pvt_thread_mutex_t logfile_mutex;

static off_t logfile_fsize;
static time_t logfile_fcreated;
static int logfile_fd = -1;
static char logpaths[2][MAXPATHLEN];
static int logpathlen;

#define SYSLOG_STAMP	"Mmm dd hh:mm:ss"
#ifdef HAVE_CLOCK_GETTIME
#define RFC3339_FRAC	".fffffffffZ"
#else
#define RFC3339_FRAC	".ffffffZ"
#endif
#define RFC3339_BASE	"YYYY-mm-ddTHH:MM:SS"
#define RFC3339_STAMP	 RFC3339_BASE RFC3339_FRAC

void
slap_debug_print( const char *data )
{
#ifdef _WIN32
	char msgbuf[4096];
	int prefixlen, poffset = 0, datalen;
#else
	char prefix[sizeof("ssssssssssssssss.ffffffff 0xtttttttttttttttt ")];
	struct iovec iov[2];
#endif
	int rotate = 0;
#ifdef HAVE_CLOCK_GETTIME
	struct timespec tv;
#define	TS	"%08x"
#define	TSf	".%09ldZ"
#define	Tfrac	tv.tv_nsec
#define gettime(tv)	clock_gettime( CLOCK_REALTIME, tv )
#else
	struct timeval tv;
#define	TS	"%05x"
#define	TSf	".%06ldZ"
#define	Tfrac	tv.tv_usec
#define	gettime(tv)	gettimeofday( tv, NULL )
#endif
	char *ptr;
	int len;


	gettime( &tv );
#ifdef _WIN32
	ptr = msgbuf;
	prefixlen = sprintf( ptr, "%lx." TS " %p ",
		(long)tv.tv_sec, (unsigned int)Tfrac, (void *)ldap_pvt_thread_self() );
	if ( prefixlen < splen ) {
		poffset = splen - prefixlen;
		AC_MEMCPY( ptr+poffset, ptr, prefixlen );
	}

	ptr = lutil_strncopy( ptr+poffset+prefixlen, data, sizeof(msgbuf) - prefixlen);
	len = ptr - msgbuf - poffset;
	datalen = len - prefixlen;
	if ( !logfile_only )
		(void)!write( 2, msgbuf+poffset, len );
	ptr = msgbuf;
#else
	iov[0].iov_base = prefix;
	iov[0].iov_len = sprintf( prefix, "%lx." TS " %p ",
		(long)tv.tv_sec, (unsigned int)Tfrac, (void *)ldap_pvt_thread_self() );
	iov[1].iov_base = (void *)data;
	iov[1].iov_len = strlen( data );
	len = iov[0].iov_len + iov[1].iov_len;
	if ( !logfile_only )
		(void)!writev( 2, iov, 2 );
#endif
	if ( logfile_fd >= 0 ) {
		if ( logfile_fslimit || logfile_age ) {
			ldap_pvt_thread_mutex_lock( &logfile_mutex );
			if ( logfile_fslimit && logfile_fsize + len > logfile_fslimit )
				rotate = 1;
			if ( logfile_age && tv.tv_sec - logfile_fcreated >= logfile_age )
				rotate |= 2;
			if ( rotate ) {
				int rc, savefd;
				strcpy( logpaths[0]+logpathlen, ".tmp" );
				if ( rename( logfile_path, logpaths[0] )) {
					rc = errno;
					if ( !logfile_rotfail ) {
						char buf[BUFSIZ];
						char ebuf[128];
						int len = snprintf(buf, sizeof( buf ), "ERROR! logfile rotate failure, err=%d \"%s\"\n",
							rc, AC_STRERROR_R( rc, ebuf, sizeof(ebuf) ));
						if ( !logfile_only )
							!write( 2, buf, len );
						!write( logfile_fd, buf, len );
						logfile_rotfail = 1;
					}
					rotate = 0;	/* don't bother since it will fail */
				} else {
					logfile_rotfail = 0;
				}
				savefd = logfile_fd;
				logfile_fd = -1;
				if (( rc = logfile_open( logfile_path ))) {
					logfile_fd = savefd;
					if ( !logfile_openfail ) {
						char buf[BUFSIZ];
						char ebuf[128];
						int len = snprintf(buf, sizeof( buf ), "ERROR! logfile couldn't be reopened, err=%d \"%s\"\n",
							rc, AC_STRERROR_R( rc, ebuf, sizeof(ebuf) ));
						if ( !logfile_only )
							!write( 2, buf, len );
						!write( logfile_fd, buf, len );
						logfile_openfail = 1;
					}
				} else {
					close( savefd );
					logfile_openfail = 0;
				}
			}
		}

		if ( logfile_format > LFMT_DEBUG ) {
			struct tm tm;
			if ( !( logfile_format & LFMT_LOCALTIME ) )
				ldap_pvt_gmtime( &tv.tv_sec, &tm );
			else
				ldap_pvt_localtime( &tv.tv_sec, &tm );
#ifdef _WIN32
			if ( splen < prefixlen )
				ptr += prefixlen - splen;
			memcpy( ptr, syslog_prefix, splen );
#else
			ptr = syslog_prefix;
#endif
			if ( logfile_format & LFMT_SYSLOG ) {
				ptr += strftime( ptr, sizeof( SYSLOG_STAMP ),
					"%b %d %H:%M:%S", &tm );
			}	else {
				ptr += strftime( ptr, sizeof( RFC3339_BASE ),
					"%Y-%m-%dT%H:%M:%S", &tm );
				ptr += snprintf( ptr, sizeof( RFC3339_FRAC ), TSf, Tfrac );
			}
			*ptr = ' ';
#ifdef _WIN32
			len = datalen + splen;
#else
			iov[0].iov_base = syslog_prefix;
			iov[0].iov_len = splen;
#endif
		}

#ifdef _WIN32
		if ( logfile_format <= LFMT_DEBUG )
			ptr += poffset;	/* only nonzero if logfile-format was explicitly set */
		len = write( logfile_fd, ptr, len );
#else
		len = writev( logfile_fd, iov, 2 );
#endif
		if ( len > 0 )
			logfile_fsize += len;
		if ( logfile_fslimit || logfile_age )
			ldap_pvt_thread_mutex_unlock( &logfile_mutex );
	}
	if ( rotate ) {
		int i;
		for (i=logfile_max; i > 1; i--) {
			sprintf( logpaths[0]+logpathlen, ".%02d", i );
			sprintf( logpaths[1]+logpathlen, ".%02d", i-1 );
			rename( logpaths[1], logpaths[0] );
		}
		sprintf( logpaths[0]+logpathlen, ".tmp" );
		rename( logpaths[0], logpaths[1] );
	}
}

void
logfile_close()
{
	if ( logfile_fd >= 0 ) {
		close( logfile_fd );
		logfile_fd = -1;
	}
	logfile_path[0] = '\0';
}

int
logfile_open( const char *path )
{
	struct stat st;
	int fd, saved_errno;

	/* the logfile is for slapd only, not tools */
	if ( !( slapMode & SLAP_SERVER_MODE ))
		return 0;

	fd = open( path, O_CREAT|O_WRONLY|O_APPEND, 0640 );
	if ( fd < 0 ) {
		saved_errno = errno;
fail:
		logfile_only = 0;	/* make sure something gets output */
		return saved_errno;
	}

	if ( fstat( fd, &st ) ) {
		saved_errno = errno;
		close( fd );
		goto fail;
	}

	if ( !logfile_path[0] ) {
		logpathlen = strlen( path );
		if ( logpathlen >= sizeof(logfile_path) ) {
			saved_errno = ENAMETOOLONG;
			goto fail;
		}
		strcpy( logfile_path, path );
		strcpy( logpaths[0], path );
		strcpy( logpaths[1], path );
	}

	logfile_fsize = st.st_size;
	logfile_fcreated = st.st_ctime;	/* not strictly true but close enough */
	logfile_fd = fd;

	return 0;
}

const char *
logfile_name()
{
	return logfile_path[0] ? logfile_path : NULL;
}

#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
#ifdef LOG_LOCAL4
int
slap_parse_syslog_user( const char *arg, int *syslogUser )
{
	static slap_verbmasks syslogUsers[] = {
		{ BER_BVC( "LOCAL0" ), LOG_LOCAL0 },
		{ BER_BVC( "LOCAL1" ), LOG_LOCAL1 },
		{ BER_BVC( "LOCAL2" ), LOG_LOCAL2 },
		{ BER_BVC( "LOCAL3" ), LOG_LOCAL3 },
		{ BER_BVC( "LOCAL4" ), LOG_LOCAL4 },
		{ BER_BVC( "LOCAL5" ), LOG_LOCAL5 },
		{ BER_BVC( "LOCAL6" ), LOG_LOCAL6 },
		{ BER_BVC( "LOCAL7" ), LOG_LOCAL7 },
#ifdef LOG_USER
		{ BER_BVC( "USER" ), LOG_USER },
#endif /* LOG_USER */
#ifdef LOG_DAEMON
		{ BER_BVC( "DAEMON" ), LOG_DAEMON },
#endif /* LOG_DAEMON */
		{ BER_BVNULL, 0 }
	};
	int i = verb_to_mask( arg, syslogUsers );

	if ( BER_BVISNULL( &syslogUsers[ i ].word ) ) {
		Debug( LDAP_DEBUG_ANY,
			"unrecognized syslog user \"%s\".\n",
			arg );
		return 1;
	}

	*syslogUser = syslogUsers[ i ].mask;

	return 0;
}
#endif /* LOG_LOCAL4 */

int
slap_parse_syslog_level( const char *arg, int *levelp )
{
	static slap_verbmasks	str2syslog_level[] = {
		{ BER_BVC( "EMERG" ),	LOG_EMERG },
		{ BER_BVC( "ALERT" ),	LOG_ALERT },
		{ BER_BVC( "CRIT" ),	LOG_CRIT },
		{ BER_BVC( "ERR" ),	LOG_ERR },
		{ BER_BVC( "WARNING" ),	LOG_WARNING },
		{ BER_BVC( "NOTICE" ),	LOG_NOTICE },
		{ BER_BVC( "INFO" ),	LOG_INFO },
		{ BER_BVC( "DEBUG" ),	LOG_DEBUG },
		{ BER_BVNULL, 0 }
	};
	int i = verb_to_mask( arg, str2syslog_level );
	if ( BER_BVISNULL( &str2syslog_level[ i ].word ) ) {
		Debug( LDAP_DEBUG_ANY,
			"unknown syslog level \"%s\".\n",
			arg );
		return 1;
	}

	*levelp = str2syslog_level[ i ].mask;

	return 0;
}
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

static char **debug_unknowns;
static char **syslog_unknowns;

static int
parse_debug_unknowns( char **unknowns, int *levelp )
{
	int i, level, rc = 0;

	for ( i = 0; unknowns[ i ] != NULL; i++ ) {
		level = 0;
		if ( str2loglevel( unknowns[ i ], &level )) {
			fprintf( stderr,
				"unrecognized log level \"%s\"\n", unknowns[ i ] );
			rc = 1;
		} else {
			*levelp |= level;
		}
	}
	return rc;
}

int
slap_parse_debug_level( const char *arg, int *levelp, int which )
{
	int	level;

	if ( arg && arg[ 0 ] != '-' && !isdigit( (unsigned char) arg[ 0 ] ) )
	{
		int	i;
		char	**levels;
		char	***unknowns = which ? &syslog_unknowns : &debug_unknowns;

		levels = ldap_str2charray( arg, "," );

		for ( i = 0; levels[ i ] != NULL; i++ ) {
			level = 0;

			if ( str2loglevel( levels[ i ], &level ) ) {
				/* remember this for later */
				ldap_charray_add( unknowns, levels[ i ] );
				fprintf( stderr,
					"unrecognized log level \"%s\" (deferred)\n",
					levels[ i ] );
			} else {
				*levelp |= level;
			}
		}

		ldap_charray_free( levels );

	} else {
		int rc;

		if ( arg[0] == '-' ) {
			rc = lutil_atoix( &level, arg, 0 );
		} else {
			unsigned ulevel;

			rc = lutil_atoux( &ulevel, arg, 0 );
			level = (int)ulevel;
		}

		if ( rc ) {
			fprintf( stderr,
				"unrecognized log level "
				"\"%s\"\n", arg );
			return 1;
		}

		if ( level == 0 ) {
			*levelp = 0;

		} else {
			*levelp |= level;
		}
	}

	return 0;
}

int
slap_parse_debug_unknowns() {
	int rc = 0;
	if ( debug_unknowns ) {
		rc = parse_debug_unknowns( debug_unknowns, &slap_debug );
		ldap_charray_free( debug_unknowns );
		debug_unknowns = NULL;
		if ( rc )
			goto leave;
		ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &slap_debug );
		ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &slap_debug );
	}
	if ( syslog_unknowns ) {
		rc = parse_debug_unknowns( syslog_unknowns, &ldap_syslog );
		ldap_charray_free( syslog_unknowns );
		syslog_unknowns = NULL;
	}
leave:
	return rc;
}

void slap_check_unknown_level( char *levelstr, int level )
{
	int i;

	if ( debug_unknowns ) {
		for ( i = 0; debug_unknowns[ i ]; i++ ) {
			if ( !strcasecmp( debug_unknowns[ i ], levelstr )) {
				slap_debug |= level;
				break;
			}
		}
	}

	if ( syslog_unknowns ) {
		for ( i = 0; syslog_unknowns[ i ]; i++ ) {
			if ( !strcasecmp( syslog_unknowns[ i ], levelstr )) {
				ldap_syslog |= level;
				break;
			}
		}
	}
}

static slap_verbmasks	*loglevel_ops;

static int
loglevel_init( void )
{
	slap_verbmasks	lo[] = {
		{ BER_BVC("Any"),	(slap_mask_t) LDAP_DEBUG_ANY },
		{ BER_BVC("Trace"),	LDAP_DEBUG_TRACE },
		{ BER_BVC("Packets"),	LDAP_DEBUG_PACKETS },
		{ BER_BVC("Args"),	LDAP_DEBUG_ARGS },
		{ BER_BVC("Conns"),	LDAP_DEBUG_CONNS },
		{ BER_BVC("BER"),	LDAP_DEBUG_BER },
		{ BER_BVC("Filter"),	LDAP_DEBUG_FILTER },
		{ BER_BVC("Config"),	LDAP_DEBUG_CONFIG },
		{ BER_BVC("ACL"),	LDAP_DEBUG_ACL },
		{ BER_BVC("Stats"),	LDAP_DEBUG_STATS },
		{ BER_BVC("Stats2"),	LDAP_DEBUG_STATS2 },
		{ BER_BVC("Shell"),	LDAP_DEBUG_SHELL },
		{ BER_BVC("Parse"),	LDAP_DEBUG_PARSE },
#if 0	/* no longer used (nor supported) */
		{ BER_BVC("Cache"),	LDAP_DEBUG_CACHE },
		{ BER_BVC("Index"),	LDAP_DEBUG_INDEX },
#endif
		{ BER_BVC("Sync"),	LDAP_DEBUG_SYNC },
		{ BER_BVC("None"),	LDAP_DEBUG_NONE },
		{ BER_BVNULL,		0 }
	};

	return slap_verbmasks_init( &loglevel_ops, lo );
}

void
slap_loglevel_destroy( void )
{
	if ( loglevel_ops ) {
		(void)slap_verbmasks_destroy( loglevel_ops );
	}
	loglevel_ops = NULL;
}

static slap_mask_t	loglevel_ignore[] = { -1, 0 };

int
slap_loglevel_get( struct berval *s, int *l )
{
	int		rc;
	slap_mask_t	m, i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	for ( m = 0, i = 1; !BER_BVISNULL( &loglevel_ops[ i ].word ); i++ ) {
		m |= loglevel_ops[ i ].mask;
	}

	for ( i = 1; m & i; i <<= 1 )
		;

	if ( i == 0 ) {
		return -1;
	}

	rc = slap_verbmasks_append( &loglevel_ops, i, s, loglevel_ignore );

	if ( rc != 0 ) {
		Debug( LDAP_DEBUG_ANY, "slap_loglevel_get(%lu, \"%s\") failed\n",
			i, s->bv_val );

	} else {
		*l = i;
		slap_check_unknown_level( s->bv_val, i );
	}

	return rc;
}

int
slap_syslog_get()
{
	return active_syslog;
}

void
slap_syslog_set( int l )
{
	active_syslog = l;
	if ( logfile_only ) {
		slap_debug |= active_syslog;
		ldap_syslog = 0;
	} else {
		ldap_syslog = active_syslog;
	}
}

int
slap_debug_get()
{
	return slap_debug_orig;
}

void
slap_debug_set( int l )
{
	slap_debug_orig = l;
	if ( logfile_only )
		slap_debug = slap_debug_orig | active_syslog;
	else
		slap_debug = slap_debug_orig;
	ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &slap_debug);
	ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &slap_debug);
	ldif_debug = slap_debug;
}

int
str2loglevel( const char *s, int *l )
{
	int	i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	i = verb_to_mask( s, loglevel_ops );

	if ( BER_BVISNULL( &loglevel_ops[ i ].word ) ) {
		return -1;
	}

	*l = loglevel_ops[ i ].mask;

	return 0;
}

const char *
loglevel2str( int l )
{
	struct berval	bv = BER_BVNULL;

	loglevel2bv( l, &bv );

	return bv.bv_val;
}

int
loglevel2bv( int l, struct berval *bv )
{
	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	BER_BVZERO( bv );

	return enum_to_verb( loglevel_ops, l, bv ) == -1;
}

int
loglevel2bvarray( int l, BerVarray *bva )
{
	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	if ( l == 0 ) {
		struct berval bv = BER_BVC("0");
		return value_add_one( bva, &bv );
	}

	return mask_to_verbs( loglevel_ops, l, bva );
}

int
loglevel_print( FILE *out )
{
	int	i;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	fprintf( out, "Installed log subsystems:\n\n" );
	for ( i = 0; !BER_BVISNULL( &loglevel_ops[ i ].word ); i++ ) {
		unsigned mask = loglevel_ops[ i ].mask & 0xffffffffUL;
		fprintf( out,
			(mask == ((slap_mask_t) -1 & 0xffffffffUL)
			 ? "\t%-30s (-1, 0xffffffff)\n" : "\t%-30s (%u, 0x%x)\n"),
			loglevel_ops[ i ].word.bv_val, mask, mask );
	}

	fprintf( out, "\nNOTE: custom log subsystems may be later installed "
		"by specific code\n\n" );

	return 0;
}

int
config_logging(ConfigArgs *c) {
	int i, rc = 0;

	if ( loglevel_ops == NULL ) {
		loglevel_init();
	}

	if (c->op == SLAP_CONFIG_EMIT) {
		switch(c->type) {
		case CFG_LOGLEVEL:
			/* Get default or commandline slapd setting */
			if ( ldap_syslog && !config_syslog )
				config_syslog = ldap_syslog;
			rc = loglevel2bvarray( config_syslog, &c->rvalue_vals );
			break;

		case CFG_LOGFILE: {
			const char *logfileName = logfile_name();
			if ( logfileName && *logfileName )
				c->value_string = ch_strdup( logfileName );
			else
				rc = 1;
			}
			break;
		case CFG_LOGFILE_FORMAT:
			if ( logfile_format ) {
				value_add_one( &c->rvalue_vals, &logformat_key[logfile_format].word );
			} else {
				rc = 1;
			}
			break;
		case CFG_LOGFILE_ONLY:
			c->value_int = logfile_only;
			break;
		case CFG_LOGFILE_ROTATE:
			rc = 1;
			if ( logfile_max ) {
				char buf[64];
				struct berval bv;
				bv.bv_len = snprintf( buf, sizeof(buf), "%d %ld %ld", logfile_max,
					(long) logfile_fslimit / 1048576, (long) logfile_age / 3600 );
				if ( bv.bv_len > 0 && bv.bv_len < sizeof(buf) ) {
					bv.bv_val = buf;
					value_add_one( &c->rvalue_vals, &bv );
					rc = 0;
				}
			}
			break;
		default:
			rc = 1;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch(c->type) {
		case CFG_LOGLEVEL:
			if ( !c->line ) {
				config_syslog = 0;
			} else {
				i = verb_to_mask( c->line, loglevel_ops );
				config_syslog &= ~loglevel_ops[i].mask;
			}
			goto reset;

		case CFG_LOGFILE:
			logfile_close();
			break;

		case CFG_LOGFILE_FORMAT:
			logfile_format = 0;
			ch_free( syslog_prefix );
			syslog_prefix = NULL;
			break;

		case CFG_LOGFILE_ONLY:
			/* remove loglevel from debuglevel */
			slap_debug = slap_debug_orig;
			ldap_syslog = config_syslog;
			break;

		case CFG_LOGFILE_ROTATE:
			logfile_max = logfile_fslimit = logfile_age = 0;
			break;
		default:
			rc = 1;
		}
		return rc;
	}

	switch(c->type) {
		case CFG_LOGLEVEL:
			for( i=1; i < c->argc; i++ ) {
				int	level;

				if ( isdigit((unsigned char)c->argv[i][0]) || c->argv[i][0] == '-' ) {
					if( lutil_atoix( &level, c->argv[i], 0 ) != 0 ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to parse level", c->argv[0] );
						Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
							c->log, c->cr_msg, c->argv[i]);
						return( 1 );
					}
				} else {
					if ( str2loglevel( c->argv[i], &level ) ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown level", c->argv[0] );
						Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
							c->log, c->cr_msg, c->argv[i]);
						return( 1 );
					}
				}
				/* Explicitly setting a zero clears all the levels */
				if ( level )
					config_syslog |= level;
				else
					config_syslog = 0;
			}

reset:
			slap_debug = slap_debug_orig;
			active_syslog = config_syslog;
			if ( slapMode & SLAP_SERVER_MODE ) {
				if ( logfile_only ) {
					slap_debug |= config_syslog;
					ldap_syslog = 0;
				} else {
					ldap_syslog = config_syslog;
				}
			}
			rc = 0;
			break;

		case CFG_LOGFILE:
			rc = logfile_open( c->value_string );
			if ( rc ) {
				char ebuf[128];
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to open logfile, err=%d \"%s\"",
					c->argv[0], rc, AC_STRERROR_R( rc, ebuf, sizeof(ebuf) ) );
				Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[1]);
				return( 1 );
			}
			ch_free( c->value_string );
			break;

		case CFG_LOGFILE_FORMAT: {
			int len;
			i = verb_to_mask( c->argv[1], logformat_key );

			if ( BER_BVISNULL( &logformat_key[ i ].word ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unknown format", c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
					c->log, c->cr_msg, c->argv[1]);
				return( 1 );
			}
			if ( syslog_prefix )
				ch_free( syslog_prefix );
			logfile_format = logformat_key[i].mask;
			len = strlen( global_host ) + 1 + strlen( serverName ) + 1 + sizeof(("[123456789]:")) +
				(( logfile_format & LFMT_RFC3339) ? sizeof( RFC3339_STAMP ) : sizeof( SYSLOG_STAMP ));
			syslog_prefix = ch_malloc( len );
			splen = sprintf( syslog_prefix, "%s %s %s[%d]: ", ( logfile_format & LFMT_RFC3339 ) ?
				RFC3339_STAMP : SYSLOG_STAMP, global_host, serverName, getpid() );
			}
			break;

		case CFG_LOGFILE_ONLY:
			logfile_only = c->value_int;
			goto reset;

		case CFG_LOGFILE_ROTATE: {
			unsigned lf_max, lf_mbyte, lf_hour;
			if ( lutil_atoux( &lf_max, c->argv[1], 0 ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> "
					"invalid max value \"%s\"", c->argv[0], c->argv[1] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg );
				return 1;
			}
			if ( !lf_max || lf_max > 99 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> "
					"invalid max value \"%s\" must be 1-99", c->argv[0], c->argv[1] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg );
				return 1;
			}
			if ( lutil_atoux( &lf_mbyte, c->argv[2], 0 ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> "
					"invalid Mbyte value \"%s\"", c->argv[0], c->argv[2] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg );
				return 1;
			}
			if ( lutil_atoux( &lf_hour, c->argv[3], 0 ) != 0 ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> "
					"invalid hours value \"%s\"", c->argv[0], c->argv[3] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg );
				return 1;
			}
			if ( !lf_mbyte && !lf_hour ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> "
					"Mbyte and hours cannot both be zero", c->argv[0] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg );
				return 1;
			}
			logfile_max = lf_max;
			logfile_fslimit = lf_mbyte * 1048576;	/* Megabytes to bytes */
			logfile_age = lf_hour * 3600;			/* hours to seconds */
			}
			break;
		default:
			rc = 1;
	}
	return rc;
}
