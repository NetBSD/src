/*	$NetBSD: nfs_bootdhcp.c,v 1.15 2000/05/28 07:01:09 gmcgarry Exp $	*/

/*-
 * Copyright (c) 1995, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Support for NFS diskless booting with BOOTP (RFC951, RFC1048)
 *
 * History:
 *
 * Tor Egge developed the initial version of this code based on
 * the Sun RPC/bootparam sources nfs_boot.c and krpc_subr.c and
 * submitted that work to NetBSD as bugreport "kern/2351" on
 * 29 Apr 1996.
 *
 * Gordon Ross reorganized Tor's version into this form and
 * integrated it into the NetBSD sources during Aug 1997.
 */

#include "opt_nfs_boot.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_arp.h> 	/* ARPHRD_ETHER, etc. */
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <nfs/rpcv2.h>

#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>

/*
 * There are two implementations of NFS diskless boot.
 * This implementation uses BOOTP (RFC951, RFC1048), and
 * the other uses Sun RPC/bootparams (nfs_bootparam.c).
 *
 * This method gets everything it needs with one BOOTP
 * request and reply.  Note that this actually uses only
 * the old BOOTP functionality subset of DHCP.  It is not
 * clear that DHCP provides any advantage over BOOTP for
 * diskless boot.  DHCP allows the server to assign an IP
 * address without any a-priori knowledge of the client,
 * but we require that the server has a-priori knowledge
 * of the client so it can export our (unique) NFS root.
 * Given that the server needs a-priori knowledge about
 * the client anyway, it might as well assign a fixed IP
 * address for the client and support BOOTP.
 * 
 * On the other hand, disk-FULL clients may use DHCP, but
 * in that case the DHCP client should be user-mode code,
 * and has no bearing on the code below. -gwr
 */

/* Begin stuff from bootp.h */
/* Definitions from RFC951 */
#define BP_CHADDR_LEN	 16
#define BP_SNAME_LEN	 64
#define BP_FILE_LEN	128
#define BP_VEND_LEN	 64
struct bootp {
	u_int8_t	bp_op;		/* packet opcode type */
	u_int8_t	bp_htype;	/* hardware addr type */
	u_int8_t	bp_hlen;	/* hardware addr length */
	u_int8_t	bp_hops;	/* gateway hops */
	u_int32_t	bp_xid;		/* transaction ID */
	u_int16_t	bp_secs;	/* seconds since boot began */
	u_int16_t	bp_flags;	/* RFC1532 broadcast, etc. */
	struct in_addr	bp_ciaddr;	/* client IP address */
	struct in_addr	bp_yiaddr;	/* 'your' IP address */
	struct in_addr	bp_siaddr;	/* server IP address */
	struct in_addr	bp_giaddr;	/* gateway IP address */
	u_int8_t bp_chaddr[BP_CHADDR_LEN]; /* client hardware address */
	char	bp_sname[BP_SNAME_LEN]; /* server host name */
	char	bp_file[BP_FILE_LEN];	/* boot file name */
	u_int8_t bp_vend[BP_VEND_LEN];	/* RFC1048 options */
	/*
	 * Note that BOOTP packets are allowed to be longer
	 * (see RFC 1532 sect. 2.1) and common practice is to
	 * allow the option data in bp_vend to extend into the
	 * additional space provided in longer packets.
	 */
};

#define IPPORT_BOOTPS 67
#define IPPORT_BOOTPC 68

#define BOOTREQUEST		1
#define BOOTREPLY		2

/*
 * Is this available from the sockaddr_dl somehow?
 * Perhaps (struct arphdr)->ar_hrd = ARPHRD_ETHER?
 * The interface has ->if_type but not the ARP fmt.
 */
#define HTYPE_ETHERNET		1
#define HTYPE_IEEE802		6

/*
 * Vendor magic cookie (v_magic) for RFC1048
 */
static const u_int8_t vm_rfc1048[4] = { 99, 130, 83, 99 };

/*
 * Tag values used to specify what information is being supplied in
 * the vendor (options) data area of the packet.
 */
