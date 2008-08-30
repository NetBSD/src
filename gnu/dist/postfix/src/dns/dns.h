#ifndef _DNS_H_INCLUDED_
#define _DNS_H_INCLUDED_

/*++
/* NAME
/*	dns 3h
/* SUMMARY
/*	domain name service lookup
/* SYNOPSIS
/*	#include <dns.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <netinet/in.h>
#include <arpa/nameser.h>
#ifdef RESOLVE_H_NEEDS_STDIO_H
#include <stdio.h>
#endif
#ifdef RESOLVE_H_NEEDS_NAMESER8_COMPAT_H
#include <nameser8_compat.h>
#endif
#include <resolv.h>

 /*
  * Name server compatibility. These undocumented macros appear in the file
  * <arpa/nameser.h>, but since they are undocumented we should not count on
  * their presence, and so they are included here just in case.
  */
#ifndef GETSHORT

#define GETSHORT(s, cp) { \
	unsigned char *t_cp = (u_char *)(cp); \
	(s) = ((unsigned)t_cp[0] << 8) \
	    | ((unsigned)t_cp[1]) \
	    ; \
	(cp) += 2; \
}

#define GETLONG(l, cp) { \
	unsigned char *t_cp = (u_char *)(cp); \
	(l) = ((unsigned)t_cp[0] << 24) \
	    | ((unsigned)t_cp[1] << 16) \
	    | ((unsigned)t_cp[2] << 8) \
	    | ((unsigned)t_cp[3]) \
	    ; \
	(cp) += 4; \
}

#endif

 /*
  * SunOS 4 needs this.
  */
#ifndef T_TXT
#define T_TXT	16
#endif

 /*
  * Utility library.
  */
#include <vstring.h>
#include <sock_addr.h>
#include <myaddrinfo.h>

 /*
  * Structure for fixed resource record data.
  */
typedef struct DNS_FIXED {
    unsigned short type;		/* T_A, T_CNAME, etc. */
    unsigned short class;		/* C_IN, etc. */
    unsigned int ttl;			/* always */
    unsigned length;			/* record length */
} DNS_FIXED;

 /*
  * Structure of a DNS resource record after expansion. The components are
  * named after the things one can expect to find in a DNS resource record.
  */
typedef struct DNS_RR {
    char   *qname;			/* query name, mystrdup()ed */
    char   *rname;			/* reply name, mystrdup()ed */
    unsigned short type;		/* T_A, T_CNAME, etc. */
    unsigned short class;		/* C_IN, etc. */
    unsigned int ttl;			/* always */
    unsigned short pref;		/* T_MX only */
    struct DNS_RR *next;		/* linkage */
    size_t  data_len;			/* actual data size */
    char    data[1];			/* actually a bunch of data */
} DNS_RR;

 /*
  * dns_strerror.c
  */
extern const char *dns_strerror(unsigned);

 /*
  * dns_strtype.c
  */
extern const char *dns_strtype(unsigned);
extern unsigned dns_type(const char *);

 /*
  * dns_rr.c
  */
extern DNS_RR *dns_rr_create(const char *, const char *,
			             ushort, ushort,
			             unsigned, unsigned,
			             const char *, size_t);
extern void dns_rr_free(DNS_RR *);
extern DNS_RR *dns_rr_copy(DNS_RR *);
extern DNS_RR *dns_rr_append(DNS_RR *, DNS_RR *);
extern DNS_RR *dns_rr_sort(DNS_RR *, int (*) (DNS_RR *, DNS_RR *));
extern int dns_rr_compare_pref(DNS_RR *, DNS_RR *);
extern DNS_RR *dns_rr_shuffle(DNS_RR *);
extern DNS_RR *dns_rr_remove(DNS_RR *, DNS_RR *);

 /*
  * dns_rr_to_pa.c
  */
extern const char *dns_rr_to_pa(DNS_RR *, MAI_HOSTADDR_STR *);

 /*
  * dns_sa_to_rr.c
  */
extern DNS_RR *dns_sa_to_rr(const char *, unsigned, struct sockaddr *);

 /*
  * dns_rr_to_sa.c
  */
extern int dns_rr_to_sa(DNS_RR *, unsigned, struct sockaddr *, SOCKADDR_SIZE *);

 /*
  * dns_rr_eq_sa.c
  */
extern int dns_rr_eq_sa(DNS_RR *, struct sockaddr *);

#ifdef HAS_IPV6
#define DNS_RR_EQ_SA(rr, sa) \
    ((SOCK_ADDR_IN_FAMILY(sa) == AF_INET && (rr)->type == T_A \
     && SOCK_ADDR_IN_ADDR(sa).s_addr == IN_ADDR((rr)->data).s_addr) \
    || (SOCK_ADDR_IN_FAMILY(sa) == AF_INET6 && (rr)->type == T_AAAA \
	&& memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)), \
		  (rr)->data, (rr)->data_len) == 0))
#else
#define DNS_RR_EQ_SA(rr, sa) \
    (SOCK_ADDR_IN_FAMILY(sa) == AF_INET && (rr)->type == T_A \
     && SOCK_ADDR_IN_ADDR(sa).s_addr == IN_ADDR((rr)->data).s_addr)
#endif

 /*
  * dns_lookup.c
  */
extern int dns_lookup(const char *, unsigned, unsigned, DNS_RR **,
		              VSTRING *, VSTRING *);
extern int dns_lookup_l(const char *, unsigned, DNS_RR **, VSTRING *,
			        VSTRING *, int,...);
extern int dns_lookup_v(const char *, unsigned, DNS_RR **, VSTRING *,
			        VSTRING *, int, unsigned *);

 /*
  * Request flags.
  */
#define DNS_REQ_FLAG_STOP_OK	(1<<0)
#define DNS_REQ_FLAG_STOP_INVAL	(1<<1)
#define DNS_REQ_FLAG_NONE	(0)

 /*
  * Status codes. Failures must have negative codes so they will not collide
  * with valid counts of answer records etc.
  */
#define DNS_INVAL	(-5)		/* query ok, malformed reply */
#define DNS_FAIL	(-4)		/* query failed, don't retry */
#define DNS_NOTFOUND	(-3)		/* query ok, data not found */
#define DNS_RETRY	(-2)		/* query failed, try again */
#define DNS_RECURSE	(-1)		/* recursion needed */
#define DNS_OK		0		/* query succeeded */

 /*
  * How long can a DNS name be?
  */
#define DNS_NAME_LEN	1024

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
