/*	$NetBSD: inet_proto.c,v 1.1.1.1.2.2 2009/09/15 06:03:59 snj Exp $	*/

/*++
/* NAME
/*	inet_proto 3
/* SUMMARY
/*	convert protocol names to assorted constants
/* SYNOPSIS
/*	#include <inet_proto.h>
/*
/*	typedef struct {
/* .in +4
/*		unsigned ai_family; /* PF_UNSPEC, PF_INET, or PF_INET6 */
/*		unsigned *ai_family_list; /* PF_INET and/or PF_INET6 */
/*		unsigned *dns_atype_list;/* TAAAA and/or TA */
/*		unsigned char *sa_family_list;/* AF_INET6 and/or AF_INET */
/* .in -4
/* } INET_PROTO_INFO;
/*
/*	INET_PROTO_INFO *inet_proto_init(context, protocols)
/*
/*	INET_PROTO_INFO *inet_proto_info()
/* DESCRIPTION
/*	inet_proto_init() converts a string with protocol names
/*	into null-terminated lists of appropriate constants used
/*	by Postfix library routines.  The idea is that one should
/*	be able to configure an MTA for IPv4 only, without having
/*	to recompile code (what a concept).
/*
/*	Unfortunately, some compilers won't link initialized data
/*	without a function call into the same source module, so
/*	we invoke inet_proto_info() in order to access the result
/*	from inet_proto_init() from within library routines.
/*	inet_proto_info() also conveniently initializes the data
/*	to built-in defaults.
/*
/*	Arguments:
/* .IP context
/*	Typically, a configuration parameter name.
/* .IP protocols
/*	Null-terminated string with protocol names separated by
/*	whitespace and/or commas:
/* .RS
/* .IP INET_PROTO_NAME_ALL
/*	Enable all available IP protocols.
/* .IP INET_PROTO_NAME_IPV4
/*	Enable IP version 4 support.
/* .IP INET_PROTO_NAME_IPV6
/*	Enable IP version 6 support.
/* .RS
/* .PP
/*	Results:
/* .IP ai_family
/*	Only one of PF_UNSPEC, PF_INET, or PF_INET6. This can be
/*	used as input for the getaddrinfo() and getnameinfo()
/*	routines.
/* .IP ai_family_list
/*	One or more of PF_INET or PF_INET6. This can be used as
/*	input for the inet_addr_local() routine.
/* .IP dns_atype_list
/*	One or more of T_AAAA or TA. This can be used as input for
/*	the dns_lookup_v() and dns_lookup_l() routines.
/* .IP sa_family_list
/*	One or more of AF_INET6 or AF_INET. This can be used as an
/*	output filter for the results from the getaddrinfo() and
/*	getnameinfo() routines.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	This module will report if IPv6 is unavailable, and will
/*	disable IPv6 support in Postfix. When IPv6 is the only
/*	selected protocol, this is a fatal error.
/*
/*	Fatal errors: memory allocation problem.
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

/* System library. */

#include <sys_defs.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#ifdef RESOLVE_H_NEEDS_STDIO_H
#include <stdio.h>
#endif
#include <resolv.h>
#include <stdarg.h>
#include <unistd.h>

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <myaddrinfo.h>
#include <name_mask.h>
#include <inet_proto.h>

 /*
  * Application-specific.
  */

 /*
  * Run-time initialization, so we can work around LINUX where IPv6 falls
  * flat on its face because it is not turned on in the kernel.
  */
INET_PROTO_INFO *inet_proto_table = 0;

 /*
  * Infrastructure: lookup table with the protocol names that we support.
  */
#define INET_PROTO_MASK_IPV4	(1<<0)
#define INET_PROTO_MASK_IPV6	(1<<1)

static const NAME_MASK proto_table[] = {
#ifdef HAS_IPV6
    INET_PROTO_NAME_ALL, INET_PROTO_MASK_IPV4 | INET_PROTO_MASK_IPV6,
    INET_PROTO_NAME_IPV6, INET_PROTO_MASK_IPV6,
#else
    INET_PROTO_NAME_ALL, INET_PROTO_MASK_IPV4,
#endif
    INET_PROTO_NAME_IPV4, INET_PROTO_MASK_IPV4,
    0,
};

/* make_uchar_vector - create and initialize uchar vector */

static unsigned char *make_uchar_vector(int len,...)
{
    const char *myname = "make_uchar_vector";
    va_list ap;
    int     count;
    unsigned char *vp;

    va_start(ap, len);
    if (len <= 0)
	msg_panic("%s: bad vector length: %d", myname, len);
    vp = (unsigned char *) mymalloc(sizeof(*vp) * len);
    for (count = 0; count < len; count++)
	vp[count] = va_arg(ap, unsigned);
    va_end(ap);
    return (vp);
}

