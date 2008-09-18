/* $OpenLDAP: pkg/ldap/libraries/libldap/util-int.c,v 1.57.2.3 2008/02/11 23:26:41 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * Portions Copyright 1998 A. Hartgers.
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
 * This work was initially developed by Bart Hartgers for inclusion in
 * OpenLDAP Software.
 */

/*
 * util-int.c	Various functions to replace missing threadsafe ones.
 *				Without the real *_r funcs, things will
 *				work, but might not be threadsafe. 
 */

#include "portable.h"

#include <ac/stdlib.h>

#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "ldap-int.h"

#ifndef h_errno
/* newer systems declare this in <netdb.h> for you, older ones don't.
 * harmless to declare it again (unless defined by a macro).
 */
extern int h_errno;
#endif

#ifdef HAVE_HSTRERROR
# define HSTRERROR(e)	hstrerror(e)
#else
# define HSTRERROR(e)	hp_strerror(e)
#endif

#ifndef LDAP_R_COMPILE
# undef HAVE_REENTRANT_FUNCTIONS
# undef HAVE_CTIME_R
# undef HAVE_GETHOSTBYNAME_R
# undef HAVE_GETHOSTBYADDR_R

#else
# include <ldap_pvt_thread.h>
  ldap_pvt_thread_mutex_t ldap_int_resolv_mutex;

# if (defined( HAVE_CTIME_R ) || defined( HAVE_REENTRANT_FUNCTIONS)) \
	 && defined( CTIME_R_NARGS )
#   define USE_CTIME_R
# else
	static ldap_pvt_thread_mutex_t ldap_int_ctime_mutex;
# endif

# if defined(HAVE_GETHOSTBYNAME_R) && \
	(GETHOSTBYNAME_R_NARGS < 5) || (6 < GETHOSTBYNAME_R_NARGS)
	/* Don't know how to handle this version, pretend it's not there */
#	undef HAVE_GETHOSTBYNAME_R
# endif
# if defined(HAVE_GETHOSTBYADDR_R) && \
	(GETHOSTBYADDR_R_NARGS < 7) || (8 < GETHOSTBYADDR_R_NARGS)
	/* Don't know how to handle this version, pretend it's not there */
#	undef HAVE_GETHOSTBYADDR_R
# endif
#endif /* LDAP_R_COMPILE */

char *ldap_pvt_ctime( const time_t *tp, char *buf )
{
#ifdef USE_CTIME_R
# if (CTIME_R_NARGS > 3) || (CTIME_R_NARGS < 2)
#	error "CTIME_R_NARGS should be 2 or 3"
# elif CTIME_R_NARGS > 2 && defined(CTIME_R_RETURNS_INT)
	return( ctime_r(tp,buf,26) < 0 ? 0 : buf );
# elif CTIME_R_NARGS > 2
	return ctime_r(tp,buf,26);
# else
	return ctime_r(tp,buf);
# endif	  

#else

# ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_lock( &ldap_int_ctime_mutex );
# endif

	AC_MEMCPY( buf, ctime(tp), 26 );

# ifdef LDAP_R_COMPILE
	ldap_pvt_thread_mutex_unlock( &ldap_int_ctime_mutex );
# endif

	return buf;
#endif	
}

#define BUFSTART (1024-32)
#define BUFMAX (32*1024-32)

#if defined(LDAP_R_COMPILE)
static char *safe_realloc( char **buf, int len );

#if !(defined(HAVE_GETHOSTBYNAME_R) && defined(HAVE_GETHOSTBYADDR_R))
static int copy_hostent( struct hostent *res,
	char **buf, struct hostent * src );
#endif
#endif

