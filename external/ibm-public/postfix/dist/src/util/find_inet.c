/*	$NetBSD: find_inet.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	find_inet 3
/* SUMMARY
/*	inet-domain name services
/* SYNOPSIS
/*	#include <find_inet.h>
/*
/*	unsigned find_inet_addr(host)
/*	const char *host;
/*
/*	int	find_inet_port(port, proto)
/*	const char *port;
/*	const char *proto;
/* DESCRIPTION
/*	These functions translate network address information from
/*	between printable form to the internal the form used by the
/*	BSD TCP/IP network software.
/*
/*	find_inet_addr() translates a symbolic or numerical hostname.
/*	This function is deprecated. Use hostname_to_hostaddr() instead.
/*
/*	find_inet_port() translates a symbolic or numerical port name.
/* BUGS
/*	find_inet_addr() ignores all but the first address listed for
/*	a symbolic hostname.
/* DIAGNOSTICS
/*	Lookup and conversion errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System libraries. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

/* Application-specific. */

#include "msg.h"
#include "stringops.h"
#include "find_inet.h"
#include "known_tcp_ports.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifdef TEST
extern NORETURN PRINTFLIKE(1, 2) test_msg_fatal(const char *,...);

#define msg_fatal test_msg_fatal
#endif

/* find_inet_addr - translate numerical or symbolic host name */

unsigned find_inet_addr(const char *host)
{
    struct in_addr addr;
    struct hostent *hp;

    addr.s_addr = inet_addr(host);
    if ((addr.s_addr == INADDR_NONE) || (addr.s_addr == 0)) {
	if ((hp = gethostbyname(host)) == 0)
	    msg_fatal("host not found: %s", host);
	if (hp->h_addrtype != AF_INET)
	    msg_fatal("unexpected address family: %d", hp->h_addrtype);
	if (hp->h_length != sizeof(addr))
	    msg_fatal("unexpected address length %d", hp->h_length);
	memcpy((void *) &addr, hp->h_addr, hp->h_length);
    }
    return (addr.s_addr);
}

/* find_inet_port - translate numerical or symbolic service name */

int     find_inet_port(const char *service, const char *protocol)
{
    struct servent *sp;
    int     port;

    service = filter_known_tcp_port(service);
    if (alldig(service) && (port = atoi(service)) != 0) {
	if (port < 0 || port > 65535)
	    msg_fatal("bad port number: %s", service);
	return (htons(port));
    } else {
	if ((sp = getservbyname(service, protocol)) == 0)
	    msg_fatal("unknown service: %s/%s", service, protocol);
	return (sp->s_port);
    }
}

#ifdef TEST

#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>

#define STR(x) vstring_str(x)

 /* TODO(wietse) make this a proper VSTREAM interface */

/* vstream_swap - kludge to capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

jmp_buf test_fatal_jbuf;

#undef msg_fatal

/* test_msg_fatal - does not return, and does not terminate */

void    test_msg_fatal(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    vmsg_warn(fmt, ap);
    va_end(ap);
    longjmp(test_fatal_jbuf, 1);
}

struct association {
    const char *lhs;			/* service name */
    const char *rhs;			/* service port */
};

struct test_case {
    const char *label;			/* identifies test case */
    struct association associations[10];
    const char *service;
    const char *proto;
    const char *exp_warning;		/* expected error */
    int     exp_hport;			/* expected port, host byte order */
};

struct test_case test_cases[] = {
    {"good-symbolic",
	 /* association */ {{"foobar", "25252"}, 0},
	 /* service */ "foobar",
	 /* proto */ "tcp",
	 /* exp_warning */ "",
	 /* exp_hport */ 25252,
    },
    {"good-numeric",
	 /* association */ {{"foobar", "25252"}, 0},
	 /* service */ "25252",
	 /* proto */ "tcp",
	 /* exp_warning */ "",
	 /* exp_hport */ 25252,
    },
    {"bad-symbolic",
	 /* association */ {{"foobar", "25252"}, 0},
	 /* service */ "an-impossible-name",
	 /* proto */ "tcp",
	 /* exp_warning */ "find_inet: warning: unknown service: an-impossible-name/tcp\n",
    },
    {"bad-numeric",
	 /* association */ {{"foobar", "25252"}, 0},
	 /* service */ "123456",
	 /* proto */ "tcp",
	 /* exp_warning */ "find_inet: warning: bad port number: 123456\n",
    },
};

int main(int argc, char **argv) {
    struct test_case *tp;
    struct association *ap;
    int     pass = 0;
    int     fail = 0;
    const char *err;
    int     test_failed;
    int     nport;
    VSTRING *msg_buf;
    VSTREAM *memory_stream;

    msg_vstream_init("find_inet", VSTREAM_ERR);
    msg_buf = vstring_alloc(100);

    for (tp = test_cases; tp->label != 0; tp++) {
	test_failed = 0;
	VSTRING_RESET(msg_buf);
	VSTRING_TERMINATE(msg_buf);
	clear_known_tcp_ports();
	for (err = 0, ap = tp->associations; err == 0 && ap->lhs != 0; ap++)
	    err = add_known_tcp_port(ap->lhs, ap->rhs);
	if (err != 0) {
	    msg_warn("test case %s: got err: \"%s\"", tp->label, err);
	    test_failed = 1;
	} else {
	    if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
		msg_fatal("open memory stream: %m");
	    vstream_swap(VSTREAM_ERR, memory_stream);
	    if (setjmp(test_fatal_jbuf) == 0)
		nport = find_inet_port(tp->service, tp->proto);
	    vstream_swap(memory_stream, VSTREAM_ERR);
	    if (vstream_fclose(memory_stream))
		msg_fatal("close memory stream: %m");
	    if (strcmp(STR(msg_buf), tp->exp_warning) != 0) {
		msg_warn("test case %s: got error: \"%s\", want: \"%s\"",
			 tp->label, STR(msg_buf), tp->exp_warning);
		test_failed = 1;
	    } else if (tp->exp_warning[0] == 0) {
		if (ntohs(nport) != tp->exp_hport) {
		    msg_warn("test case %s: got port \"%d\", want: \"%d\"",
			     tp->label, ntohs(nport), tp->exp_hport);
		    test_failed = 1;
		}
	    }
	}
	if (test_failed) {
	    msg_info("%s: FAIL", tp->label);
	    fail++;
	} else {
	    msg_info("%s: PASS", tp->label);
	    pass++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    vstring_free(msg_buf);
    exit(fail != 0);
}

#endif
