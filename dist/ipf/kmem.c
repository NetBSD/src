/*	$NetBSD: kmem.c,v 1.2.4.1 2002/02/09 16:55:04 he Exp $	*/

/*
 * Copyright (C) 1993-2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
/*
 * kmemcpy() - copies n bytes from kernel memory into user buffer.
 * returns 0 on success, -1 on error.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <kvm.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <net/if.h>
#if __FreeBSD_version >= 300000
# include <net/if_var.h>
#endif

#include "kmem.h"

#ifndef __STDC__
# define	const
#endif

#if !defined(lint)
static const char sccsid[] = "@(#)kmem.c	1.4 1/12/96 (C) 1992 Darren Reed";
static const char rcsid[] = "@(#)Id: kmem.c,v 2.2.2.8 2002/01/15 14:36:53 darrenr Exp";
#endif

static	kvm_t	*kvm_f = NULL;

int	openkmem(kern, core)
char	*kern, *core;
{
	kvm_f = kvm_open(kern, core, NULL, O_RDONLY, NULL);
	if (kvm_f == NULL)
	    {
		perror("openkmem:open");
		return -1;
	    }
	return 0;
}

int	kmemcpy(buf, pos, n)
register char	*buf;
long	pos;
register int	n;
{
	register int	r;

	if (!n)
		return 0;

	if (kvm_f == NULL)
		if (openkmem(NULL, NULL) == -1)
			return -1;

	while ((r = kvm_read(kvm_f, pos, buf, n)) < n)
		if (r <= 0)
		    {
			fprintf(stderr, "pos=0x%x ", (u_int)pos);
			perror("kmemcpy:read");
			return -1;
		    }
		else
		    {
			buf += r;
			pos += r;
			n -= r;
		    }
	return 0;
}

int	kstrncpy(buf, pos, n)
register char	*buf;
long	pos;
register int	n;
{
	register int	r;

	if (!n)
		return 0;

	if (kvm_f == NULL)
		if (openkmem(NULL, NULL) == -1)
			return -1;

	while (n > 0)
	    {
		r = kvm_read(kvm_f, pos, buf, 1);
		if (r <= 0)
		    {
			fprintf(stderr, "pos=0x%x ", (u_int)pos);
			perror("kmemcpy:read");
			return -1;
		    }
		else
		    {
			if (*buf == '\0')
				break;
			buf++;
			pos++;
			n--;
		    }
	    }
	return 0;
}


/*
 * Given a pointer to an interface in the kernel, return a pointer to a
 * string which is the interface name.
 */
char *getifname(ptr)
void *ptr;
{
#if SOLARIS
	char *ifname;
	ill_t ill;

	if (ptr == (void *)-1)
		return "!";
	if (ptr == NULL)
		return "-";

	if (kmemcpy((char *)&ill, (u_long)ptr, sizeof(ill)) == -1)
		return "X";
	ifname = malloc(ill.ill_name_length + 1);
	if (kmemcpy(ifname, (u_long)ill.ill_name,
		    ill.ill_name_length) == -1)
		return "X";
	return ifname;
#else
# if defined(NetBSD) && (NetBSD >= 199905) && (NetBSD < 1991011) || \
    defined(__OpenBSD__)
#else
	char buf[32];
	int len;
# endif
	struct ifnet netif;

	if (ptr == (void *)-1)
		return "!";
	if (ptr == NULL)
		return "-";

	if (kmemcpy((char *)&netif, (u_long)ptr, sizeof(netif)) == -1)
		return "X";
# if defined(NetBSD) && (NetBSD >= 199905) && (NetBSD < 1991011) || \
    defined(__OpenBSD__)
	return strdup(netif.if_xname);
# else
	if (kstrncpy(buf, (u_long)netif.if_name, sizeof(buf)) == -1)
		return "X";
	if (netif.if_unit < 10)
		len = 2;
	else if (netif.if_unit < 1000)
		len = 3;
	else if (netif.if_unit < 10000)
		len = 4;
	else
		len = 5;
	buf[sizeof(buf) - len] = '\0';
	sprintf(buf + strlen(buf), "%d", netif.if_unit % 10000);
	return strdup(buf);
# endif
#endif
}