int ldap_pvt_gethostbyname_a(
	const char *name, 
	struct hostent *resbuf,
	char **buf,
	struct hostent **result,
	int *herrno_ptr )
{
#if defined( HAVE_GETHOSTBYNAME_R )

# define NEED_SAFE_REALLOC 1   
	int r=-1;
	int buflen=BUFSTART;
	*buf = NULL;
	for(;buflen<BUFMAX;) {
		if (safe_realloc( buf, buflen )==NULL)
			return r;

#if (GETHOSTBYNAME_R_NARGS < 6)
		*result=gethostbyname_r( name, resbuf, *buf, buflen, herrno_ptr );
		r = (*result == NULL) ?  -1 : 0;
#else
		r = gethostbyname_r( name, resbuf, *buf,
			buflen, result, herrno_ptr );
#endif

		Debug( LDAP_DEBUG_TRACE, "ldap_pvt_gethostbyname_a: host=%s, r=%d\n",
		       name, r, 0 );

#ifdef NETDB_INTERNAL
		if ((r<0) &&
			(*herrno_ptr==NETDB_INTERNAL) &&
			(errno==ERANGE))
		{
			buflen*=2;
			continue;
	 	}
#endif
		return r;
	}
	return -1;
#elif defined( LDAP_R_COMPILE )
# define NEED_COPY_HOSTENT   
	struct hostent *he;
	int	retval;
	*buf = NULL;
	
	ldap_pvt_thread_mutex_lock( &ldap_int_resolv_mutex );
	
	he = gethostbyname( name );
	
	if (he==NULL) {
		*herrno_ptr = h_errno;
		retval = -1;
	} else if (copy_hostent( resbuf, buf, he )<0) {
		*herrno_ptr = -1;
		retval = -1;
	} else {
		*result = resbuf;
		retval = 0;
	}
	
	ldap_pvt_thread_mutex_unlock( &ldap_int_resolv_mutex );
	
	return retval;
#else	
	*buf = NULL;
	*result = gethostbyname( name );

	if (*result!=NULL) {
		return 0;
	}

	*herrno_ptr = h_errno;
	
	return -1;
#endif	
}

#if !defined( HAVE_GETNAMEINFO ) && !defined( HAVE_HSTRERROR )
static const char *
hp_strerror( int err )
{
	switch (err) {
	case HOST_NOT_FOUND:	return _("Host not found (authoritative)");
	case TRY_AGAIN:			return _("Host not found (server fail?)");
	case NO_RECOVERY:		return _("Non-recoverable failure");
	case NO_DATA:			return _("No data of requested type");
#ifdef NETDB_INTERNAL
	case NETDB_INTERNAL:	return STRERROR( errno );
#endif
	}
	return _("Unknown resolver error");
}
#endif

int ldap_pvt_get_hname(
	const struct sockaddr *sa,
	int len,
	char *name,
	int namelen,
	char **err )
{
	int rc;
#if defined( HAVE_GETNAMEINFO )

#if defined( LDAP_R_COMPILE )
	ldap_pvt_thread_mutex_lock( &ldap_int_resolv_mutex );
#endif
	rc = getnameinfo( sa, len, name, namelen, NULL, 0, 0 );
#if defined( LDAP_R_COMPILE )
	ldap_pvt_thread_mutex_unlock( &ldap_int_resolv_mutex );
#endif
	if ( rc ) *err = (char *)AC_GAI_STRERROR( rc );
	return rc;

#else /* !HAVE_GETNAMEINFO */
	char *addr;
	int alen;
	struct hostent *hp = NULL;
#ifdef HAVE_GETHOSTBYADDR_R
	struct hostent hb;
	int buflen=BUFSTART, h_errno;
	char *buf=NULL;
#endif

#ifdef LDAP_PF_INET6
	if (sa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)sa;
		addr = (char *)&sin->sin6_addr;
		alen = sizeof(sin->sin6_addr);
	} else
#endif
	if (sa->sa_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		addr = (char *)&sin->sin_addr;
		alen = sizeof(sin->sin_addr);
	} else {
		rc = NO_RECOVERY;
		*err = (char *)HSTRERROR( rc );
		return rc;
	}
#if defined( HAVE_GETHOSTBYADDR_R )
	for(;buflen<BUFMAX;) {
		if (safe_realloc( &buf, buflen )==NULL) {
			*err = (char *)STRERROR( ENOMEM );
			return ENOMEM;
		}
#if (GETHOSTBYADDR_R_NARGS < 8)
		hp=gethostbyaddr_r( addr, alen, sa->sa_family,
			&hb, buf, buflen, &h_errno );
		rc = (hp == NULL) ? -1 : 0;
#else
		rc = gethostbyaddr_r( addr, alen, sa->sa_family,
			&hb, buf, buflen, 
			&hp, &h_errno );
#endif
#ifdef NETDB_INTERNAL
		if ((rc<0) &&
			(h_errno==NETDB_INTERNAL) &&
			(errno==ERANGE))
		{
			buflen*=2;
			continue;
		}
#endif
		break;
	}
	if (hp) {
		strncpy( name, hp->h_name, namelen );
	} else {
		*err = (char *)HSTRERROR( h_errno );
	}
	LDAP_FREE(buf);
#else /* HAVE_GETHOSTBYADDR_R */

