/*	$NetBSD: resolve_local.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	resolve_local 3
/* SUMMARY
/*	determine if domain resolves to local mail system
/* SYNOPSIS
/*	#include <resolve_local.h>
/*
/*	void	resolve_local_init()
/*
/*	int	resolve_local(domain)
/*	const char *domain;
/* DESCRIPTION
/*	resolve_local() determines if the named domain resolves to the
/*	local mail system, either by case-insensitive exact match
/*	against the domains, files or tables listed in $mydestination,
/*	or by a match of an [address-literal] against of the network
/*	addresses listed in $inet_interfaces or in $proxy_interfaces.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <string_list.h>
#include <myaddrinfo.h>
#include <valid_mailhost_addr.h>

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
    resolve_local_list = string_list_init(MATCH_FLAG_NONE, var_mydest);
}

/* resolve_local - match domain against list of local destinations */

int     resolve_local(const char *addr)
{
    char   *saved_addr = mystrdup(addr);
    char   *dest;
    const char *bare_dest;
    struct addrinfo *res0 = 0;
    ssize_t len;

    /*
     * The optimizer will eliminate tests that always fail.
     */
#define RETURN(x) \
    do { \
	myfree(saved_addr); \
	if (res0) \
	    freeaddrinfo(res0); \
	return(x); \
    } while (0)

    if (resolve_local_list == 0)
	resolve_local_init();

    /*
     * Strip one trailing dot but not dot-dot.
     * 
     * XXX This should not be distributed all over the code. Problem is,
     * addresses can enter the system via multiple paths: networks, local
     * forward/alias/include files, even as the result of address rewriting.
     */
    len = strlen(saved_addr);
    if (len == 0)
	RETURN(0);
    if (saved_addr[len - 1] == '.')
	saved_addr[--len] = 0;
    if (len == 0 || saved_addr[len - 1] == '.')
	RETURN(0);

    /*
     * Compare the destination against the list of destinations that we
     * consider local.
     */
    if (string_list_match(resolve_local_list, saved_addr))
	RETURN(1);

    /*
     * Compare the destination against the list of interface addresses that
     * we are supposed to listen on.
     * 
     * The destination may be an IPv6 address literal that was buried somewhere
     * inside a deeply recursively nested address. This information comes
     * from an untrusted source, and Wietse is not confident that everyone's
     * getaddrinfo() etc. implementation is sufficiently robust. The syntax
     * is complex enough with null field compression and with IPv4-in-IPv6
     * addresses that errors are likely.
     * 
     * The solution below is ad-hoc. We neutralize the string as soon as we
     * realize that its contents could be harmful. We neutralize the string
     * here, instead of neutralizing it in every resolve_local() caller.
     * That's because resolve_local knows how the address is going to be
     * parsed and converted into binary form.
     * 
     * There are several more structural solutions to this.
     * 
     * - One solution is to disallow address literals. This is not as bad as it
     * seems: I have never seen actual legitimate use of address literals.
     * 
     * - Another solution is to label each string with a trustworthiness label
     * and to expect that all Postfix infrastructure will exercise additional
     * caution when given a string with untrusted content. This is not likely
     * to happen.
     * 
     * FIX 200501 IPv6 patch did not require "IPv6:" prefix in numerical
     * addresses.
     */
    dest = saved_addr;
    if (*dest == '[' && dest[len - 1] == ']') {
	dest++;
	dest[len -= 2] = 0;
	if ((bare_dest = valid_mailhost_addr(dest, DO_GRIPE)) != 0
	    && hostaddr_to_sockaddr(bare_dest, (char *) 0, 0, &res0) == 0) {
	    if (own_inet_addr(res0->ai_addr) || proxy_inet_addr(res0->ai_addr))
		RETURN(1);
	}
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
    return (0);
}

#endif
