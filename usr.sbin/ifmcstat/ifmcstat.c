/*	$NetBSD: ifmcstat.c,v 1.13 2014/05/30 01:44:21 rmind Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
# include <net/if_var.h>
#endif
#include <net/if_types.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#ifndef __NetBSD__
# ifdef	__FreeBSD__
#  define	KERNEL
# endif
# include <netinet/if_ether.h>
# ifdef	__FreeBSD__
#  undef	KERNEL
# endif
#else
# include <net/if_ether.h>
#endif
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include <netdb.h>

kvm_t	*kvmd;

struct	nlist nl[] = {
#define	N_IFNET_LIST	0
	{ "_ifnet_list", 0, 0, 0, 0 },
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
#define N_IN6_MK 1
	{ "_in6_mk", 0, 0, 0, 0 },
#endif
	{ "", 0, 0, 0, 0 },
};

const char *inet6_n2a __P((struct in6_addr *));
int main __P((void));
char *ifname __P((struct ifnet *));
void kread __P((u_long, void *, int));
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
void acmc __P((struct ether_multi *));
#endif
void if6_addrlist __P((struct ifaddr *));
void in6_multilist __P((struct in6_multi *));
struct in6_multi * in6_multientry __P((struct in6_multi *));

#if !defined(__NetBSD__) && !(defined(__FreeBSD__) && __FreeBSD__ >= 3) && !defined(__OpenBSD__)
#ifdef __bsdi__
struct ether_addr {
	u_int8_t ether_addr_octet[6];
};
#endif
static char *ether_ntoa __P((struct ether_addr *));
#endif

#define	KREAD(addr, buf, type) \
	kread((u_long)addr, (void *)buf, sizeof(type))

#ifdef N_IN6_MK
struct multi6_kludge {
	LIST_ENTRY(multi6_kludge) mk_entry;
	struct ifnet *mk_ifp;
	struct in6_multihead mk_head;
};
#endif

const char *inet6_n2a(p)
	struct in6_addr *p;
{
	static char buf[NI_MAXHOST];
	struct sockaddr_in6 sin6;
	const int niflags = NI_NUMERICHOST;

	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_addr = *p;
	inet6_getscopeid(&sin6, INET6_IS_ADDR_LINKLOCAL|
	    INET6_IS_ADDR_MC_LINKLOCAL);
	if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
			buf, sizeof(buf), NULL, 0, niflags) == 0)
		return buf;
	else
		return "(invalid)";
}

int main()
{
	char	buf[_POSIX2_LINE_MAX], ifnam[IFNAMSIZ];
	struct	ifnet	*ifp, *nifp, ifnet;
#ifndef __NetBSD__
	struct	arpcom	arpcom;
#else
	struct ethercom ec;
	union {
		struct sockaddr_storage st;
		struct sockaddr_dl sdl;
	} su;
	struct sockaddr_dl *sdlp;
	sdlp = &su.sdl;
#endif

	if ((kvmd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, buf)) == NULL) {
		perror("kvm_openfiles");
		exit(1);
	}
	if (kvm_nlist(kvmd, nl) < 0) {
		perror("kvm_nlist");
		exit(1);
	}
	if (nl[N_IFNET_LIST].n_value == 0) {
		printf("symbol %s not found\n", nl[N_IFNET_LIST].n_name);
		exit(1);
	}
	KREAD(nl[N_IFNET_LIST].n_value, &ifp, struct ifnet *);
	while (ifp) {
		KREAD(ifp, &ifnet, struct ifnet);
		printf("%s:\n", if_indextoname(ifnet.if_index, ifnam));

#if defined(__NetBSD__) || defined(__OpenBSD__)
		if6_addrlist(ifnet.if_addrlist.tqh_first);
		nifp = ifnet.if_list.tqe_next;
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		if6_addrlist(TAILQ_FIRST(&ifnet.if_addrhead));
		nifp = ifnet.if_link.tqe_next;
#else
		if6_addrlist(ifnet.if_addrlist);
		nifp = ifnet.if_next;
#endif

#ifdef __NetBSD__
		KREAD(ifnet.if_sadl, sdlp, struct sockaddr_dl);
		if (sdlp->sdl_type == IFT_ETHER) {
			/* If we didn't get all of it, try again */
			if (sdlp->sdl_len > sizeof(struct sockaddr_dl))
				kread((u_long)ifnet.if_sadl, (void *)sdlp, sdlp->sdl_len);
			printf("\tenaddr %s",
			       ether_ntoa((struct ether_addr *)LLADDR(sdlp)));
			KREAD(ifp, &ec, struct ethercom);
			printf(" multicnt %d", ec.ec_multicnt);
			acmc(ec.ec_multiaddrs.lh_first);
			printf("\n");
		}
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		/* not supported */
#else
		if (ifnet.if_type == IFT_ETHER) {
			KREAD(ifp, &arpcom, struct arpcom);
			printf("\tenaddr %s",
			    ether_ntoa((struct ether_addr *)arpcom.ac_enaddr));
			KREAD(ifp, &arpcom, struct arpcom);
			printf(" multicnt %d", arpcom.ac_multicnt);
#ifdef __OpenBSD__
			acmc(arpcom.ac_multiaddrs.lh_first);
#else
			acmc(arpcom.ac_multiaddrs);
#endif
			printf("\n");
		}
#endif

		ifp = nifp;
	}

	exit(0);
	/*NOTREACHED*/
}

