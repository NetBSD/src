/* $OpenLDAP: pkg/ldap/libraries/libldap/cyrus.c,v 1.133.2.8 2008/02/11 23:26:41 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/errno.h>
#include <ac/ctype.h>
#include <ac/unistd.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "ldap-int.h"

#ifdef HAVE_CYRUS_SASL

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef INT_MAX
#define	INT_MAX	2147483647	/* 32 bit signed max */
#endif

#ifdef LDAP_R_COMPILE
ldap_pvt_thread_mutex_t ldap_int_sasl_mutex;
#endif

#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#else
#include <sasl.h>
#endif

#if SASL_VERSION_MAJOR >= 2
#define SASL_CONST const
#else
#define SASL_CONST
#endif

/*
* Various Cyrus SASL related stuff.
*/

static const sasl_callback_t client_callbacks[] = {
#ifdef SASL_CB_GETREALM
	{ SASL_CB_GETREALM, NULL, NULL },
#endif
	{ SASL_CB_USER, NULL, NULL },
	{ SASL_CB_AUTHNAME, NULL, NULL },
	{ SASL_CB_PASS, NULL, NULL },
	{ SASL_CB_ECHOPROMPT, NULL, NULL },
	{ SASL_CB_NOECHOPROMPT, NULL, NULL },
	{ SASL_CB_LIST_END, NULL, NULL }
};

int ldap_int_sasl_init( void )
{
	/* XXX not threadsafe */
	static int sasl_initialized = 0;

#ifdef HAVE_SASL_VERSION
	/* stringify the version number, sasl.h doesn't do it for us */
#define VSTR0(maj, min, pat)	#maj "." #min "." #pat
#define VSTR(maj, min, pat)	VSTR0(maj, min, pat)
#define SASL_VERSION_STRING	VSTR(SASL_VERSION_MAJOR, SASL_VERSION_MINOR, \
				SASL_VERSION_STEP)
	{ int rc;
	sasl_version( NULL, &rc );
	if ( ((rc >> 16) != ((SASL_VERSION_MAJOR << 8)|SASL_VERSION_MINOR)) ||
		(rc & 0xffff) < SASL_VERSION_STEP) {
		char version[sizeof("xxx.xxx.xxxxx")];
		sprintf( version, "%u.%d.%d", (unsigned)rc >> 24, (rc >> 16) & 0xff,
			rc & 0xffff );

		Debug( LDAP_DEBUG_ANY,
		"ldap_int_sasl_init: SASL library version mismatch:"
		" expected " SASL_VERSION_STRING ","
		" got %s\n", version, 0, 0 );
		return -1;
	}
	}
#endif
	if ( sasl_initialized ) {
		return 0;
	}

/* SASL 2 takes care of its own memory completely internally */
#if SASL_VERSION_MAJOR < 2 && !defined(CSRIMALLOC)
	sasl_set_alloc(
		ber_memalloc,
		ber_memcalloc,
		ber_memrealloc,
		ber_memfree );
#endif /* CSRIMALLOC */

#ifdef LDAP_R_COMPILE
	sasl_set_mutex(
		ldap_pvt_sasl_mutex_new,
		ldap_pvt_sasl_mutex_lock,
		ldap_pvt_sasl_mutex_unlock,    
		ldap_pvt_sasl_mutex_dispose );    
#endif

	if ( sasl_client_init( NULL ) == SASL_OK ) {
		sasl_initialized = 1;
		return 0;
	}

#if SASL_VERSION_MAJOR < 2
	/* A no-op to make sure we link with Cyrus 1.5 */
	sasl_client_auth( NULL, NULL, NULL, 0, NULL, NULL );
#endif
	return -1;
}

/*
 * SASL encryption support for LBER Sockbufs
 */

struct sb_sasl_data {
	sasl_conn_t		*sasl_context;
	unsigned		*sasl_maxbuf;
	Sockbuf_Buf		sec_buf_in;
	Sockbuf_Buf		buf_in;
	Sockbuf_Buf		buf_out;
};

static int
sb_sasl_setup( Sockbuf_IO_Desc *sbiod, void *arg )
{
	struct sb_sasl_data	*p;

	assert( sbiod != NULL );

	p = LBER_MALLOC( sizeof( *p ) );
	if ( p == NULL )
		return -1;
	p->sasl_context = (sasl_conn_t *)arg;
	ber_pvt_sb_buf_init( &p->sec_buf_in );
	ber_pvt_sb_buf_init( &p->buf_in );
	ber_pvt_sb_buf_init( &p->buf_out );
	if ( ber_pvt_sb_grow_buffer( &p->sec_buf_in, SASL_MIN_BUFF_SIZE ) < 0 ) {
		LBER_FREE( p );
		sock_errset(ENOMEM);
		return -1;
	}
	sasl_getprop( p->sasl_context, SASL_MAXOUTBUF,
		(SASL_CONST void **)(char *) &p->sasl_maxbuf );
	    
	sbiod->sbiod_pvt = p;

	return 0;
}

static int
sb_sasl_remove( Sockbuf_IO_Desc *sbiod )
{
	struct sb_sasl_data	*p;

	assert( sbiod != NULL );
	
	p = (struct sb_sasl_data *)sbiod->sbiod_pvt;
#if SASL_VERSION_MAJOR >= 2
	/*
	 * SASLv2 encode/decode buffers are managed by
	 * libsasl2. Ensure they are not freed by liblber.
	 */
	p->buf_in.buf_base = NULL;
	p->buf_out.buf_base = NULL;
#endif
	ber_pvt_sb_buf_destroy( &p->sec_buf_in );
	ber_pvt_sb_buf_destroy( &p->buf_in );
	ber_pvt_sb_buf_destroy( &p->buf_out );
	LBER_FREE( p );
	sbiod->sbiod_pvt = NULL;
	return 0;
}