#if defined( LDAP_R_COMPILE )
	ldap_pvt_thread_mutex_lock( &ldap_int_resolv_mutex );
#endif
	hp = gethostbyaddr( addr, alen, sa->sa_family );
	if (hp) {
		strncpy( name, hp->h_name, namelen );
		rc = 0;
	} else {
		rc = h_errno;
		*err = (char *)HSTRERROR( h_errno );
	}
#if defined( LDAP_R_COMPILE )
	ldap_pvt_thread_mutex_unlock( &ldap_int_resolv_mutex );
#endif

#endif	/* !HAVE_GETHOSTBYADDR_R */
	return rc;
#endif	/* !HAVE_GETNAMEINFO */
}

int ldap_pvt_gethostbyaddr_a(
	const char *addr,
	int len,
	int type,
	struct hostent *resbuf,
	char **buf,
	struct hostent **result,
	int *herrno_ptr )
{
#if defined( HAVE_GETHOSTBYADDR_R )

# undef NEED_SAFE_REALLOC
# define NEED_SAFE_REALLOC   
	int r=-1;
	int buflen=BUFSTART;
	*buf = NULL;   
	for(;buflen<BUFMAX;) {
		if (safe_realloc( buf, buflen )==NULL)
			return r;
#if (GETHOSTBYADDR_R_NARGS < 8)
		*result=gethostbyaddr_r( addr, len, type,
			resbuf, *buf, buflen, herrno_ptr );
		r = (*result == NULL) ? -1 : 0;
#else
		r = gethostbyaddr_r( addr, len, type,
			resbuf, *buf, buflen, 
			result, herrno_ptr );
#endif

#ifdef NETDB_INTERNAL
		if ((r<0) &&
			(*herrno_ptr==NETDB_INTERNAL) &&
			(errno==ERANGE))
		{
			buflen*=2;
			continue;
		}
#endif
		return r;
	}
	return -1;
#elif defined( LDAP_R_COMPILE )
# undef NEED_COPY_HOSTENT
# define NEED_COPY_HOSTENT   
	struct hostent *he;
	int	retval;
	*buf = NULL;   
	
	ldap_pvt_thread_mutex_lock( &ldap_int_resolv_mutex );
	
	he = gethostbyaddr( addr, len, type );
	
	if (he==NULL) {
		*herrno_ptr = h_errno;
		retval = -1;
	} else if (copy_hostent( resbuf, buf, he )<0) {
		*herrno_ptr = -1;
		retval = -1;
	} else {
		*result = resbuf;
		retval = 0;
	}
	
	ldap_pvt_thread_mutex_unlock( &ldap_int_resolv_mutex );
	
	return retval;

#else /* gethostbyaddr() */
	*buf = NULL;   
	*result = gethostbyaddr( addr, len, type );

	if (*result!=NULL) {
		return 0;
	}
	return -1;
#endif	
}
/* 
 * ldap_int_utils_init() should be called before any other function.
 */

void ldap_int_utils_init( void )
{
	static int done=0;
	if (done)
	  return;
	done=1;

#ifdef LDAP_R_COMPILE
#if !defined( USE_CTIME_R ) && !defined( HAVE_REENTRANT_FUNCTIONS )
	ldap_pvt_thread_mutex_init( &ldap_int_ctime_mutex );
#endif
	ldap_pvt_thread_mutex_init( &ldap_int_resolv_mutex );

#ifdef HAVE_CYRUS_SASL
	ldap_pvt_thread_mutex_init( &ldap_int_sasl_mutex );
#endif
#endif

	/* call other module init functions here... */
}

#if defined( NEED_COPY_HOSTENT )
# undef NEED_SAFE_REALLOC
#define NEED_SAFE_REALLOC

static char *cpy_aliases(
	char ***tgtio,
	char *buf,
	char **src )
{
	int len;
	char **tgt=*tgtio;
	for( ; (*src) ; src++ ) {
		len = strlen( *src ) + 1;
		AC_MEMCPY( buf, *src, len );
		*tgt++=buf;
		buf+=len;
	}
	*tgtio=tgt;   
	return buf;
}

static char *cpy_addresses(
	char ***tgtio,
	char *buf,
	char **src,
	int len )
{
   	char **tgt=*tgtio;
	for( ; (*src) ; src++ ) {
		AC_MEMCPY( buf, *src, len );
		*tgt++=buf;
		buf+=len;
	}
	*tgtio=tgt;      
	return buf;
}

