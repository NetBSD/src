/*++
/* NAME
/*	own_inet_addr 3
/* SUMMARY
/*	determine if IP address belongs to this mail system instance
/* SYNOPSIS
/*	#include <own_inet_addr.h>
/*
/*	int	own_inet_addr(addr)
/*	struct in_addr *addr;
/*
/*	INET_ADDR_LIST *own_inet_addr_list()
/*
/*	INET_ADDR_LIST *own_inet_mask_list()
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
#ifdef INET6
#include <sys/socket.h>
#include <netdb.h>
#endif

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <inet_addr_local.h>
#include <inet_addr_host.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>

/* Application-specific. */

static INET_ADDR_LIST addr_list;
static INET_ADDR_LIST mask_list;

/* own_inet_addr_init - initialize my own address list */

static void own_inet_addr_init(INET_ADDR_LIST *addr_list,
			               INET_ADDR_LIST *mask_list)
{
    INET_ADDR_LIST local_addrs;
    INET_ADDR_LIST local_masks;
    char   *hosts;
    char   *host;
    char   *sep = " \t,";
    char   *bufp;
    int     nvirtual;
    int     nlocal;

    inet_addr_list_init(addr_list);
    inet_addr_list_init(mask_list);

    /*
     * If we are listening on all interfaces (default), ask the system what
     * the interfaces are.
     */
    if (strcasecmp(var_inet_interfaces, DEF_INET_INTERFACES) == 0) {
	if (inet_addr_local(addr_list, mask_list) == 0)
	    msg_fatal("could not find any active network interfaces");
#if 0
	if (addr_list->used == 1)
	    msg_warn("found only one active network interface: %s",
		     inet_ntoa(addr_list->addrs[0]));
#endif
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

	inet_addr_list_init(&local_addrs);
	inet_addr_list_init(&local_masks);
	if (inet_addr_local(&local_addrs, &local_masks) == 0)
	    msg_fatal("could not find any active network interfaces");
	for (nvirtual = 0; nvirtual < addr_list->used; nvirtual++) {
	    for (nlocal = 0; /* see below */ ; nlocal++) {
		if (nlocal >= local_addrs.used) {
#ifdef INET6
		    char hbuf[NI_MAXHOST];
		    if (getnameinfo((struct sockaddr *)&addr_list->addrs[nvirtual],
		        addr_list->addrs[nvirtual].ss_len, hbuf,
		        sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0)
			strncpy(hbuf, "???", sizeof(hbuf));
		    msg_fatal("parameter %s: no local interface found for %s",
			      VAR_INET_INTERFACES, hbuf);
#else
		    msg_fatal("parameter %s: no local interface found for %s",
			      VAR_INET_INTERFACES,
			      inet_ntoa(addr_list->addrs[nvirtual]));
#endif
		}
#ifdef INET6
		if (addr_list->addrs[nvirtual].ss_family == 
		    local_addrs.addrs[nlocal].ss_family &&
		    addr_list->addrs[nvirtual].ss_len == 
		    local_addrs.addrs[nlocal].ss_len &&
		    memcmp(&addr_list->addrs[nvirtual],
		    &local_addrs.addrs[nlocal],
		    local_addrs.addrs[nlocal].ss_len) == 0) {
		    inet_addr_list_append(mask_list, (struct sockaddr *)&local_masks.addrs[nlocal]);
		    break;
		}
#else
		if (addr_list->addrs[nvirtual].s_addr
		    == local_addrs.addrs[nlocal].s_addr) {
		    inet_addr_list_append(mask_list, &local_masks.addrs[nlocal]);
		    break;
		}
#endif
	    }
	}
	inet_addr_list_free(&local_addrs);
	inet_addr_list_free(&local_masks);
    }
}

/* own_inet_addr - is this my own internet address */

#ifdef INET6
int     own_inet_addr(struct sockaddr * addr)
{
    int     i;
    char *p, *q;
    int l;
    struct sockaddr *sa;

    if (addr_list.used == 0)
	own_inet_addr_init(&addr_list, &mask_list);

    for (i = 0; i < addr_list.used; i++) {
	sa = (struct sockaddr *)&addr_list.addrs[i];
	if (addr->sa_family != sa->sa_family)
	    continue;
	switch (addr->sa_family) {
	case AF_INET:
	    p = (char *)&((struct sockaddr_in *)addr)->sin_addr;
	    q = (char *)&((struct sockaddr_in *)&addr_list.addrs[i])->sin_addr;
	    l = sizeof(struct in_addr);
	    break;
	case AF_INET6:
	    /* XXX scope */
	    p = (char *)&((struct sockaddr_in6 *)addr)->sin6_addr;
	    q = (char *)&((struct sockaddr_in6 *)&addr_list.addrs[i])->sin6_addr;
	    l = sizeof(struct in6_addr);
	    break;
	default:
	    continue;
	}
	if (memcmp(p, q, l) == 0)
	    return (1);
    }
    return (0);
}
#else
int     own_inet_addr(struct in_addr * addr)
{
    int     i;

    if (addr_list.used == 0)
	own_inet_addr_init(&addr_list, &mask_list);

    for (i = 0; i < addr_list.used; i++)
	if (addr->s_addr == addr_list.addrs[i].s_addr)
	    return (1);
}
#endif

/* own_inet_addr_list - return list of addresses */

INET_ADDR_LIST *own_inet_addr_list(void)
{
    if (addr_list.used == 0)
	own_inet_addr_init(&addr_list, &mask_list);

    return (&addr_list);
}

/* own_inet_mask_list - return list of addresses */

INET_ADDR_LIST *own_inet_mask_list(void)
{
    if (addr_list.used == 0)
	own_inet_addr_init(&addr_list, &mask_list);

    return (&mask_list);
}