/* RFC 1048 */
#define TAG_END			((unsigned char) 255)
#define TAG_PAD			((unsigned char)   0)
#define TAG_SUBNET_MASK		((unsigned char)   1)
#define TAG_TIME_OFFSET		((unsigned char)   2)
#define TAG_GATEWAY		((unsigned char)   3)
#define TAG_TIME_SERVER		((unsigned char)   4)
#define TAG_NAME_SERVER		((unsigned char)   5)
#define TAG_DOMAIN_SERVER	((unsigned char)   6)
#define TAG_LOG_SERVER		((unsigned char)   7)
#define TAG_COOKIE_SERVER	((unsigned char)   8)
#define TAG_LPR_SERVER		((unsigned char)   9)
#define TAG_IMPRESS_SERVER	((unsigned char)  10)
#define TAG_RLP_SERVER		((unsigned char)  11)
#define TAG_HOST_NAME		((unsigned char)  12)
#define TAG_BOOT_SIZE		((unsigned char)  13)
/* RFC 1395 */
#define TAG_DUMP_FILE		((unsigned char)  14)
#define TAG_DOMAIN_NAME		((unsigned char)  15)
#define TAG_SWAP_SERVER		((unsigned char)  16)
#define TAG_ROOT_PATH		((unsigned char)  17)
/* End of stuff from bootp.h */

#ifdef NFS_BOOT_DHCP
#define TAG_REQ_ADDR		((unsigned char)  50)
#define TAG_LEASETIME		((unsigned char)  51)
#define TAG_OVERLOAD		((unsigned char)  52)
#define TAG_DHCP_MSGTYPE	((unsigned char)  53)
#define TAG_SERVERID		((unsigned char)  54)
#define TAG_PARAM_REQ		((unsigned char)  55)
#define TAG_MSG			((unsigned char)  56)
#define TAG_MAXSIZE		((unsigned char)  57)
#define TAG_T1			((unsigned char)  58)
#define TAG_T2			((unsigned char)  59)
#define TAG_CLASSID		((unsigned char)  60)
#define TAG_CLIENTID		((unsigned char)  61)
#endif

#ifdef NFS_BOOT_DHCP
#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPDECLINE 4
#define DHCPACK 5
#define DHCPNAK 6
#define DHCPRELEASE 7
#endif

#ifdef NFS_BOOT_DHCP
#define BOOTP_SIZE_MAX	(sizeof(struct bootp)+312-64)
#else
/*
 * The "extended" size is somewhat arbitrary, but is
 * constrained by the maximum message size specified
 * by RFC1533 (567 total).  This value increases the
 * space for options from 64 bytes to 256 bytes.
 */
#define BOOTP_SIZE_MAX	(sizeof(struct bootp)+256-64)
#endif
#define BOOTP_SIZE_MIN	(sizeof(struct bootp))

/* Convenience macro */
#define INTOHL(ina) ((u_int32_t)ntohl((ina).s_addr))

static int bootpc_call __P((struct nfs_diskless *, struct proc *));
static void bootp_extract __P((struct bootp *, int, struct nfs_diskless *));

/* #define DEBUG	XXX */

#ifdef	DEBUG
#define DPRINT(s) printf("nfs_boot: %s\n", s)
#else
#define DPRINT(s) (void)0
#endif


/*
 * Get our boot parameters using BOOTP.
 */
int
nfs_bootdhcp(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	struct ifnet *ifp = nd->nd_ifp;
	int error;

	/*
	 * Do enough of ifconfig(8) so that the chosen interface
	 * can talk to the servers.  Use address zero for now.
	 */
	error = nfs_boot_setaddress(ifp, procp, INADDR_ANY, INADDR_ANY,
				    INADDR_BROADCAST);
	if (error) {
		printf("nfs_boot: set ifaddr zero, error=%d\n", error);
		return (error);
	}

	/* This function call does the real send/recv work. */
	error = bootpc_call(nd, procp);

	/* Get rid of the temporary (zero) IP address. */
	(void) nfs_boot_deladdress(ifp, procp, INADDR_ANY);

	/* NOW we can test the error from bootpc_call. */
	if (error)
		goto out;

	/*
	 * Do ifconfig with our real IP address and mask.
	 */
	error = nfs_boot_setaddress(ifp, procp, nd->nd_myip.s_addr,
				    nd->nd_mask.s_addr, INADDR_ANY);
	if (error) {
		printf("nfs_boot: set ifaddr real, error=%d\n", error);
		goto out;
	}

out:
	if (error) {
		(void) nfs_boot_ifupdown(ifp, procp, 0);
		nfs_boot_flushrt(ifp);
	}
	return (error);
}