static ber_len_t
sb_sasl_pkt_length( const unsigned char *buf, int debuglevel )
{
	ber_len_t		size;

	assert( buf != NULL );

	size = buf[0] << 24
		| buf[1] << 16
		| buf[2] << 8
		| buf[3];

	if ( size > SASL_MAX_BUFF_SIZE ) {
		/* somebody is trying to mess me up. */
		ber_log_printf( LDAP_DEBUG_ANY, debuglevel,
			"sb_sasl_pkt_length: received illegal packet length "
			"of %lu bytes\n", (unsigned long)size );      
		size = 16; /* this should lead to an error. */
	}

	return size + 4; /* include the size !!! */
}

/* Drop a processed packet from the input buffer */
static void
sb_sasl_drop_packet ( Sockbuf_Buf *sec_buf_in, int debuglevel )
{
	ber_slen_t			len;

	len = sec_buf_in->buf_ptr - sec_buf_in->buf_end;
	if ( len > 0 )
		AC_MEMCPY( sec_buf_in->buf_base, sec_buf_in->buf_base +
			sec_buf_in->buf_end, len );
   
	if ( len >= 4 ) {
		sec_buf_in->buf_end = sb_sasl_pkt_length(
			(unsigned char *) sec_buf_in->buf_base, debuglevel);
	}
	else {
		sec_buf_in->buf_end = 0;
	}
	sec_buf_in->buf_ptr = len;
}

static ber_slen_t
sb_sasl_read( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct sb_sasl_data	*p;
	ber_slen_t		ret, bufptr;
   
	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct sb_sasl_data *)sbiod->sbiod_pvt;

	/* Are there anything left in the buffer? */
	ret = ber_pvt_sb_copy_out( &p->buf_in, buf, len );
	bufptr = ret;
	len -= ret;

	if ( len == 0 )
		return bufptr;

#if SASL_VERSION_MAJOR >= 2
	ber_pvt_sb_buf_init( &p->buf_in );
#else
	ber_pvt_sb_buf_destroy( &p->buf_in );
#endif

	/* Read the length of the packet */
	while ( p->sec_buf_in.buf_ptr < 4 ) {
		ret = LBER_SBIOD_READ_NEXT( sbiod, p->sec_buf_in.buf_base +
			p->sec_buf_in.buf_ptr,
			4 - p->sec_buf_in.buf_ptr );
#ifdef EINTR
		if ( ( ret < 0 ) && ( errno == EINTR ) )
			continue;
#endif
		if ( ret <= 0 )
			return bufptr ? bufptr : ret;

		p->sec_buf_in.buf_ptr += ret;
	}

	/* The new packet always starts at p->sec_buf_in.buf_base */
	ret = sb_sasl_pkt_length( (unsigned char *) p->sec_buf_in.buf_base,
		sbiod->sbiod_sb->sb_debug );

	/* Grow the packet buffer if neccessary */
	if ( ( p->sec_buf_in.buf_size < (ber_len_t) ret ) && 
		ber_pvt_sb_grow_buffer( &p->sec_buf_in, ret ) < 0 )
	{
		sock_errset(ENOMEM);
		return -1;
	}
	p->sec_buf_in.buf_end = ret;

	/* Did we read the whole encrypted packet? */
	while ( p->sec_buf_in.buf_ptr < p->sec_buf_in.buf_end ) {
		/* No, we have got only a part of it */
		ret = p->sec_buf_in.buf_end - p->sec_buf_in.buf_ptr;

		ret = LBER_SBIOD_READ_NEXT( sbiod, p->sec_buf_in.buf_base +
			p->sec_buf_in.buf_ptr, ret );
#ifdef EINTR
		if ( ( ret < 0 ) && ( errno == EINTR ) )
			continue;
#endif
		if ( ret <= 0 )
			return bufptr ? bufptr : ret;

		p->sec_buf_in.buf_ptr += ret;
   	}

	/* Decode the packet */
	{
		unsigned tmpsize = p->buf_in.buf_end;
		ret = sasl_decode( p->sasl_context, p->sec_buf_in.buf_base,
			p->sec_buf_in.buf_end,
			(SASL_CONST char **)&p->buf_in.buf_base,
			(unsigned *)&tmpsize );
		p->buf_in.buf_end = tmpsize;
	}

	/* Drop the packet from the input buffer */
	sb_sasl_drop_packet( &p->sec_buf_in, sbiod->sbiod_sb->sb_debug );

	if ( ret != SASL_OK ) {
		ber_log_printf( LDAP_DEBUG_ANY, sbiod->sbiod_sb->sb_debug,
			"sb_sasl_read: failed to decode packet: %s\n",
			sasl_errstring( ret, NULL, NULL ) );
		sock_errset(EIO);
		return -1;
	}
	
	p->buf_in.buf_size = p->buf_in.buf_end;

	bufptr += ber_pvt_sb_copy_out( &p->buf_in, (char*) buf + bufptr, len );

	return bufptr;
}