/* make_unsigned_vector - create and initialize integer vector */

static unsigned *make_unsigned_vector(int len,...)
{
    const char *myname = "make_unsigned_vector";
    va_list ap;
    int     count;
    unsigned *vp;

    va_start(ap, len);
    if (len <= 0)
	msg_panic("%s: bad vector length: %d", myname, len);
    vp = (unsigned *) mymalloc(sizeof(*vp) * len);
    for (count = 0; count < len; count++)
	vp[count] = va_arg(ap, unsigned);
    va_end(ap);
    return (vp);
}

/* inet_proto_free - destroy data */

static void inet_proto_free(INET_PROTO_INFO *pf)
{
    myfree((char *) pf->ai_family_list);
    myfree((char *) pf->dns_atype_list);
    myfree((char *) pf->sa_family_list);
    myfree((char *) pf);
}

/* inet_proto_init - convert protocol names to library inputs */

INET_PROTO_INFO *inet_proto_init(const char *context, const char *protocols)
{
    const char *myname = "inet_proto";
    INET_PROTO_INFO *pf;
    int     inet_proto_mask;
    int     sock;

    /*
     * Store addess family etc. info as null-terminated vectors. If that
     * breaks because we must be able to store nulls, we'll deal with the
     * additional complexity.
     * 
     * XXX Use compile-time initialized data templates instead of building the
     * reply on the fly.
     */
    inet_proto_mask = name_mask(context, proto_table, protocols);
    switch (inet_proto_mask) {
#ifdef HAS_IPV6
    case INET_PROTO_MASK_IPV6:
	if ((sock = socket(PF_INET6, SOCK_STREAM, 0)) >= 0) {
	    close(sock);
	    pf = (INET_PROTO_INFO *) mymalloc(sizeof(*pf));
	    pf->ai_family = PF_INET6;
	    pf->ai_family_list = make_unsigned_vector(2, PF_INET6, 0);
	    pf->dns_atype_list = make_unsigned_vector(2, T_AAAA, 0);
	    pf->sa_family_list = make_uchar_vector(2, AF_INET6, 0);
	    break;
	} else if (errno == EAFNOSUPPORT) {
	    msg_fatal("%s: IPv6 support is disabled: %m", context);
	} else {
	    msg_fatal("socket: %m");
	}
    case (INET_PROTO_MASK_IPV6 | INET_PROTO_MASK_IPV4):
	if ((sock = socket(PF_INET6, SOCK_STREAM, 0)) >= 0) {
	    close(sock);
	    pf = (INET_PROTO_INFO *) mymalloc(sizeof(*pf));
	    pf->ai_family = PF_UNSPEC;
	    pf->ai_family_list = make_unsigned_vector(3, PF_INET, PF_INET6, 0);
	    pf->dns_atype_list = make_unsigned_vector(3, T_A, T_AAAA, 0);
	    pf->sa_family_list = make_uchar_vector(3, AF_INET, AF_INET6, 0);
	    break;
	} else if (errno == EAFNOSUPPORT) {
	    msg_warn("%s: IPv6 support is disabled: %m", context);
	    msg_warn("%s: configuring for IPv4 support only", context);
	    /* FALLTHROUGH */
	} else {
	    msg_fatal("socket: %m");
	}
#endif
    case INET_PROTO_MASK_IPV4:
	pf = (INET_PROTO_INFO *) mymalloc(sizeof(*pf));
	pf->ai_family = PF_INET;
	pf->ai_family_list = make_unsigned_vector(2, PF_INET, 0);
	pf->dns_atype_list = make_unsigned_vector(2, T_A, 0);
	pf->sa_family_list = make_uchar_vector(2, AF_INET, 0);
	break;
    default:
	msg_panic("%s: bad inet_proto_mask 0x%x", myname, inet_proto_mask);
    }
    if (inet_proto_table)
	inet_proto_free(inet_proto_table);
    return (inet_proto_table = pf);
}

#ifdef TEST

 /*
  * Small driver for unit tests.
  */
int     main(int argc, char **argv)
{
    const char *myname = argv[0];
    INET_PROTO_INFO *pf;

    if (argc < 2)
	msg_fatal("usage: %s protocol(s)...", myname);

    while (*++argv) {
	msg_info("=== %s ===", *argv);
	if (**argv)
	    inet_proto_init(myname, *argv);
	pf = inet_proto_table;
	msg_info("ai_family = %u", pf->ai_family);
	msg_info("ai_family_list = %u %u...",
		 pf->ai_family_list[0], pf->ai_family_list[1]);
	msg_info("dns_atype_list = %u %u...",
		 pf->dns_atype_list[0], pf->dns_atype_list[1]);
	msg_info("sa_family_list = %u %u...",
		 pf->sa_family_list[0], pf->sa_family_list[1]);
    }
    return (0);
}

#endif