struct bootpcontext {
	int xid;
	u_char *haddr;
	u_char halen;
	struct bootp *replybuf;
	int replylen;
#ifdef NFS_BOOT_DHCP
	char expected_dhcpmsgtype, dhcp_ok;
	struct in_addr dhcp_serverip;
#endif
};

static int bootpset __P((struct mbuf*, void*, int));
static int bootpcheck __P((struct mbuf*, void*));

static int
bootpset(m, context, waited)
	struct mbuf *m;
	void *context;
	int waited;
{
	struct bootp *bootp;

	/* we know it's contigous (in 1 mbuf cluster) */
	bootp = mtod(m, struct bootp*);

	bootp->bp_secs = htons(waited);

	return (0);
}

static int
bootpcheck(m, context)
	struct mbuf *m;
	void *context;
{
	struct bootp *bootp;
	struct bootpcontext *bpc = context;
	u_int tag, len;
	u_char *p, *limit;

	/*
	 * Is this a valid reply?
	 */
	if (m->m_pkthdr.len < BOOTP_SIZE_MIN) {
		DPRINT("short packet");
		return (-1);
	}
	if (m->m_pkthdr.len > BOOTP_SIZE_MAX) {
		DPRINT("long packet");
		return (-1);
	}

	/*
	 * don't make first checks more expensive than necessary
	 */
#define ofs(what, elem) ((int)&(((what *)0)->elem))
	if (m->m_len < ofs(struct bootp, bp_secs)) {
		m = m_pullup(m, ofs(struct bootp, bp_secs));
		if (m == NULL)
			return (-1);
	}
#undef ofs
	bootp = mtod(m, struct bootp*);

	if (bootp->bp_op != BOOTREPLY) {
		DPRINT("not reply");
		return (-1);
	}
	if (bootp->bp_hlen != bpc->halen) {
		DPRINT("bad hwa_len");
		return (-1);
	}
	if (memcmp(bootp->bp_chaddr, bpc->haddr, bpc->halen)) {
		DPRINT("wrong hwaddr");
		return (-1);
	}
	if (bootp->bp_xid != bpc->xid) {
		DPRINT("wrong xid");
		return (-1);
	}

	/*
	 * OK, it's worth to look deeper.
	 * We copy the mbuf into a flat buffer here because
	 * m_pullup() is a bit limited for this purpose
	 * (doesn't allocate a cluster if necessary).
	 */
	bpc->replylen = m->m_pkthdr.len;
	m_copydata(m, 0, bpc->replylen, (caddr_t)bpc->replybuf);
	bootp = bpc->replybuf;

	/*
	 * Check if the IP address we get looks correct.
	 * (DHCP servers can send junk to unknown clients.)
	 * XXX more checks might be needed
	 */
	if (bootp->bp_yiaddr.s_addr == INADDR_ANY ||
	    bootp->bp_yiaddr.s_addr == INADDR_BROADCAST) {
		printf("nfs_boot: wrong IP addr %s",
		       inet_ntoa(bootp->bp_yiaddr));
		goto warn;
	}

	/*
	 * Check the vendor data.
	 */
	if (memcmp(bootp->bp_vend, vm_rfc1048, 4)) {
		printf("nfs_boot: reply missing options");
		goto warn;
	}
	p = &bootp->bp_vend[4];
	limit = ((char*)bootp) + bpc->replylen;
	while (p < limit) {
		tag = *p++;
		if (tag == TAG_END)
			break;
		if (tag == TAG_PAD)
			continue;
		len = *p++;
		if ((p + len) > limit) {
			printf("nfs_boot: option %d too long", tag);
			goto warn;
		}
		switch (tag) {
#ifdef NFS_BOOT_DHCP
		    case TAG_DHCP_MSGTYPE:
			if (*p != bpc->expected_dhcpmsgtype)
				return (-1);
			bpc->dhcp_ok = 1;
			break;
		    case TAG_SERVERID:
			memcpy(&bpc->dhcp_serverip.s_addr, p,
			      sizeof(bpc->dhcp_serverip.s_addr));
			break;
#endif
		    default:
			break;
		}
		p += len;
	}
	return (0);

warn:
	printf(" (bad reply from %s)\n", inet_ntoa(bootp->bp_siaddr));
	return (-1);
}