static ber_slen_t
sb_sasl_write( Sockbuf_IO_Desc *sbiod, void *buf, ber_len_t len)
{
	struct sb_sasl_data	*p;
	int			ret;

	assert( sbiod != NULL );
	assert( SOCKBUF_VALID( sbiod->sbiod_sb ) );

	p = (struct sb_sasl_data *)sbiod->sbiod_pvt;

	/* Are there anything left in the buffer? */
	if ( p->buf_out.buf_ptr != p->buf_out.buf_end ) {
		ret = ber_pvt_sb_do_write( sbiod, &p->buf_out );
		if ( ret < 0 ) return ret;

		/* Still have something left?? */
		if ( p->buf_out.buf_ptr != p->buf_out.buf_end ) {
			sock_errset(EAGAIN);
			return -1;
		}
	}

	/* now encode the next packet. */
#if SASL_VERSION_MAJOR >= 2
	ber_pvt_sb_buf_init( &p->buf_out );
#else
	ber_pvt_sb_buf_destroy( &p->buf_out );
#endif
	if ( len > *p->sasl_maxbuf - 100 ) {
		len = *p->sasl_maxbuf - 100;	/* For safety margin */
	}

	{
		unsigned tmpsize = p->buf_out.buf_size;
		ret = sasl_encode( p->sasl_context, buf, len,
			(SASL_CONST char **)&p->buf_out.buf_base,
			&tmpsize );
		p->buf_out.buf_size = tmpsize;
	}

	if ( ret != SASL_OK ) {
		ber_log_printf( LDAP_DEBUG_ANY, sbiod->sbiod_sb->sb_debug,
			"sb_sasl_write: failed to encode packet: %s\n",
			sasl_errstring( ret, NULL, NULL ) );
		sock_errset(EIO);
		return -1;
	}
	p->buf_out.buf_end = p->buf_out.buf_size;

	ret = ber_pvt_sb_do_write( sbiod, &p->buf_out );

	/* return number of bytes encoded, not written, to ensure
	 * no byte is encoded twice (even if only sent once).
	 */
	return len;
}

static int
sb_sasl_ctrl( Sockbuf_IO_Desc *sbiod, int opt, void *arg )
{
	struct sb_sasl_data	*p;

	p = (struct sb_sasl_data *)sbiod->sbiod_pvt;

	if ( opt == LBER_SB_OPT_DATA_READY ) {
		if ( p->buf_in.buf_ptr != p->buf_in.buf_end ) return 1;
	}
	
	return LBER_SBIOD_CTRL_NEXT( sbiod, opt, arg );
}

Sockbuf_IO ldap_pvt_sockbuf_io_sasl = {
	sb_sasl_setup,		/* sbi_setup */
	sb_sasl_remove,		/* sbi_remove */
	sb_sasl_ctrl,		/* sbi_ctrl */
	sb_sasl_read,		/* sbi_read */
	sb_sasl_write,		/* sbi_write */
	NULL			/* sbi_close */
};

int ldap_pvt_sasl_install( Sockbuf *sb, void *ctx_arg )
{
	Debug( LDAP_DEBUG_TRACE, "ldap_pvt_sasl_install\n",
		0, 0, 0 );

	/* don't install the stuff unless security has been negotiated */

	if ( !ber_sockbuf_ctrl( sb, LBER_SB_OPT_HAS_IO,
			&ldap_pvt_sockbuf_io_sasl ) )
	{
#ifdef LDAP_DEBUG
		ber_sockbuf_add_io( sb, &ber_sockbuf_io_debug,
			LBER_SBIOD_LEVEL_APPLICATION, (void *)"sasl_" );
#endif
		ber_sockbuf_add_io( sb, &ldap_pvt_sockbuf_io_sasl,
			LBER_SBIOD_LEVEL_APPLICATION, ctx_arg );
	}

	return LDAP_SUCCESS;
}

void ldap_pvt_sasl_remove( Sockbuf *sb )
{
	ber_sockbuf_remove_io( sb, &ldap_pvt_sockbuf_io_sasl,
		LBER_SBIOD_LEVEL_APPLICATION );
#ifdef LDAP_DEBUG
	ber_sockbuf_remove_io( sb, &ber_sockbuf_io_debug,
		LBER_SBIOD_LEVEL_APPLICATION );
#endif
}

static int
sasl_err2ldap( int saslerr )
{
	int rc;

	/* map SASL errors to LDAP API errors returned by:
	 *	sasl_client_new()
	 *		SASL_OK, SASL_NOMECH, SASL_NOMEM
	 *	sasl_client_start()
	 *		SASL_OK, SASL_NOMECH, SASL_NOMEM, SASL_INTERACT
	 *	sasl_client_step()
	 *		SASL_OK, SASL_INTERACT, SASL_BADPROT, SASL_BADSERV
	 */

	switch (saslerr) {
		case SASL_CONTINUE:
			rc = LDAP_MORE_RESULTS_TO_RETURN;
			break;
		case SASL_INTERACT:
			rc = LDAP_LOCAL_ERROR;
			break;
		case SASL_OK:
			rc = LDAP_SUCCESS;
			break;
		case SASL_NOMEM:
			rc = LDAP_NO_MEMORY;
			break;
		case SASL_NOMECH:
			rc = LDAP_AUTH_UNKNOWN;
			break;
		case SASL_BADPROT:
			rc = LDAP_DECODING_ERROR;
			break;
		case SASL_BADSERV:
			rc = LDAP_AUTH_UNKNOWN;
			break;

		/* other codes */
		case SASL_BADAUTH:
			rc = LDAP_AUTH_UNKNOWN;
			break;
		case SASL_NOAUTHZ:
			rc = LDAP_PARAM_ERROR;
			break;
		case SASL_FAIL:
			rc = LDAP_LOCAL_ERROR;
			break;
		case SASL_TOOWEAK:
		case SASL_ENCRYPT:
			rc = LDAP_AUTH_UNKNOWN;
			break;
		default:
			rc = LDAP_LOCAL_ERROR;
			break;
	}

	assert( rc == LDAP_SUCCESS || LDAP_API_ERROR( rc ) );
	return rc;
}

