/*	$NetBSD: scache.h,v 1.1.1.1.2.2 2009/09/15 06:02:53 snj Exp $	*/

#ifndef _SCACHE_H_INCLUDED_
#define _SCACHE_H_INCLUDED_

/*++
/* NAME
/*      scache 3h
/* SUMMARY
/*      generic session cache API
/* SYNOPSIS
/*      #include <scache.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

typedef struct SCACHE SCACHE;
typedef struct SCACHE_SIZE SCACHE_SIZE;

 /*
  * In order to cache a session, we specify:
  * 
  * - TTL for this session.
  * 
  * - File descriptor.
  * 
  * - Transport name and physical endpoint. The transport name must be included,
  * because fall-back destinations can be transport-dependent, and routing
  * for a given destination may be context dependent.
  * 
  * In the case of SMTP, the physical endpoint is the numerical IP address and
  * TCP port.
  * 
  * - Application-specific endpoint properties.
  * 
  * In the case of SMTP, the properties specify the ESMTP features advertised by
  * the server.
  * 
  * Note: with message delivery transports that have only one endpoint per
  * logical destination, the above is the only information that needs to be
  * maintained in a connection cache.
  */
typedef void (*SCACHE_SAVE_ENDP_FN) (SCACHE *, int, const char *, const char *, int);
typedef int (*SCACHE_FIND_ENDP_FN) (SCACHE *, const char *, VSTRING *);

 /*
  * The following information is stored in order to make a binding from
  * logical destination to physical destination. One logical destination can
  * have multiple physical destinations (and one physical destination can
  * have multiple sessions).
  * 
  * - TTL for this binding.
  * 
  * - Transport name and logical destination.
  * 
  * In the case of SMTP: the next-hop (NOT: fall-back) destination domain and
  * destination network port. It is not useful to create a link for an
  * address literal (but it is not harmful either: it just wastes a few
  * bits). This information specifies the destination domain in [] if no MX
  * lookup is done.
  * 
  * - Application-specific properties.
  * 
  * In case the of SMTP, the properties specify a) whether a physical endpoint
  * is best mx host with respect to a logical or fall-back destination (this
  * information is needed by the loop detection code in smtp_proto.c).
  * 
  * - Transport name and physical endpoint (see above).
  * 
  * Note 1: there is no need to store the binding's MX preference or equivalent
  * with respect to the logical destination; a client should store only the
  * first successful session for a given delivery request (otherwise the
  * client would keep talking to a less preferred server after the cached
  * connection for a more preferred server expires). After a failed delivery,
  * the client should not attempt to cache follow-up sessions with less
  * preferred endpoints under the same logical destination.
  * 
  * Note 2: logical to physical bindings exist independently from cached
  * sessions. The two types of information have independent TTLs; creation
  * and destruction proceed independently. Thus, a logical to physical
  * binding can refer to an endpoint for which all cached connections are
  * occupied or expired.
  */
typedef void (*SCACHE_SAVE_DEST_FN) (SCACHE *, int, const char *, const char *, const char *);
typedef int (*SCACHE_FIND_DEST_FN) (SCACHE *, const char *, VSTRING *, VSTRING *);

 /*
  * Session cache statistics. These are the actual numbers at a specific
  * point in time.
  */
struct SCACHE_SIZE {
    int     dest_count;			/* Nr of destination names */
    int     endp_count;			/* Nr of endpoint adresses */
    int     sess_count;			/* Nr of cached sessions */
};

 /*
  * Generic session cache object. Actual session cache objects are derived
  * types with some additional, cache dependent, private information.
  */
struct SCACHE {
    SCACHE_SAVE_ENDP_FN save_endp;
    SCACHE_FIND_ENDP_FN find_endp;
    SCACHE_SAVE_DEST_FN save_dest;
    SCACHE_FIND_DEST_FN find_dest;
    void    (*size) (struct SCACHE *, SCACHE_SIZE *);
    void    (*free) (struct SCACHE *);
};

extern SCACHE *scache_single_create(void);
extern SCACHE *scache_clnt_create(const char *, int, int, int);
extern SCACHE *scache_multi_create(void);

#define scache_save_endp(scache, ttl, endp_label, endp_prop, fd) \
    (scache)->save_endp((scache), (ttl), (endp_label), (endp_prop), (fd))
#define scache_find_endp(scache, endp_label, endp_prop) \
    (scache)->find_endp((scache), (endp_label), (endp_prop))
#define scache_save_dest(scache, ttl, dest_label, dest_prop, endp_label) \
    (scache)->save_dest((scache), (ttl), (dest_label), (dest_prop), (endp_label))
#define scache_find_dest(scache, dest_label, dest_prop, endp_prop) \
    (scache)->find_dest((scache), (dest_label), (dest_prop), (endp_prop))
#define scache_size(scache, stats) (scache)->size((scache), (stats))
#define scache_free(scache) (scache)->free(scache)

 /*
  * Cache types.
  */
#define SCACHE_TYPE_SINGLE	1	/* single-instance cache */
#define SCACHE_TYPE_CLIENT	2	/* session cache client */
#define SCACHE_TYPE_MULTI	3	/* multi-instance cache */

 /*
  * Client-server protocol.
  */
#define SCACHE_REQ_FIND_ENDP	"find_endp"
#define SCACHE_REQ_SAVE_ENDP	"save_endp"
#define SCACHE_REQ_FIND_DEST	"find_dest"
#define SCACHE_REQ_SAVE_DEST	"save_dest"

 /*
  * Session cache server status codes.
  */
#define SCACHE_STAT_OK		0	/* request completed successfully */
#define SCACHE_STAT_BAD		1	/* malformed request */
#define SCACHE_STAT_FAIL	2	/* request completed unsuccessfully */

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
