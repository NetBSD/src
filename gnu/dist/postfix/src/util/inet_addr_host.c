/*++
/* NAME
/*	inet_addr_host 3
/* SUMMARY
/*	determine all host internet interface addresses
/* SYNOPSIS
/*	#include <inet_addr_host.h>
/*
/*	int	inet_addr_host(addr_list, hostname)
/*	INET_ADDR_LIST *addr_list;
/*	const char *hostname;
/* DESCRIPTION
/*	inet_addr_host() determines all interface addresses of the
/*	named host. The host may be specified as a symbolic name,
/*	or as a numerical address. Address results are appended to
/*	the specified address list. The result value is the number
/*	of addresses appended to the list.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/* BUGS
/*	This code uses the name service, so it talks to the network,
/*	and that may not be desirable.
/* SEE ALSO
/*	inet_addr_list(3) address list management
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
#include <netdb.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <inet_addr_list.h>
#include <inet_addr_host.h>

/* inet_addr_host - look up address list for host */

int     inet_addr_host(INET_ADDR_LIST *addr_list, const char *hostname)
{
    struct hostent *hp;
    struct in_addr addr;
    int     initial_count = addr_list->used;

    if ((addr.s_addr = inet_addr(hostname)) != INADDR_NONE) {
	inet_addr_list_append(addr_list, &addr);
    } else {
	if ((hp = gethostbyname(hostname)) != 0)
	    while (hp->h_addr_list[0])
		inet_addr_list_append(addr_list,
				    (struct in_addr *) * hp->h_addr_list++);
    }
    return (addr_list->used - initial_count);
}

#ifdef TEST

#include <msg.h>
#include <vstream.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    INET_ADDR_LIST addr_list;
    int     i;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    if (argc < 2)
	msg_fatal("usage: %s hostname...", argv[0]);

    while (--argc && *++argv) {
	inet_addr_list_init(&addr_list);
	if (inet_addr_host(&addr_list, *argv) == 0)
	    msg_fatal("not found: %s", *argv);

	for (i = 0; i < addr_list.used; i++)
	    vstream_printf("%s\n", inet_ntoa(addr_list.addrs[i]));
	vstream_fflush(VSTREAM_OUT);
    }
    inet_addr_list_free(&addr_list);
}

#endif