int
ldap_int_sasl_open(
	LDAP *ld, 
	LDAPConn *lc,
	const char * host )
{
	int rc;
	sasl_conn_t *ctx;

	assert( lc->lconn_sasl_authctx == NULL );

	if ( host == NULL ) {
		ld->ld_errno = LDAP_LOCAL_ERROR;
		return ld->ld_errno;
	}

	if ( ldap_int_sasl_init() ) {
		ld->ld_errno = LDAP_LOCAL_ERROR;
		return ld->ld_errno;
	}

#if SASL_VERSION_MAJOR >= 2
	rc = sasl_client_new( "ldap", host, NULL, NULL,
		client_callbacks, 0, &ctx );
#else
	rc = sasl_client_new( "ldap", host, client_callbacks,
		SASL_SECURITY_LAYER, &ctx );
#endif

	if ( rc != SASL_OK ) {
		ld->ld_errno = sasl_err2ldap( rc );
		return ld->ld_errno;
	}

	Debug( LDAP_DEBUG_TRACE, "ldap_int_sasl_open: host=%s\n",
		host, 0, 0 );

	lc->lconn_sasl_authctx = ctx;

	return LDAP_SUCCESS;
}

int ldap_int_sasl_close( LDAP *ld, LDAPConn *lc )
{
	sasl_conn_t *ctx = lc->lconn_sasl_authctx;

	if( ctx != NULL ) {
		sasl_dispose( &ctx );
		if ( lc->lconn_sasl_sockctx &&
			lc->lconn_sasl_authctx != lc->lconn_sasl_sockctx ) {
			ctx = lc->lconn_sasl_sockctx;
			sasl_dispose( &ctx );
		}
		lc->lconn_sasl_sockctx = NULL;
		lc->lconn_sasl_authctx = NULL;
	}

	return LDAP_SUCCESS;
}

int
ldap_int_sasl_bind(
	LDAP			*ld,
	const char		*dn,
	const char		*mechs,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	unsigned		flags,
	LDAP_SASL_INTERACT_PROC *interact,
	void * defaults )
{
	char *data;
	const char *mech = NULL;
	const char *pmech = NULL;
	int			saslrc, rc;
	sasl_ssf_t		*ssf = NULL;
	sasl_conn_t	*ctx, *oldctx = NULL;
	sasl_interact_t *prompts = NULL;
	unsigned credlen;
	struct berval ccred;
	ber_socket_t		sd;
	void	*ssl;

	Debug( LDAP_DEBUG_TRACE, "ldap_int_sasl_bind: %s\n",
		mechs ? mechs : "<null>", 0, 0 );

	/* do a quick !LDAPv3 check... ldap_sasl_bind will do the rest. */
	if (ld->ld_version < LDAP_VERSION3) {
		ld->ld_errno = LDAP_NOT_SUPPORTED;
		return ld->ld_errno;
	}

	rc = 0;
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &ld->ld_req_mutex );
#endif
	ber_sockbuf_ctrl( ld->ld_sb, LBER_SB_OPT_GET_FD, &sd );

	if ( sd == AC_SOCKET_INVALID ) {
 		/* not connected yet */

		rc = ldap_open_defconn( ld );

		if ( rc == 0 ) {
			ber_sockbuf_ctrl( ld->ld_defconn->lconn_sb,
				LBER_SB_OPT_GET_FD, &sd );

			if( sd == AC_SOCKET_INVALID ) {
				ld->ld_errno = LDAP_LOCAL_ERROR;
				rc = ld->ld_errno;
			}
		}
	}   
#ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &ld->ld_req_mutex );
#endif
	if( rc != 0 ) return ld->ld_errno;

	oldctx = ld->ld_defconn->lconn_sasl_authctx;

	/* If we already have an authentication context, clear it out */
	if( oldctx ) {
		if ( oldctx != ld->ld_defconn->lconn_sasl_sockctx ) {
			sasl_dispose( &oldctx );
		}
		ld->ld_defconn->lconn_sasl_authctx = NULL;
	}

	{
		char *saslhost = ldap_host_connected_to( ld->ld_defconn->lconn_sb,
			"localhost" );
		rc = ldap_int_sasl_open( ld, ld->ld_defconn, saslhost );
		LDAP_FREE( saslhost );
	}

	if ( rc != LDAP_SUCCESS ) return rc;

	ctx = ld->ld_defconn->lconn_sasl_authctx;

	/* Check for TLS */
	ssl = ldap_pvt_tls_sb_ctx( ld->ld_defconn->lconn_sb );
	if ( ssl ) {
		struct berval authid = BER_BVNULL;
		ber_len_t fac;

		fac = ldap_pvt_tls_get_strength( ssl );
		/* failure is OK, we just can't use SASL EXTERNAL */
		(void) ldap_pvt_tls_get_my_dn( ssl, &authid, NULL, 0 );

		(void) ldap_int_sasl_external( ld, ld->ld_defconn, authid.bv_val, fac );
		LDAP_FREE( authid.bv_val );
	}

#if !defined(_WIN32)
	/* Check for local */
	if ( ldap_pvt_url_scheme2proto(
		ld->ld_defconn->lconn_server->lud_scheme ) == LDAP_PROTO_IPC )
	{
		char authid[sizeof("gidNumber=4294967295+uidNumber=4294967295,"
			"cn=peercred,cn=external,cn=auth")];
		sprintf( authid, "gidNumber=%u+uidNumber=%u,"
			"cn=peercred,cn=external,cn=auth",
			getegid(), geteuid() );
		(void) ldap_int_sasl_external( ld, ld->ld_defconn, authid,
			LDAP_PVT_SASL_LOCAL_SSF );
	}
