/*	$NetBSD: memcache_proto.h,v 1.1.1.1.6.2 2013/02/25 00:27:19 tls Exp $	*/

#ifndef _MEMCACHE_PROTO_H_INCLUDED_
#define _MEMCACHE_PROTO_H_INCLUDED_

/*++
/* NAME
/*      memcache_proto 3h
/* SUMMARY
/*	memcache low-level protocol
/* SYNOPSIS
/*      #include <memcache_proto.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int memcache_get(VSTREAM *, VSTRING *, ssize_t);
extern int memcache_vprintf(VSTREAM *, const char *, va_list);
extern int memcache_printf(VSTREAM *, const char *fmt,...);
extern int memcache_fread(VSTREAM *, VSTRING *, ssize_t);
extern int memcache_fwrite(VSTREAM *, const char *, ssize_t);

/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/*--*/

#endif
