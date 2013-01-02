/*	$NetBSD: own_inet_addr.c,v 1.1.1.2 2013/01/02 18:58:59 tron Exp $	*/

/*++
/* NAME
/*	own_inet_addr 3
/* SUMMARY
/*	determine if IP address belongs to this mail system instance
/* SYNOPSIS
/*	#include <own_inet_addr.h>
/*
/*	int	own_inet_addr(addr)
/*	struct sockaddr *addr;
/*
/*	INET_ADDR_LIST *own_inet_addr_list()
/*
/*	INET_ADDR_LIST *own_inet_mask_list()
/*
/*	int	proxy_inet_addr(addr)
/*	struct in_addr *addr;
/*
/*	INET_ADDR_LIST *proxy_inet_addr_list()
/* DESCRIPTION
/*	own_inet_addr() determines if the specified IP address belongs
/*	to this mail system instance, i.e. if this mail system instance
/*	is supposed to be listening on this specific IP address.
/*
/*	own_inet_addr_list() returns the list of all addresses that
/*	belong to this mail system instance.
/*
/*	own_inet_mask_list() returns the list of all corresponding
/*	netmasks.
/*
/*	proxy_inet_addr() determines if the specified IP address is
/*	listed with the proxy_interfaces configuration parameter.
/*
/*	proxy_inet_addr_list() returns the list of all addresses that
/*	belong to proxy network interfaces.
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <inet_addr_local.h>
#include <inet_addr_host.h>
#include <stringops.h>
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_proto.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>

/* Application-specific. */

static INET_ADDR_LIST saved_addr_list;
static INET_ADDR_LIST saved_mask_list;
static INET_ADDR_LIST saved_proxy_list;

/* own_inet_addr_init - initialize my own address list */

