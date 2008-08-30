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
/*	or as a numerical address. An empty host expands as the
/*	wild-card address.  Address results are appended to
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
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include <mymalloc.h>
#include <inet_addr_list.h>
#include <inet_addr_host.h>
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_proto.h>
#include <msg.h>

/* inet_addr_host - look up address list for host */

int     inet_addr_host(INET_ADDR_LIST *addr_list, const char *hostname)
{
    const char *myname = "inet_addr_host";
    int     sock;
    struct addrinfo *res0;
    struct addrinfo *res;
    int     aierr;
    ssize_t hostnamelen;
    const char *hname;
    const char *serv;
    int     initial_count = addr_list->used;
    INET_PROTO_INFO *proto_info;

    /*
     * The use of square brackets around an IPv6 addresses is required, even
     * though we don't enforce it as it'd make the code unnecessarily
     * complicated.
     * 
     * XXX AIX 5.1 getaddrinfo() does not allow "0" as service, regardless of
     * whether or not a host is specified.
     */
    if (*hostname == 0) {
	hname = 0;
	serv = "1";
    } else if (*hostname == '['
	       && hostname[(hostnamelen = strlen(hostname)) - 1] == ']') {
	hname = mystrndup(hostname + 1, hostnamelen - 2);
	serv = 0;
    } else {
	hname = hostname;
	serv = 0;
    }

    proto_info = inet_proto_info();
    if ((aierr = hostname_to_sockaddr(hname, serv, SOCK_STREAM, &res0)) == 0) {
	for (res = res0; res; res = res->ai_next) {

	    /*
	     * Safety net.
	     */
	    if (strchr((char *) proto_info->sa_family_list, res->ai_family) == 0) {
		msg_info("%s: skipping address family %d for host \"%s\"",
			 myname, res->ai_family, hostname);
		continue;
	    }

	    /*
	     * On Linux systems it is not unusual for user-land to be out of
	     * sync with kernel-land. When this is the case we try to be
	     * helpful and filter out address families that the library
	     * claims to understand but that are not supported by the kernel.
	     */
	    if ((sock = socket(res->ai_family, SOCK_STREAM, 0)) < 0) {
		msg_warn("%s: skipping address family %d: %m",
			 myname, res->ai_family);
		continue;
	    }
	    if (close(sock))
		msg_warn("%s: close socket: %m", myname);

	    inet_addr_list_append(addr_list, res->ai_addr);
	}
	freeaddrinfo(res0);
    }
    if (hname && hname != hostname)
	myfree((char *) hname);

    return (addr_list->used - initial_count);
}

#ifdef TEST

#include <msg.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <sock_addr.h>

int     main(int argc, char **argv)
{
    INET_ADDR_LIST list;
    struct sockaddr_storage *sa;
    MAI_HOSTADDR_STR hostaddr;
    INET_PROTO_INFO *proto_info;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    if (argc < 3)
	msg_fatal("usage: %s protocols hostname...", argv[0]);

    proto_info = inet_proto_init(argv[0], argv[1]);
    argv += 1;

    while (--argc && *++argv) {
	inet_addr_list_init(&list);
	if (inet_addr_host(&list, *argv) == 0)
	    msg_fatal("not found: %s", *argv);

	for (sa = list.addrs; sa < list.addrs + list.used; sa++) {
	    SOCKADDR_TO_HOSTADDR(SOCK_ADDR_PTR(sa), SOCK_ADDR_LEN(sa),
				 &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	    vstream_printf("%s\t%s\n", *argv, hostaddr.buf);
	}
	vstream_fflush(VSTREAM_OUT);
	inet_addr_list_free(&list);
    }
    return (0);
}

#endif
