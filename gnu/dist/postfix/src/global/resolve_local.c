/*++
/* NAME
/*	resolve_local 3
/* SUMMARY
/*	determine if address resolves to local mail system
/* SYNOPSIS
/*	#include <resolve_local.h>
/*
/*	void	resolve_local_init()
/*
/*	int	resolve_local(host)
/*	const char *host;
/* DESCRIPTION
/*	resolve_local() determines if the named domain resolves to the
/*	local mail system, either by case-insensitive exact match
/*	against the domains, files or tables listed in $mydestination,
/*	or by any of the network addresses listed in $inet_interfaces.
/*
/*	resolve_local_init() performs initialization. If this routine is
/*	not called explicitly ahead of time, it will be called on the fly.
/* BUGS
/*	Calling resolve_local_init() on the fly is an incomplete solution.
/*	It is bound to fail with applications that enter a chroot jail.
/* SEE ALSO
/*	own_inet_addr(3), find out my own network interfaces
/*	match_list(3), generic pattern matching engine
/*	match_ops(3), generic pattern matching operators
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
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <string_list.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>
#include <resolve_local.h>

/* Application-specific */

static STRING_LIST *resolve_local_list;

/* resolve_local_init - initialize lookup table */

void    resolve_local_init(void)
{
    if (resolve_local_list)
	msg_panic("resolve_local_init: duplicate initialization");
    resolve_local_list = string_list_init(var_mydest);
}

/* resolve_local - match address against list of local destinations */

int     resolve_local(const char *addr)
{
    char   *saved_addr = mystrdup(addr);
    char   *dest;
#ifdef INET6
    struct addrinfo hints, *res, *res0;
    int error;
#else
    struct in_addr ipaddr;
#endif
    int     len;

#define RETURN(x) { myfree(saved_addr); return(x); }

    if (resolve_local_list == 0)
	resolve_local_init();

    /*
     * Strip one trailing dot.
     */
    len = strlen(saved_addr);
    if (saved_addr[len - 1] == '.')
	saved_addr[--len] = 0;

    /*
     * Compare the destination against the list of destinations that we
     * consider local.
     */
    if (string_list_match(resolve_local_list, saved_addr))
	RETURN(1);

    /*
     * Compare the destination against the list of interface addresses that
     * we are supposed to listen on.
     */
    dest = saved_addr;
    if (*dest == '[' && dest[len - 1] == ']') {
	dest++;
	dest[len -= 2] = 0;
#ifdef INET6
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(dest, NULL, &hints, &res0);
	if (!error) {
	    for (res = res0; res; res = res->ai_next) {
		if (own_inet_addr(res->ai_addr)) {
		    freeaddrinfo(res0);
		    RETURN(1);
		}
	    }
	    freeaddrinfo(res0);
	}
#else
	if ((ipaddr.s_addr = inet_addr(dest)) != INADDR_NONE
	    && own_inet_addr(&ipaddr))
	    RETURN(1);
#endif
    }

    /*
     * Must be remote, or a syntax error.
     */
    RETURN(0);
}

#ifdef TEST

#include <vstream.h>
#include <mail_conf.h>

int     main(int argc, char **argv)
{
    if (argc != 2)
	msg_fatal("usage: %s domain", argv[0]);
    mail_conf_read();
    vstream_printf("%s\n", resolve_local(argv[1]) ? "yes" : "no");
    vstream_fflush(VSTREAM_OUT);
}

#endif