static void own_inet_addr_init(INET_ADDR_LIST *addr_list,
			               INET_ADDR_LIST *mask_list)
{
    INET_ADDR_LIST local_addrs;
    INET_ADDR_LIST local_masks;
    char   *hosts;
    char   *host;
    const char *sep = " \t,";
    char   *bufp;
    int     nvirtual;
    int     nlocal;
    MAI_HOSTADDR_STR hostaddr;
    struct sockaddr_storage *sa;
    struct sockaddr_storage *ma;

    inet_addr_list_init(addr_list);
    inet_addr_list_init(mask_list);

    /*
     * Avoid run-time errors when all network protocols are disabled. We
     * can't look up interface information, and we can't convert explicit
     * names or addresses.
     */
    if (inet_proto_info()->ai_family_list[0] == 0) {
	if (msg_verbose)
	    msg_info("skipping %s setting - "
		     "all network protocols are disabled",
		     VAR_INET_INTERFACES);
	return;
    }

    /*
     * If we are listening on all interfaces (default), ask the system what
     * the interfaces are.
     */
    if (strcmp(var_inet_interfaces, INET_INTERFACES_ALL) == 0) {
	if (inet_addr_local(addr_list, mask_list,
			    inet_proto_info()->ai_family_list) == 0)
	    msg_fatal("could not find any active network interfaces");
    }

    /*
     * Select all loopback interfaces from the system's available interface
     * list.
     */
    else if (strcmp(var_inet_interfaces, INET_INTERFACES_LOCAL) == 0) {
	inet_addr_list_init(&local_addrs);
	inet_addr_list_init(&local_masks);
	if (inet_addr_local(&local_addrs, &local_masks,
			    inet_proto_info()->ai_family_list) == 0)
	    msg_fatal("could not find any active network interfaces");
	for (sa = local_addrs.addrs, ma = local_masks.addrs;
	     sa < local_addrs.addrs + local_addrs.used; sa++, ma++) {
	    if (sock_addr_in_loopback(SOCK_ADDR_PTR(sa))) {
		inet_addr_list_append(addr_list, SOCK_ADDR_PTR(sa));
		inet_addr_list_append(mask_list, SOCK_ADDR_PTR(ma));
	    }
	}
	inet_addr_list_free(&local_addrs);
	inet_addr_list_free(&local_masks);
    }

    /*
     * If we are supposed to be listening only on specific interface
     * addresses (virtual hosting), look up the addresses of those
     * interfaces.
     */
    else {
	bufp = hosts = mystrdup(var_inet_interfaces);
	while ((host = mystrtok(&bufp, sep)) != 0)
	    if (inet_addr_host(addr_list, host) == 0)
		msg_fatal("config variable %s: host not found: %s",
			  VAR_INET_INTERFACES, host);
	myfree(hosts);

	/*
	 * Weed out duplicate IP addresses. Duplicates happen when the same
	 * IP address is listed under multiple hostnames. If we don't weed
	 * out duplicates, Postfix can suddenly stop working after the DNS is
	 * changed.
	 */
	inet_addr_list_uniq(addr_list);

	/*
	 * Find out the netmask for each virtual interface, by looking it up
	 * among all the local interfaces.
	 */
	inet_addr_list_init(&local_addrs);
	inet_addr_list_init(&local_masks);
	if (inet_addr_local(&local_addrs, &local_masks,
			    inet_proto_info()->ai_family_list) == 0)
	    msg_fatal("could not find any active network interfaces");
	for (nvirtual = 0; nvirtual < addr_list->used; nvirtual++) {
	    for (nlocal = 0; /* see below */ ; nlocal++) {
		if (nlocal >= local_addrs.used) {
		    SOCKADDR_TO_HOSTADDR(
				 SOCK_ADDR_PTR(addr_list->addrs + nvirtual),
				 SOCK_ADDR_LEN(addr_list->addrs + nvirtual),
				      &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
		    msg_fatal("parameter %s: no local interface found for %s",
			      VAR_INET_INTERFACES, hostaddr.buf);
		}
		if (SOCK_ADDR_EQ_ADDR(addr_list->addrs + nvirtual,
				      local_addrs.addrs + nlocal)) {
		    inet_addr_list_append(mask_list,
				 SOCK_ADDR_PTR(local_masks.addrs + nlocal));
		    break;
		}
	    }
	}
	inet_addr_list_free(&local_addrs);
	inet_addr_list_free(&local_masks);
    }
}

/* own_inet_addr - is this my own internet address */

int     own_inet_addr(struct sockaddr * addr)
{
    int     i;

    if (saved_addr_list.used == 0)
	own_inet_addr_init(&saved_addr_list, &saved_mask_list);

    for (i = 0; i < saved_addr_list.used; i++)
	if (SOCK_ADDR_EQ_ADDR(addr, saved_addr_list.addrs + i))
	    return (1);
    return (0);
}

/* own_inet_addr_list - return list of addresses */

INET_ADDR_LIST *own_inet_addr_list(void)
{
    if (saved_addr_list.used == 0)
	own_inet_addr_init(&saved_addr_list, &saved_mask_list);

    return (&saved_addr_list);
}

/* own_inet_mask_list - return list of addresses */

INET_ADDR_LIST *own_inet_mask_list(void)
{
    if (saved_addr_list.used == 0)
	own_inet_addr_init(&saved_addr_list, &saved_mask_list);

    return (&saved_mask_list);
}

/* proxy_inet_addr_init - initialize my proxy interface list */

static void proxy_inet_addr_init(INET_ADDR_LIST *addr_list)
{
    char   *hosts;
    char   *host;
    const char *sep = " \t,";
    char   *bufp;

    /*
     * Parse the proxy_interfaces parameter, and expand any symbolic
     * hostnames into IP addresses.
     */
    inet_addr_list_init(addr_list);
    bufp = hosts = mystrdup(var_proxy_interfaces);
    while ((host = mystrtok(&bufp, sep)) != 0)
	if (inet_addr_host(addr_list, host) == 0)
	    msg_fatal("config variable %s: host not found: %s",
		      VAR_PROXY_INTERFACES, host);
    myfree(hosts);

    /*
     * Weed out duplicate IP addresses.
     */
    inet_addr_list_uniq(addr_list);
}

/* proxy_inet_addr - is this my proxy internet address */

int     proxy_inet_addr(struct sockaddr * addr)
{
    int     i;

    if (*var_proxy_interfaces == 0)
	return (0);

    if (saved_proxy_list.used == 0)
	proxy_inet_addr_init(&saved_proxy_list);

    for (i = 0; i < saved_proxy_list.used; i++)
	if (SOCK_ADDR_EQ_ADDR(addr, saved_proxy_list.addrs + i))
	    return (1);
    return (0);
}

/* proxy_inet_addr_list - return list of addresses */

INET_ADDR_LIST *proxy_inet_addr_list(void)
{
    if (*var_proxy_interfaces != 0 && saved_proxy_list.used == 0)
	proxy_inet_addr_init(&saved_proxy_list);

    return (&saved_proxy_list);
}

#ifdef TEST
#include <inet_proto.h>

static void inet_addr_list_print(INET_ADDR_LIST *list)
{
    MAI_HOSTADDR_STR hostaddr;
    struct sockaddr_storage *sa;

    for (sa = list->addrs; sa < list->addrs + list->used; sa++) {
	SOCKADDR_TO_HOSTADDR(SOCK_ADDR_PTR(sa), SOCK_ADDR_LEN(sa),
			     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	msg_info("%s", hostaddr.buf);
    }
}

char   *var_inet_interfaces;

int     main(int argc, char **argv)
{
    INET_PROTO_INFO *proto_info;
    INET_ADDR_LIST *list;

    if (argc != 3)
	msg_fatal("usage: %s protocols interface_list (e.g. \"all all\")",
		  argv[0]);
    msg_verbose = 10;
    proto_info = inet_proto_init(argv[0], argv[1]);
    var_inet_interfaces = argv[2];
    list = own_inet_addr_list();
    inet_addr_list_print(list);
    return (0);
}

#endif