#endif

	/* (re)set security properties */
	sasl_setprop( ctx, SASL_SEC_PROPS,
		&ld->ld_options.ldo_sasl_secprops );

	ccred.bv_val = NULL;
	ccred.bv_len = 0;

	do {
		saslrc = sasl_client_start( ctx,
			mechs,
#if SASL_VERSION_MAJOR < 2
			NULL,
#endif
			&prompts,
			(SASL_CONST char **)&ccred.bv_val,
			&credlen,
			&mech );

		if( pmech == NULL && mech != NULL ) {
			pmech = mech;

			if( flags != LDAP_SASL_QUIET ) {
				fprintf(stderr,
					"SASL/%s authentication started\n",
					pmech );
			}
		}

		if( saslrc == SASL_INTERACT ) {
			int res;
			if( !interact ) break;
			res = (interact)( ld, flags, defaults, prompts );

			if( res != LDAP_SUCCESS ) break;
		}
	} while ( saslrc == SASL_INTERACT );

	ccred.bv_len = credlen;

	if ( (saslrc != SASL_OK) && (saslrc != SASL_CONTINUE) ) {
		rc = ld->ld_errno = sasl_err2ldap( saslrc );
#if SASL_VERSION_MAJOR >= 2
		if ( ld->ld_error ) {
			LDAP_FREE( ld->ld_error );
		}
		ld->ld_error = LDAP_STRDUP( sasl_errdetail( ctx ) );
#endif
		goto done;
	}

	do {
		struct berval *scred;
		unsigned credlen;

		scred = NULL;

		rc = ldap_sasl_bind_s( ld, dn, mech, &ccred, sctrls, cctrls,
			&scred );

		if ( ccred.bv_val != NULL ) {
#if SASL_VERSION_MAJOR < 2
			LDAP_FREE( ccred.bv_val );
#endif
			ccred.bv_val = NULL;
		}

		if ( rc != LDAP_SUCCESS && rc != LDAP_SASL_BIND_IN_PROGRESS ) {
			if( scred ) {
				/* and server provided us with data? */
				Debug( LDAP_DEBUG_TRACE,
					"ldap_int_sasl_bind: rc=%d sasl=%d len=%ld\n",
					rc, saslrc, scred ? scred->bv_len : -1 );
				ber_bvfree( scred );
				scred = NULL;
			}
			rc = ld->ld_errno;
			goto done;
		}

		if( rc == LDAP_SUCCESS && saslrc == SASL_OK ) {
			/* we're done, no need to step */
			if( scred ) {
				/* but we got additional data? */
#define KLUDGE_FOR_MSAD
#ifdef 	KLUDGE_FOR_MSAD
				/*
				 * MSAD provides empty additional data in violation of LDAP
				 * technical specifications.  As no existing SASL mechanism
				 * allows empty data with an outcome message, just ignore it
				 * for now.  Hopefully MS will fix their bug before someone
				 * defines a mechanism with possibly empty additional data.
				 */
				if( scred->bv_len == 0 ) {
					Debug( LDAP_DEBUG_ANY,
						"ldap_int_sasl_bind: ignoring "
							" bogus empty data provided with SASL outcome message.\n",
						rc, saslrc, scred->bv_len );
					ber_bvfree( scred );
				} else
#endif
				{
					Debug( LDAP_DEBUG_TRACE,
						"ldap_int_sasl_bind: rc=%d sasl=%d len=%ld\n",
						rc, saslrc, scred->bv_len );
					rc = ld->ld_errno = LDAP_LOCAL_ERROR;
					ber_bvfree( scred );
					goto done;
				}
			}
			break;
		}

		do {
			if( ! scred ) {
				/* no data! */
				Debug( LDAP_DEBUG_TRACE,
					"ldap_int_sasl_bind: no data in step!\n",
					0, 0, 0 );
			}

			saslrc = sasl_client_step( ctx,
				(scred == NULL) ? NULL : scred->bv_val,
				(scred == NULL) ? 0 : scred->bv_len,
				&prompts,
				(SASL_CONST char **)&ccred.bv_val,
				&credlen );

			Debug( LDAP_DEBUG_TRACE, "sasl_client_step: %d\n",
				saslrc, 0, 0 );

			if( saslrc == SASL_INTERACT ) {
				int res;
				if( !interact ) break;
				res = (interact)( ld, flags, defaults, prompts );
				if( res != LDAP_SUCCESS ) break;
			}
		} while ( saslrc == SASL_INTERACT );

		ccred.bv_len = credlen;
		ber_bvfree( scred );

		if ( (saslrc != SASL_OK) && (saslrc != SASL_CONTINUE) ) {
			ld->ld_errno = sasl_err2ldap( saslrc );
#if SASL_VERSION_MAJOR >= 2
			if ( ld->ld_error ) {
				LDAP_FREE( ld->ld_error );
			}
			ld->ld_error = LDAP_STRDUP( sasl_errdetail( ctx ) );
#endif
			rc = ld->ld_errno;
			goto done;
		}
	} while ( rc == LDAP_SASL_BIND_IN_PROGRESS );

	if ( rc != LDAP_SUCCESS ) goto done;

	if ( saslrc != SASL_OK ) {
#if SASL_VERSION_MAJOR >= 2
		if ( ld->ld_error ) {
			LDAP_FREE( ld->ld_error );
		}
		ld->ld_error = LDAP_STRDUP( sasl_errdetail( ctx ) );
#endif
		rc = ld->ld_errno = sasl_err2ldap( saslrc );
		goto done;
	}

	if( flags != LDAP_SASL_QUIET ) {
		saslrc = sasl_getprop( ctx, SASL_USERNAME,
			(SASL_CONST void **)(char *) &data );
		if( saslrc == SASL_OK && data && *data ) {
			fprintf( stderr, "SASL username: %s\n", data );
		}

#if SASL_VERSION_MAJOR < 2
		saslrc = sasl_getprop( ctx, SASL_REALM,
			(SASL_CONST void **) &data );
		if( saslrc == SASL_OK && data && *data ) {
			fprintf( stderr, "SASL realm: %s\n", data );
		}
#endif
	}

	saslrc = sasl_getprop( ctx, SASL_SSF, (SASL_CONST void **)(char *) &ssf );
	if( saslrc == SASL_OK ) {
		if( flags != LDAP_SASL_QUIET ) {
			fprintf( stderr, "SASL SSF: %lu\n",
				(unsigned long) *ssf );
		}

		if( ssf && *ssf ) {
			if ( ld->ld_defconn->lconn_sasl_sockctx ) {
				oldctx = ld->ld_defconn->lconn_sasl_sockctx;
				sasl_dispose( &oldctx );
				ldap_pvt_sasl_remove( ld->ld_defconn->lconn_sb );
			}
			ldap_pvt_sasl_install( ld->ld_defconn->lconn_sb, ctx );
			ld->ld_defconn->lconn_sasl_sockctx = ctx;

			if( flags != LDAP_SASL_QUIET ) {
				fprintf( stderr, "SASL data security layer installed.\n" );
			}
		}
	}
	ld->ld_defconn->lconn_sasl_authctx = ctx;

