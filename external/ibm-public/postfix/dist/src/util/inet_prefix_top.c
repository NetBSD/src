/*	$NetBSD: inet_prefix_top.c,v 1.2.2.2 2023/12/25 12:43:37 martin Exp $	*/

/*++
/* NAME
/*	inet_prefix_top 3
/* SUMMARY
/*	convert net/mask to printable string
/* SYNOPSIS
/*	#include <inet_prefix_top.h>
/*
/*	char	*inet_prefix_top(
/*	int	family,
/*	const void *src,
/*	int	prefix_len)
/* DESCRIPTION
/*	inet_prefix_top() prints the network portion of the specified
/*	IPv4 or IPv6 address, null bits for the host portion, and
/*	the prefix length if it is shorter than the address.
/*	The result should be passed to myfree(). The code can
/*	handle addresses of any length, and bytes of any width.
/*
/*	Arguments:
/* .IP af
/*	The address family, as with inet_ntop().
/* .IP src
/*	Pointer to storage for an IPv4 or IPv6 address, as with
/*	inet_ntop().
/* .IP prefix_len
/*	The number of most-significant bits in \fBsrc\fR that should
/*	not be cleared.
/* DIAGNOSTICS
/*	Panic: unexpected protocol family, bad prefix length. Fatal
/*	errors: address conversion error.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <mask_addr.h>
#include <msg.h>
#include <inet_addr_sizes.h>
#include <inet_prefix_top.h>
#include <vstring.h>

/* inet_prefix_top - printable net/mask pattern */

char   *inet_prefix_top(int af, const void *src, int prefix_len)
{
    const char myname[] = "inet_prefix_top";
    union {
	struct in_addr in_addr;
	struct in6_addr in6_addr;
    }       u;
    VSTRING *buf;
    const INET_ADDR_SIZES *sp;

    /*
     * Sanity checks. XXX We use msg_fatal() because mail_conf_int() does not
     * (yet) support non-negative integers.
     */
    if ((sp = inet_addr_sizes(af)) == 0)
	msg_panic("%s: unexpected address family: %d", myname, af);
    if (prefix_len > sp->addr_bitcount || prefix_len < 0)
	msg_fatal("%s: bad %s address prefix length: %d",
		  myname, sp->ipproto_str, prefix_len);

    /*
     * Strip a copy of the input address. When allocating the result memory,
     * add 1 for the string terminator from inet_ntop(), or 1 for the '/'
     * before the prefix. We should not rely on vstring(3)'s safety byte.
     */
    memcpy((void *) &u, src, sp->addr_bytecount);
    if (prefix_len < sp->addr_bitcount) {
	mask_addr((unsigned char *) &u, sp->addr_bytecount, prefix_len);
	buf = vstring_alloc(sp->addr_strlen + sp->addr_bitcount_strlen + 1);
    } else {
	buf = vstring_alloc(sp->addr_strlen + 1);
    }

    /*
     * Convert the result to string, and append the optional /prefix.
     */
    if (inet_ntop(af, &u, vstring_str(buf), vstring_avail(buf)) == 0)
	msg_fatal("%s: inet_ntop: %m", myname);
    vstring_set_payload_size(buf, strlen(vstring_str(buf)));
    if (prefix_len < sp->addr_bitcount)
	vstring_sprintf_append(buf, "/%d", prefix_len);
    return (vstring_export(buf));
}

#ifdef TEST

#include <stdlib.h>
#include <msg_vstream.h>
#include <name_code.h>

 /*
  * TODO: add test cases for fatal and panic errors, intercept msg_fatal()
  * and msg_panic(), and verify the expected error messages.
  */
typedef struct TEST_CASE {
    int     in_af;
    int     in_prefix_len;
    const char *exp_prefix;
} TEST_CASE;

static TEST_CASE test_cases[] = {
    AF_INET, 32, "255.255.255.255",
    AF_INET, 28, "255.255.255.240/28",
    AF_INET, 4, "240.0.0.0/4",
    AF_INET, 0, "0.0.0.0/0",
    AF_INET6, 128, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
    AF_INET6, 124, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff0/124",
    AF_INET6, 4, "f000::/4",
    AF_INET6, 0, "::/0",
};

const NAME_CODE af_map[] = {
    "AF_INET", AF_INET,
    "AF_INET6", AF_INET6,
    0,
};

#define TEST_CASE_COUNT (sizeof(test_cases) / sizeof(test_cases[0]))

int     main(int argc, char **argv)
{
    TEST_CASE *tp;
    union {
	struct in_addr in_addr;
	struct in6_addr in6_addr;
    }       u;
    char   *act_prefix;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    memset(&u, ~0, sizeof(u));

    for (tp = test_cases; tp < test_cases + TEST_CASE_COUNT; tp++) {
	msg_info("RUN  %s/%d", str_name_code(af_map, tp->in_af),
		 tp->in_prefix_len);
	act_prefix = inet_prefix_top(tp->in_af, &u, tp->in_prefix_len);
	if (strcmp(act_prefix, tp->exp_prefix) != 0) {
	    msg_warn("got \"%s\", want \"%s\"", act_prefix, tp->exp_prefix);
	    fail += 1;
	    msg_info("FAIL %s/%d", str_name_code(af_map, tp->in_af),
		     tp->in_prefix_len);
	} else {
	    pass += 1;
	    msg_info("PASS %s/%d", str_name_code(af_map, tp->in_af),
		     tp->in_prefix_len);
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    return (fail > 0);
}

#endif					/* TEST */