static int copy_hostent(
	struct hostent *res,
	char **buf,
	struct hostent * src )
{
	char	**p;
	char	**tp;
	char	*tbuf;
	int	name_len;
	int	n_alias=0;
	int	total_alias_len=0;
	int	n_addr=0;
	int	total_addr_len=0;
	int	total_len;
	  
	/* calculate the size needed for the buffer */
	name_len = strlen( src->h_name ) + 1;
	
	if( src->h_aliases != NULL ) {
		for( p = src->h_aliases; (*p) != NULL; p++ ) {
			total_alias_len += strlen( *p ) + 1;
			n_alias++; 
		}
	}

	if( src->h_addr_list != NULL ) {
		for( p = src->h_addr_list; (*p) != NULL; p++ ) {
			n_addr++;
		}
		total_addr_len = n_addr * src->h_length;
	}
	
	total_len = (n_alias + n_addr + 2) * sizeof( char * ) +
		total_addr_len + total_alias_len + name_len;
	
	if (safe_realloc( buf, total_len )) {			 
		tp = (char **) *buf;
		tbuf = *buf + (n_alias + n_addr + 2) * sizeof( char * );
		AC_MEMCPY( res, src, sizeof( struct hostent ) );
		/* first the name... */
		AC_MEMCPY( tbuf, src->h_name, name_len );
		res->h_name = tbuf; tbuf+=name_len;
		/* now the aliases */
		res->h_aliases = tp;
		if ( src->h_aliases != NULL ) {
			tbuf = cpy_aliases( &tp, tbuf, src->h_aliases );
		}
		*tp++=NULL;
		/* finally the addresses */
		res->h_addr_list = tp;
		if ( src->h_addr_list != NULL ) {
			tbuf = cpy_addresses( &tp, tbuf, src->h_addr_list, src->h_length );
		}
		*tp++=NULL;
		return 0;
	}
	return -1;
}
#endif

#if defined( NEED_SAFE_REALLOC )
static char *safe_realloc( char **buf, int len )
{
	char *tmpbuf;
	tmpbuf = LDAP_REALLOC( *buf, len );
	if (tmpbuf) {
		*buf=tmpbuf;
	} 
	return tmpbuf;
}
#endif

char * ldap_pvt_get_fqdn( char *name )
{
	char *fqdn, *ha_buf;
	char hostbuf[MAXHOSTNAMELEN+1];
	struct hostent *hp, he_buf;
	int rc, local_h_errno;

	if( name == NULL ) {
		if( gethostname( hostbuf, MAXHOSTNAMELEN ) == 0 ) {
			hostbuf[MAXHOSTNAMELEN] = '\0';
			name = hostbuf;
		} else {
			name = "localhost";
		}
	}

	rc = ldap_pvt_gethostbyname_a( name,
		&he_buf, &ha_buf, &hp, &local_h_errno );

	if( rc < 0 || hp == NULL || hp->h_name == NULL ) {
		fqdn = LDAP_STRDUP( name );
	} else {
		fqdn = LDAP_STRDUP( hp->h_name );
	}

	LDAP_FREE( ha_buf );
	return fqdn;
}

#if ( defined( HAVE_GETADDRINFO ) || defined( HAVE_GETNAMEINFO ) ) \
	&& !defined( HAVE_GAI_STRERROR )
char *ldap_pvt_gai_strerror (int code) {
	static struct {
		int code;
		const char *msg;
	} values[] = {
#ifdef EAI_ADDRFAMILY
		{ EAI_ADDRFAMILY, N_("Address family for hostname not supported") },
#endif
		{ EAI_AGAIN, N_("Temporary failure in name resolution") },
		{ EAI_BADFLAGS, N_("Bad value for ai_flags") },
		{ EAI_FAIL, N_("Non-recoverable failure in name resolution") },
		{ EAI_FAMILY, N_("ai_family not supported") },
		{ EAI_MEMORY, N_("Memory allocation failure") },
#ifdef EAI_NODATA
		{ EAI_NODATA, N_("No address associated with hostname") },
#endif    
		{ EAI_NONAME, N_("Name or service not known") },
		{ EAI_SERVICE, N_("Servname not supported for ai_socktype") },
		{ EAI_SOCKTYPE, N_("ai_socktype not supported") },
		{ EAI_SYSTEM, N_("System error") },
		{ 0, NULL }
	};

	int i;

	for ( i = 0; values[i].msg != NULL; i++ ) {
		if ( values[i].code == code ) {
			return (char *) _(values[i].msg);
		}
	}
	
	return _("Unknown error");
}
#endif