done:
	return rc;
}

int
ldap_int_sasl_external(
	LDAP *ld,
	LDAPConn *conn,
	const char * authid,
	ber_len_t ssf )
{
	int sc;
	sasl_conn_t *ctx;
#if SASL_VERSION_MAJOR < 2
	sasl_external_properties_t extprops;
#else
	sasl_ssf_t sasl_ssf = ssf;
#endif

	ctx = conn->lconn_sasl_authctx;

	if ( ctx == NULL ) {
		return LDAP_LOCAL_ERROR;
	}
   
#if SASL_VERSION_MAJOR >= 2
	sc = sasl_setprop( ctx, SASL_SSF_EXTERNAL, &sasl_ssf );
	if ( sc == SASL_OK )
		sc = sasl_setprop( ctx, SASL_AUTH_EXTERNAL, authid );
#else
	memset( &extprops, '\0', sizeof(extprops) );
	extprops.ssf = ssf;
	extprops.auth_id = (char *) authid;

	sc = sasl_setprop( ctx, SASL_SSF_EXTERNAL,
		(void *) &extprops );
#endif

	if ( sc != SASL_OK ) {
		return LDAP_LOCAL_ERROR;
	}

	return LDAP_SUCCESS;
}


#define GOT_MINSSF	1
#define	GOT_MAXSSF	2
#define	GOT_MAXBUF	4

static struct {
	struct berval key;
	int sflag;
	int ival;
	int idef;
} sprops[] = {
	{ BER_BVC("none"), 0, 0, 0 },
	{ BER_BVC("nodict"), SASL_SEC_NODICTIONARY, 0, 0 },
	{ BER_BVC("noplain"), SASL_SEC_NOPLAINTEXT, 0, 0 },
	{ BER_BVC("noactive"), SASL_SEC_NOACTIVE, 0, 0 },
	{ BER_BVC("passcred"), SASL_SEC_PASS_CREDENTIALS, 0, 0 },
	{ BER_BVC("forwardsec"), SASL_SEC_FORWARD_SECRECY, 0, 0 },
	{ BER_BVC("noanonymous"), SASL_SEC_NOANONYMOUS, 0, 0 },
	{ BER_BVC("minssf="), 0, GOT_MINSSF, 0 },
	{ BER_BVC("maxssf="), 0, GOT_MAXSSF, INT_MAX },
	{ BER_BVC("maxbufsize="), 0, GOT_MAXBUF, 65536 },
	{ BER_BVNULL, 0, 0, 0 }
};

void ldap_pvt_sasl_secprops_unparse(
	sasl_security_properties_t *secprops,
	struct berval *out )
{
	int i, l = 0;
	int comma;
	char *ptr;

	if ( secprops == NULL || out == NULL ) {
		return;
	}

	comma = 0;
	for ( i=0; !BER_BVISNULL( &sprops[i].key ); i++ ) {
		if ( sprops[i].ival ) {
			int v = 0;

			switch( sprops[i].ival ) {
			case GOT_MINSSF: v = secprops->min_ssf; break;
			case GOT_MAXSSF: v = secprops->max_ssf; break;
			case GOT_MAXBUF: v = secprops->maxbufsize; break;
			}
			/* It is the default, ignore it */
			if ( v == sprops[i].idef ) continue;

			l += sprops[i].key.bv_len + 24;
		} else if ( sprops[i].sflag ) {
			if ( sprops[i].sflag & secprops->security_flags ) {
				l += sprops[i].key.bv_len;
			}
		} else if ( secprops->security_flags == 0 ) {
			l += sprops[i].key.bv_len;
		}
		if ( comma ) l++;
		comma = 1;
	}
	l++;

	out->bv_val = LDAP_MALLOC( l );
	if ( out->bv_val == NULL ) {
		out->bv_len = 0;
		return;
	}