static int
bootpc_call(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	struct socket *so;
	struct ifnet *ifp = nd->nd_ifp;
	static u_int32_t xid = ~0xFF;
	struct bootp *bootp;	/* request */
	struct mbuf *m, *nam;
	struct sockaddr_in *sin;
	int error;
	u_char *haddr;
	u_char hafmt, halen;
	struct bootpcontext bpc;

	error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
	if (error) {
		printf("bootp: socreate, error=%d\n", error);
		return (error);
	}

	/*
	 * Initialize to NULL anything that will hold an allocation,
	 * and free each at the end if not null.
	 */
	bpc.replybuf = NULL;
	m = nam = NULL;

	/* Record our H/W (Ethernet) address. */
	{	struct sockaddr_dl *sdl = ifp->if_sadl;
		switch (sdl->sdl_type) {
		    case IFT_ISO88025:
			hafmt = HTYPE_IEEE802;
			break;
		    case IFT_ETHER:
		    case IFT_FDDI:
			hafmt = HTYPE_ETHERNET;
			break;
		    default:
			printf("bootp: unsupported interface type %d\n",
			       sdl->sdl_type);
			error = EINVAL;
			goto out;
		}
		halen = sdl->sdl_alen;
		haddr = (unsigned char *)LLADDR(sdl);
	}

	/*
	 * Skip the route table when sending on this socket.
	 * If this is not done, ip_output finds the loopback
	 * interface (why?) and then fails because broadcast
	 * is not supported on that interface...
	 */
	{	int32_t *opt;
		m = m_get(M_WAIT, MT_SOOPTS);
		opt = mtod(m, int32_t *);
		m->m_len = sizeof(*opt);
		*opt = 1;
		error = sosetopt(so, SOL_SOCKET, SO_DONTROUTE, m);
		m = NULL;	/* was consumed */
	}
	if (error) {
		DPRINT("SO_DONTROUTE");
		goto out;
	}

	/* Enable broadcast. */
	if ((error = nfs_boot_enbroadcast(so))) {
		DPRINT("SO_BROADCAST");
		goto out;
	}

	/* Set the receive timeout for the socket. */
	if ((error = nfs_boot_setrecvtimo(so))) {
		DPRINT("SO_RCVTIMEO");
		goto out;
	}

	/*
	 * Bind the local endpoint to a bootp client port.
	 */
	if ((error = nfs_boot_sobind_ipport(so, IPPORT_BOOTPC))) {
		DPRINT("bind failed\n");
		goto out;
	}

	/*
	 * Setup socket address for the server.
	 */
	nam = m_get(M_WAIT, MT_SONAME);
	sin = mtod(nam, struct sockaddr_in *);
	sin->sin_len = nam->m_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_BROADCAST;
	sin->sin_port = htons(IPPORT_BOOTPS);

	/*
	 * Allocate buffer used for request
	 */
	m = m_gethdr(M_WAIT, MT_DATA);
	MCLGET(m, M_WAIT);
	bootp = mtod(m, struct bootp*);
	m->m_pkthdr.len = m->m_len = BOOTP_SIZE_MAX;
	m->m_pkthdr.rcvif = NULL;

	/*
	 * Build the BOOTP reqest message.
	 * Note: xid is host order! (opaque to server)
	 */
	memset((caddr_t)bootp, 0, BOOTP_SIZE_MAX);
	bootp->bp_op    = BOOTREQUEST;
	bootp->bp_htype = hafmt;
	bootp->bp_hlen  = halen;	/* Hardware address length */
	bootp->bp_xid = ++xid;
	memcpy(bootp->bp_chaddr, haddr, halen);
	/* Fill-in the vendor data. */
	memcpy(bootp->bp_vend, vm_rfc1048, 4);
#ifdef NFS_BOOT_DHCP
	bootp->bp_vend[4] = TAG_DHCP_MSGTYPE;
	bootp->bp_vend[5] = 1;
	bootp->bp_vend[6] = DHCPDISCOVER;
	bootp->bp_vend[7] = TAG_END;
#else
	bootp->bp_vend[4] = TAG_END;
#endif

	bpc.xid = xid;
	bpc.haddr = haddr;
	bpc.halen = halen;
	bpc.replybuf = malloc(BOOTP_SIZE_MAX, M_DEVBUF, M_WAITOK);
	if (bpc.replybuf == NULL)
		panic("nfs_boot: malloc reply buf");
#ifdef NFS_BOOT_DHCP
	bpc.expected_dhcpmsgtype = DHCPOFFER;
	bpc.dhcp_ok = 0;
#endif

	error = nfs_boot_sendrecv(so, nam, bootpset, m,
				  bootpcheck, 0, 0, &bpc);
	if (error)
		goto out;

#ifdef NFS_BOOT_DHCP
	if (bpc.dhcp_ok) {
		u_int32_t leasetime;
		bootp->bp_vend[6] = DHCPREQUEST;
		bootp->bp_vend[7] = TAG_REQ_ADDR;
		bootp->bp_vend[8] = 4;
		memcpy(&bootp->bp_vend[9], &bpc.replybuf->bp_yiaddr, 4);
		bootp->bp_vend[13] = TAG_SERVERID;
		bootp->bp_vend[14] = 4;
		memcpy(&bootp->bp_vend[15], &bpc.dhcp_serverip.s_addr, 4);
		bootp->bp_vend[19] = TAG_LEASETIME;
		bootp->bp_vend[20] = 4;
		leasetime = htonl(300);
		memcpy(&bootp->bp_vend[21], &leasetime, 4);
		bootp->bp_vend[25] = TAG_END;

		bpc.expected_dhcpmsgtype = DHCPACK;

		error = nfs_boot_sendrecv(so, nam, bootpset, m,
					  bootpcheck, 0, 0, &bpc);
		if (error)
			goto out;
	}
#endif

	/*
	 * bootpcheck() has copied the receive mbuf into
	 * the buffer at bpc.replybuf.
	 */
#ifdef NFS_BOOT_DHCP
	printf("nfs_boot: %s server: %s\n",
	       (bpc.dhcp_ok ? "DHCP" : "BOOTP"),
#else
	printf("nfs_boot: BOOTP server: %s\n",
#endif
	       inet_ntoa(bpc.replybuf->bp_siaddr));

	bootp_extract(bpc.replybuf, bpc.replylen, nd);

out:
	if (bpc.replybuf)
		free(bpc.replybuf, M_DEVBUF);
	if (m)
		m_freem(m);
	if (nam)
		m_freem(nam);
	soclose(so);
	return (error);
}

static void
bootp_extract(bootp, replylen, nd)
	struct bootp *bootp;
	int replylen;
	struct nfs_diskless *nd;
{
	struct sockaddr_in *sin;
	struct in_addr netmask;
	struct in_addr gateway;
	struct in_addr rootserver;
	char *myname;	/* my hostname */
	char *mydomain;	/* my domainname */
	char *rootpath;
	int mynamelen;
	int mydomainlen;
	int rootpathlen;
	int overloaded;
	u_int tag, len;
	u_char *p, *limit;

	/* Default these to "unspecified". */
	netmask.s_addr = 0;
	gateway.s_addr = 0;
	mydomain    = myname    = rootpath = NULL;
	mydomainlen = mynamelen = rootpathlen = 0;
	/* default root server to bootp next-server */
	rootserver = bootp->bp_siaddr;
	/* assume that server name field is not overloaded by default */
	overloaded = 0;

	p = &bootp->bp_vend[4];
	limit = ((char*)bootp) + replylen;
	while (p < limit) {
		tag = *p++;
		if (tag == TAG_END)
			break;
		if (tag == TAG_PAD)
			continue;
		len = *p++;
#if 0 /* already done in bootpcheck() */
		if ((p + len) > limit) {
			printf("nfs_boot: option %d too long\n", tag);
			break;
		}
#endif
		switch (tag) {
		    case TAG_SUBNET_MASK:
			memcpy(&netmask, p, 4);
			break;
		    case TAG_GATEWAY:
			/* Routers */
			memcpy(&gateway, p, 4);
			break;
		    case TAG_HOST_NAME:
			if (len >= sizeof(hostname)) {
				printf("nfs_boot: host name >= %lu bytes",
				       (u_long)sizeof(hostname));
				break;
			}
			myname = p;
			mynamelen = len;
			break;
		    case TAG_DOMAIN_NAME:
			if (len >= sizeof(domainname)) {
				printf("nfs_boot: domain name >= %lu bytes",
				       (u_long)sizeof(domainname));
				break;
			}
			mydomain = p;
			mydomainlen = len;
			break;
		    case TAG_ROOT_PATH:
			/* Leave some room for the server name. */
			if (len >= (MNAMELEN-10)) {
				printf("nfs_boot: rootpath >=%d bytes",
				       (MNAMELEN-10));
				break;
			}
			rootpath = p;
			rootpathlen = len;
			break;
		    case TAG_SWAP_SERVER:
			/* override NFS server address */
			memcpy(&rootserver, p, 4);
			break;
#ifdef NFS_BOOT_DHCP
		    case TAG_OVERLOAD:
			if (len > 0 && ((*p & 0x02) != 0))
				/*
				 * The server name field in the dhcp packet
				 * is overloaded and we can't find server
				 * name there.
				 */
				overloaded = 1;
			break;
#endif
		    default:
			break;
		}
		p += len;
	}

	/*
	 * Store and print network config info.
	 */
	if (myname) {
		myname[mynamelen] = '\0';
		strncpy(hostname, myname, sizeof(hostname));
		hostnamelen = mynamelen;
		printf("nfs_boot: my_name=%s\n", hostname);
	}
	if (mydomain) {
		mydomain[mydomainlen] = '\0';
		strncpy(domainname, mydomain, sizeof(domainname));
		domainnamelen = mydomainlen;
		printf("nfs_boot: my_domain=%s\n", domainname);
	}
	nd->nd_myip = bootp->bp_yiaddr;
	if (nd->nd_myip.s_addr)
		printf("nfs_boot: my_addr=%s\n", inet_ntoa(nd->nd_myip));
	nd->nd_mask = netmask;
	if (nd->nd_mask.s_addr)
		printf("nfs_boot: my_mask=%s\n", inet_ntoa(nd->nd_mask));
	nd->nd_gwip = gateway;
	if (nd->nd_gwip.s_addr)
		printf("nfs_boot: gateway=%s\n", inet_ntoa(nd->nd_gwip));

	/*
	 * Store the information about our NFS root mount.
	 * The caller will print it, so be silent here.
	 */
	{
		struct nfs_dlmount *ndm = &nd->nd_root;

		/* Server IP address. */
		sin = (struct sockaddr_in *) &ndm->ndm_saddr;
		memset((caddr_t)sin, 0, sizeof(*sin));
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr = rootserver;
		/* Server name. */
		if (!overloaded && bootp->bp_sname[0] != 0 &&
		    !memcmp(&rootserver, &bootp->bp_siaddr,
			  sizeof(struct in_addr))) {
			/* standard root server, we have the name */
			strncpy(ndm->ndm_host, bootp->bp_sname, BP_SNAME_LEN-1);
		} else {
			/* Show the server IP address numerically. */
			strncpy(ndm->ndm_host, inet_ntoa(rootserver),
				BP_SNAME_LEN-1);
		}
		len = strlen(ndm->ndm_host);
		if (rootpath &&
		    len + 1 + rootpathlen + 1 <= sizeof(ndm->ndm_host)) {
			ndm->ndm_host[len++] = ':';
			strncpy(ndm->ndm_host + len,
				rootpath, rootpathlen);
			ndm->ndm_host[len + rootpathlen] = '\0';
		} /* else: upper layer will handle error */
	}
}