char *ifname(ifp)
	struct ifnet *ifp;
{
	static char buf[BUFSIZ];
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct ifnet ifnet;
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
	KREAD(ifp, &ifnet, struct ifnet);
	strncpy(buf, ifnet.if_xname, BUFSIZ);
#else
	KREAD(ifp->if_name, buf, IFNAMSIZ);
#endif
	return buf;
}

void kread(addr, buf, len)
	u_long addr;
	void *buf;
	int len;
{
	if (kvm_read(kvmd, addr, buf, len) != len) {
		perror("kvm_read");
		exit(1);
	}
}

#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
void acmc(am)
	struct ether_multi *am;
{
	struct ether_multi em;

	while (am) {
		KREAD(am, &em, struct ether_multi);
		
		printf("\n\t\t");
		printf("%s -- ", ether_ntoa((struct ether_addr *)em.enm_addrlo));
		printf("%s ", ether_ntoa((struct ether_addr *)&em.enm_addrhi));
		printf("%d", em.enm_refcount);
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
		am = em.enm_next;
#else
		am = em.enm_list.le_next;
#endif
	}
}
#endif

void
if6_addrlist(ifap)
	struct ifaddr *ifap;
{
	struct ifaddr ifa;
	struct sockaddr sa;
	struct in6_ifaddr if6a;
	struct in6_multi *mc = 0;
	struct ifaddr *ifap0;

	ifap0 = ifap;
	while (ifap) {
		KREAD(ifap, &ifa, struct ifaddr);
		if (ifa.ifa_addr == NULL)
			goto nextifap;
		KREAD(ifa.ifa_addr, &sa, struct sockaddr);
		if (sa.sa_family != PF_INET6)
			goto nextifap;
		KREAD(ifap, &if6a, struct in6_ifaddr);
		printf("\tinet6 %s\n", inet6_n2a(&if6a.ia_addr.sin6_addr));
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
		mc = mc ? mc : if6a.ia6_multiaddrs.lh_first;
#endif
	nextifap:
#if defined(__NetBSD__) || defined(__OpenBSD__)
		ifap = ifa.ifa_list.tqe_next;
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		ifap = ifa.ifa_link.tqe_next;
#else
		ifap = ifa.ifa_next;
#endif /* __FreeBSD__ >= 3 */
	}
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	if (ifap0) {
		struct ifnet ifnet;
		struct ifmultiaddr ifm, *ifmp = 0;
		struct sockaddr_in6 sin6;
		struct in6_multi in6m;
		struct sockaddr_dl sdl;
		int in6_multilist_done = 0;

		KREAD(ifap0, &ifa, struct ifaddr);
		KREAD(ifa.ifa_ifp, &ifnet, struct ifnet);
		if (ifnet.if_multiaddrs.lh_first)
			ifmp = ifnet.if_multiaddrs.lh_first;
		while (ifmp) {
			KREAD(ifmp, &ifm, struct ifmultiaddr);
			if (ifm.ifma_addr == NULL)
				goto nextmulti;
			KREAD(ifm.ifma_addr, &sa, struct sockaddr);
			if (sa.sa_family != AF_INET6)
				goto nextmulti;
			(void)in6_multientry((struct in6_multi *)
					     ifm.ifma_protospec);
			if (ifm.ifma_lladdr == 0)
				goto nextmulti;
			KREAD(ifm.ifma_lladdr, &sdl, struct sockaddr_dl);
			printf("\t\t\tmcast-macaddr %s multicnt %d\n",
			       ether_ntoa((struct ether_addr *)LLADDR(&sdl)),
			       ifm.ifma_refcount);
		    nextmulti:
			ifmp = ifm.ifma_link.le_next;
		}
	}
#else
	if (mc)
		in6_multilist(mc);
#endif
#ifdef N_IN6_MK
	if (nl[N_IN6_MK].n_value != 0) {
		LIST_HEAD(in6_mktype, multi6_kludge) in6_mk;
		struct multi6_kludge *mkp, mk;
		char *nam;

		KREAD(nl[N_IN6_MK].n_value, &in6_mk, struct in6_mktype);
		KREAD(ifap0, &ifa, struct ifaddr);

		nam = strdup(ifname(ifa.ifa_ifp));

		for (mkp = in6_mk.lh_first; mkp; mkp = mk.mk_entry.le_next) {
			KREAD(mkp, &mk, struct multi6_kludge);
			if (strcmp(nam, ifname(mk.mk_ifp)) == 0 &&
			    mk.mk_head.lh_first) {
				printf("\t(on kludge entry for %s)\n", nam);
				in6_multilist(mk.mk_head.lh_first);
			}
		}

		free(nam);
	}
#endif
}

struct in6_multi *
in6_multientry(mc)
	struct in6_multi *mc;
{
	struct in6_multi multi;

	KREAD(mc, &multi, struct in6_multi);
	printf("\t\tgroup %s", inet6_n2a(&multi.in6m_addr));
	printf(" refcnt %u\n", multi.in6m_refcount);
	return(multi.in6m_entry.le_next);
}

void
in6_multilist(mc)
	struct in6_multi *mc;
{
	while (mc)
		mc = in6_multientry(mc);
}

#if !defined(__NetBSD__) && !(defined(__FreeBSD__) && __FreeBSD__ >= 3) && !defined(__OpenBSD__)
static char *
ether_ntoa(e)
	struct ether_addr *e;
{
	static char buf[20];
	u_char *p;

	p = (u_char *)e;

	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
		p[0], p[1], p[2], p[3], p[4], p[5]);
	return buf;
}
#endif