	ptr = out->bv_val;
	comma = 0;
	for ( i=0; !BER_BVISNULL( &sprops[i].key ); i++ ) {
		if ( sprops[i].ival ) {
			int v = 0;

			switch( sprops[i].ival ) {
			case GOT_MINSSF: v = secprops->min_ssf; break;
			case GOT_MAXSSF: v = secprops->max_ssf; break;
			case GOT_MAXBUF: v = secprops->maxbufsize; break;
			}
			/* It is the default, ignore it */
			if ( v == sprops[i].idef ) continue;

			if ( comma ) *ptr++ = ',';
			ptr += sprintf(ptr, "%s%d", sprops[i].key.bv_val, v );
			comma = 1;
		} else if ( sprops[i].sflag ) {
			if ( sprops[i].sflag & secprops->security_flags ) {
				if ( comma ) *ptr++ = ',';
				ptr += sprintf(ptr, "%s", sprops[i].key.bv_val );
				comma = 1;
			}
		} else if ( secprops->security_flags == 0 ) {
			if ( comma ) *ptr++ = ',';
			ptr += sprintf(ptr, "%s", sprops[i].key.bv_val );
			comma = 1;
		}
	}
	out->bv_len = ptr - out->bv_val;
}

int ldap_pvt_sasl_secprops(
	const char *in,
	sasl_security_properties_t *secprops )
{
	int i, j, l;
	char **props;
	unsigned sflags = 0;
	int got_sflags = 0;
	sasl_ssf_t max_ssf = 0;
	int got_max_ssf = 0;
	sasl_ssf_t min_ssf = 0;
	int got_min_ssf = 0;
	unsigned maxbufsize = 0;
	int got_maxbufsize = 0;

	if( secprops == NULL ) {
		return LDAP_PARAM_ERROR;
	}
	props = ldap_str2charray( in, "," );
	if( props == NULL ) {
		return LDAP_PARAM_ERROR;
	}

	for( i=0; props[i]; i++ ) {
		l = strlen( props[i] );
		for ( j=0; !BER_BVISNULL( &sprops[j].key ); j++ ) {
			if ( l < sprops[j].key.bv_len ) continue;
			if ( strncasecmp( props[i], sprops[j].key.bv_val,
				sprops[j].key.bv_len )) continue;
			if ( sprops[j].ival ) {
				unsigned v;
				char *next = NULL;
				if ( !isdigit( (unsigned char)props[i][sprops[j].key.bv_len] ))
					continue;
				v = strtoul( &props[i][sprops[j].key.bv_len], &next, 10 );
				if ( next == &props[i][sprops[j].key.bv_len] || next[0] != '\0' ) continue;
				switch( sprops[j].ival ) {
				case GOT_MINSSF:
					min_ssf = v; got_min_ssf++; break;
				case GOT_MAXSSF:
					max_ssf = v; got_max_ssf++; break;
				case GOT_MAXBUF:
					maxbufsize = v; got_maxbufsize++; break;
				}
			} else {
				if ( props[i][sprops[j].key.bv_len] ) continue;
				if ( sprops[j].sflag )
					sflags |= sprops[j].sflag;
				else
					sflags = 0;
				got_sflags++;
			}
			break;
		}
		if ( BER_BVISNULL( &sprops[j].key )) {
			ldap_charray_free( props );
			return LDAP_NOT_SUPPORTED;
		}
	}

	if(got_sflags) {
		secprops->security_flags = sflags;
	}
	if(got_min_ssf) {
		secprops->min_ssf = min_ssf;
	}
	if(got_max_ssf) {
		secprops->max_ssf = max_ssf;
	}
	if(got_maxbufsize) {
		secprops->maxbufsize = maxbufsize;
	}

	ldap_charray_free( props );
	return LDAP_SUCCESS;
}

int
ldap_int_sasl_config( struct ldapoptions *lo, int option, const char *arg )
{
	int rc;

	switch( option ) {
	case LDAP_OPT_X_SASL_SECPROPS:
		rc = ldap_pvt_sasl_secprops( arg, &lo->ldo_sasl_secprops );
		if( rc == LDAP_SUCCESS ) return 0;
	}

	return -1;
}

int
ldap_int_sasl_get_option( LDAP *ld, int option, void *arg )
{
	if ( ld == NULL )
		return -1;

	switch ( option ) {
		case LDAP_OPT_X_SASL_MECH: {
			*(char **)arg = ld->ld_options.ldo_def_sasl_mech
				? LDAP_STRDUP( ld->ld_options.ldo_def_sasl_mech ) : NULL;
		} break;
		case LDAP_OPT_X_SASL_REALM: {
			*(char **)arg = ld->ld_options.ldo_def_sasl_realm
				? LDAP_STRDUP( ld->ld_options.ldo_def_sasl_realm ) : NULL;
		} break;
		case LDAP_OPT_X_SASL_AUTHCID: {
			*(char **)arg = ld->ld_options.ldo_def_sasl_authcid
				? LDAP_STRDUP( ld->ld_options.ldo_def_sasl_authcid ) : NULL;
		} break;
		case LDAP_OPT_X_SASL_AUTHZID: {
			*(char **)arg = ld->ld_options.ldo_def_sasl_authzid
				? LDAP_STRDUP( ld->ld_options.ldo_def_sasl_authzid ) : NULL;
		} break;

		case LDAP_OPT_X_SASL_SSF: {
			int sc;
			sasl_ssf_t	*ssf;
			sasl_conn_t *ctx;

			if( ld->ld_defconn == NULL ) {
				return -1;
			}

			ctx = ld->ld_defconn->lconn_sasl_sockctx;

			if ( ctx == NULL ) {
				return -1;
			}

			sc = sasl_getprop( ctx, SASL_SSF,
				(SASL_CONST void **)(char *) &ssf );

			if ( sc != SASL_OK ) {
				return -1;
			}

			*(ber_len_t *)arg = *ssf;
		} break;

		case LDAP_OPT_X_SASL_SSF_EXTERNAL:
			/* this option is write only */
			return -1;

		case LDAP_OPT_X_SASL_SSF_MIN:
			*(ber_len_t *)arg = ld->ld_options.ldo_sasl_secprops.min_ssf;
			break;
		case LDAP_OPT_X_SASL_SSF_MAX:
			*(ber_len_t *)arg = ld->ld_options.ldo_sasl_secprops.max_ssf;
			break;
		case LDAP_OPT_X_SASL_MAXBUFSIZE:
			*(ber_len_t *)arg = ld->ld_options.ldo_sasl_secprops.maxbufsize;
			break;

		case LDAP_OPT_X_SASL_SECPROPS:
			/* this option is write only */
			return -1;

		default:
			return -1;
	}
	return 0;
}

int
ldap_int_sasl_set_option( LDAP *ld, int option, void *arg )
{
	if ( ld == NULL || arg == NULL )
		return -1;

	switch ( option ) {
	case LDAP_OPT_X_SASL_SSF:
		/* This option is read-only */
		return -1;

	case LDAP_OPT_X_SASL_SSF_EXTERNAL: {
		int sc;
#if SASL_VERSION_MAJOR < 2
		sasl_external_properties_t extprops;
#else
		sasl_ssf_t sasl_ssf;
#endif
		sasl_conn_t *ctx;

		if( ld->ld_defconn == NULL ) {
			return -1;
		}

		ctx = ld->ld_defconn->lconn_sasl_authctx;

		if ( ctx == NULL ) {
			return -1;
		}

#if SASL_VERSION_MAJOR >= 2
		sasl_ssf = * (ber_len_t *)arg;
		sc = sasl_setprop( ctx, SASL_SSF_EXTERNAL, &sasl_ssf);
#else
		memset(&extprops, 0L, sizeof(extprops));

		extprops.ssf = * (ber_len_t *) arg;

		sc = sasl_setprop( ctx, SASL_SSF_EXTERNAL,
			(void *) &extprops );
#endif

		if ( sc != SASL_OK ) {
			return -1;
		}
		} break;

	case LDAP_OPT_X_SASL_SSF_MIN:
		ld->ld_options.ldo_sasl_secprops.min_ssf = *(ber_len_t *)arg;
		break;
	case LDAP_OPT_X_SASL_SSF_MAX:
		ld->ld_options.ldo_sasl_secprops.max_ssf = *(ber_len_t *)arg;
		break;
	case LDAP_OPT_X_SASL_MAXBUFSIZE:
		ld->ld_options.ldo_sasl_secprops.maxbufsize = *(ber_len_t *)arg;
		break;

	case LDAP_OPT_X_SASL_SECPROPS: {
		int sc;
		sc = ldap_pvt_sasl_secprops( (char *) arg,
			&ld->ld_options.ldo_sasl_secprops );

		return sc == LDAP_SUCCESS ? 0 : -1;
		}

	default:
		return -1;
	}
	return 0;
}

#ifdef LDAP_R_COMPILE
#define LDAP_DEBUG_R_SASL
void *ldap_pvt_sasl_mutex_new(void)
{
	ldap_pvt_thread_mutex_t *mutex;

	mutex = (ldap_pvt_thread_mutex_t *) LDAP_CALLOC( 1,
		sizeof(ldap_pvt_thread_mutex_t) );

	if ( ldap_pvt_thread_mutex_init( mutex ) == 0 ) {
		return mutex;
	}
#ifndef LDAP_DEBUG_R_SASL
	assert( 0 );
#endif /* !LDAP_DEBUG_R_SASL */
	return NULL;
}

int ldap_pvt_sasl_mutex_lock(void *mutex)
{
#ifdef LDAP_DEBUG_R_SASL
	if ( mutex == NULL ) {
		return SASL_OK;
	}
#else /* !LDAP_DEBUG_R_SASL */
	assert( mutex != NULL );
#endif /* !LDAP_DEBUG_R_SASL */
	return ldap_pvt_thread_mutex_lock( (ldap_pvt_thread_mutex_t *)mutex )
		? SASL_FAIL : SASL_OK;
}

int ldap_pvt_sasl_mutex_unlock(void *mutex)
{
#ifdef LDAP_DEBUG_R_SASL
	if ( mutex == NULL ) {
		return SASL_OK;
	}
#else /* !LDAP_DEBUG_R_SASL */
	assert( mutex != NULL );
#endif /* !LDAP_DEBUG_R_SASL */
	return ldap_pvt_thread_mutex_unlock( (ldap_pvt_thread_mutex_t *)mutex )
		? SASL_FAIL : SASL_OK;
}

void ldap_pvt_sasl_mutex_dispose(void *mutex)
{
#ifdef LDAP_DEBUG_R_SASL
	if ( mutex == NULL ) {
		return;
	}
#else /* !LDAP_DEBUG_R_SASL */
	assert( mutex != NULL );
#endif /* !LDAP_DEBUG_R_SASL */
	(void) ldap_pvt_thread_mutex_destroy( (ldap_pvt_thread_mutex_t *)mutex );
	LDAP_FREE( mutex );
}
#endif

#else
int ldap_int_sasl_init( void )
{ return LDAP_SUCCESS; }

int ldap_int_sasl_close( LDAP *ld, LDAPConn *lc )
{ return LDAP_SUCCESS; }

int
ldap_int_sasl_bind(
	LDAP			*ld,
	const char		*dn,
	const char		*mechs,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	unsigned		flags,
	LDAP_SASL_INTERACT_PROC *interact,
	void * defaults )
{ return LDAP_NOT_SUPPORTED; }

int
ldap_int_sasl_external(
	LDAP *ld,
	LDAPConn *conn,
	const char * authid,
	ber_len_t ssf )
{ return LDAP_SUCCESS; }

#endif /* HAVE_CYRUS_SASL */
