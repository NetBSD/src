/*	$NetBSD: nfs_bootdhcp.c,v 1.1 1997/08/29 16:10:31 gwr Exp $	*/

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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/if_arp.h> 	/* ARPHRD_ETHER, etc. */
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <nfs/nfsproto.h>
/* #include <nfs/nfs.h> */
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
#define HTYPE_ETHERNET		  1

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


/*
 * The "extended" size is somewhat arbitrary, but is
 * constrained by the maximum message size specified
 * by RFC1533 (567 total).  This value increases the
 * space for options from 64 bytes to 256 bytes.
 */
#define BOOTP_SIZE_MAX	(sizeof(struct bootp)+192)
#define BOOTP_SIZE_MIN	(sizeof(struct bootp))

/*
 * What is the longest we will wait before re-sending a request?
 * Note this is also the frequency of "BOOTP timeout" messages.
 * The re-send loop counts up linearly to this maximum, so the
 * first complaint will happen after (1+2+3+4+5)=15 seconds.
 */
#define	MAX_RESEND_DELAY 5	/* seconds */
#define TOTAL_TIMEOUT   30	/* seconds */

/* Convenience macro */
#define INTOHL(ina) ((u_int32_t)ntohl((ina).s_addr))

static int bootpc_call __P((struct socket *, struct ifnet *,
			struct nfs_diskless *, struct proc *));


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
nfs_bootdhcp(ifp, nd, procp)
	struct ifnet *ifp;
	struct nfs_diskless *nd;
	struct proc *procp;
{
	struct ifaliasreq iareq;
	struct socket *so;
	struct sockaddr_in *sin;
	int error;

	/*
	 * Get a socket to use for various things in here.
	 * After this, use "goto out" to cleanup and return.
	 */
	error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
	if (error) {
		printf("nfs_boot: socreate, error=%d\n", error);
		return (error);
	}

	/*
	 * Do enough of ifconfig(8) so that the chosen interface
	 * can talk to the servers.  Use address zero for now.
	 */
	bzero(&iareq, sizeof(iareq));
	bcopy(ifp->if_xname, iareq.ifra_name, IFNAMSIZ);
	/* Set the I/F address */
	sin = (struct sockaddr_in *)&iareq.ifra_addr;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	/* Leave subnetmask unspecified (len=0) */
	/* Set the broadcast addr. */
	sin = (struct sockaddr_in *)&iareq.ifra_broadaddr;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_BROADCAST;
	error = ifioctl(so, SIOCAIFADDR, (caddr_t)&iareq, procp);
	if (error) {
		printf("nfs_boot: set ifaddr zero, error=%d\n", error);
		goto out;
	}

	/* This function call does the real send/recv work. */
	error = bootpc_call(so, ifp, nd, procp);
	/* Get rid of the temporary (zero) IP address. */
	(void) ifioctl(so, SIOCDIFADDR, (caddr_t)&iareq, procp);
	/* NOW we can test the error from bootpc_call. */
	if (error)
		goto out;

	/*
	 * Do ifconfig with our real IP address and mask.
	 */
	/* I/F address */
	sin = (struct sockaddr_in *)&iareq.ifra_addr;
	sin->sin_addr = nd->nd_myip;
	/* subnetmask */
	if (nd->nd_mask.s_addr) {
		sin = (struct sockaddr_in *)&iareq.ifra_mask;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr = nd->nd_mask;
	}
	/* Let ifioctl() default the broadcast address. */
	sin = (struct sockaddr_in *)&iareq.ifra_broadaddr;
	sin->sin_len = 0;
	sin->sin_family = 0;
	sin->sin_addr.s_addr = 0;
	error = ifioctl(so, SIOCAIFADDR, (caddr_t)&iareq, procp);
	if (error) {
		printf("nfs_boot: set ifaddr real, error=%d\n", error);
		goto out;
	}

out:
	soclose(so);
	return (error);
}

static int
bootpc_call(so, ifp, nd, procp)
	struct socket *so;
	struct ifnet *ifp;
	struct nfs_diskless *nd;
	struct proc *procp;
{
	static u_int32_t xid = ~0xFF;
	struct uio uio;
	struct iovec iov;
	struct in_addr netmask;
	struct in_addr gateway;
	char *myname;	/* my hostname */
	char *mydomain;	/* my domainname */
	char *rootpath;
	int mynamelen;
	int mydomainlen;
	int rootpathlen;
	struct bootp *bootp;	/* request and reply */
	struct mbuf *m, *nam;
	struct sockaddr_in *sin;
	int error, rcvflg, timo, secs, waited;
	int sendlen, recvlen;
	u_int tag, len;
	u_char *p, *limit;
	u_char *haddr;
	u_char hafmt, halen;

	/*
	 * Initialize to NULL anything that will hold an allocation,
	 * and free each at the end if not null.
	 */
	bootp = NULL;
	m = nam = NULL;

	/* Record our H/W (Ethernet) address. */
	{	struct sockaddr_dl *sdl = ifp->if_sadl;
		hafmt = HTYPE_ETHERNET; /* XXX: sdl->sdl_type? */
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
	{	int32_t *opt;
		m = m_get(M_WAIT, MT_SOOPTS);
		opt = mtod(m, int32_t *);
		m->m_len = sizeof(*opt);
		*opt = 1;
		error = sosetopt(so, SOL_SOCKET, SO_BROADCAST, m);
		m = NULL;	/* was consumed */
	}
	if (error) {
		DPRINT("SO_BROADCAST");
		goto out;
	}

	/* Set the receive timeout for the socket. */
	{	struct timeval *tv;
		m = m_get(M_WAIT, MT_SOOPTS);
		tv = mtod(m, struct timeval *);
		m->m_len = sizeof(*tv);
		tv->tv_sec = 1;
		tv->tv_usec = 0;
		error = sosetopt(so, SOL_SOCKET, SO_RCVTIMEO, m);
		m = NULL;	/* was consumed */
	}
	if (error) {
		DPRINT("SO_RCVTIMEO");
		goto out;
	}

	/*
	 * Bind the local endpoint to a bootp client port.
	 */
	m = m_getclr(M_WAIT, MT_SONAME);
	sin = mtod(m, struct sockaddr_in *);
	sin->sin_len = m->m_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = htons(IPPORT_BOOTPC);
	error = sobind(so, m);
	m_freem(m);
	if (error) {
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
	 * Allocate buffer used for request/reply
	 */
	bootp = malloc(BOOTP_SIZE_MAX, M_DEVBUF, M_WAITOK);
	if (bootp == NULL)
		panic("nfs_boot: malloc buf");

	/*
	 * Send it, repeatedly, until a reply is received,
	 * but delay each re-send by an increasing amount.
	 * If the delay hits the maximum, start complaining.
	 * Try extended-length request first, and if that
	 * results in total timeout, start over with the
	 * standard-length BOOTP request.
	 */
	sendlen = BOOTP_SIZE_MAX;
	waited = timo = 0;
send_again:
	waited += timo;
	if (waited >= TOTAL_TIMEOUT) {
		if (sendlen > BOOTP_SIZE_MIN) {
			sendlen = BOOTP_SIZE_MIN;
			waited = timo = 0;
			printf("nfs_boot: trying short requests\n");
			goto send_again;
		}
		error = ETIMEDOUT;
		goto out;
	}
	/* Determine new timeout. */
	if (timo < MAX_RESEND_DELAY)
		timo++;
	else
		printf("nfs_boot: BOOTP timeout...\n");

	/*
	 * Build the BOOTP reqest message.  Do it every time
	 * we send a new request because we reuse the buffer.
	 * Also, use a new transaction ID for each request.
	 * Note: xid is host order! (opaque to server)
	 */
	bzero((caddr_t)bootp, BOOTP_SIZE_MAX);
	bootp->bp_op    = BOOTREQUEST;
	bootp->bp_htype = hafmt;
	bootp->bp_hlen  = halen;	/* Hardware address length */
	bootp->bp_xid = ++xid;
	bootp->bp_secs = htons(waited);
	bcopy(haddr, bootp->bp_chaddr, halen);
	/* Fill-in the vendor data. */
	bcopy(vm_rfc1048, bootp->bp_vend, 4);
	bootp->bp_vend[4] = TAG_END;

	/*
	 * The BOOTP request is complete.  Send it.
	 */
	iov.iov_base = (caddr_t) bootp;
	iov.iov_len = sendlen;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_rw = UIO_WRITE;
	uio.uio_offset = 0;
	uio.uio_resid = sendlen;
	uio.uio_procp = procp;
	error = sosend(so, nam, &uio, NULL, NULL, 0);
	if (error) {
		printf("nfs_boot: sosend: %d\n", error);
		goto out;
	}

	/*
	 * Wait for up to timo seconds for a reply.
	 * The socket receive timeout was set to 1 second.
	 */
	secs = timo;
	for (;;) {
		iov.iov_base = (caddr_t) bootp;
		iov.iov_len = BOOTP_SIZE_MAX;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_segflg = UIO_SYSSPACE;
		uio.uio_rw = UIO_READ;
		uio.uio_offset = 0;
		uio.uio_resid = BOOTP_SIZE_MAX;
		uio.uio_procp = procp;

		rcvflg = 0;
		error = soreceive(so, NULL, &uio, NULL, NULL, &rcvflg);
		if (error == EWOULDBLOCK) {
			if (--secs <= 0)
				goto send_again;
			continue;
		}
		if (error)
			goto out;
		recvlen = BOOTP_SIZE_MAX - uio.uio_resid;

		/*
		 * Is this a valid reply?
		 */
		if (recvlen < BOOTP_SIZE_MIN) {
			DPRINT("short packet");
			continue;
		}
		if (bootp->bp_op != BOOTREPLY) {
			DPRINT("not reply");
			continue;
		}
		if (bootp->bp_hlen != halen) {
			DPRINT("bad hwa_len");
			continue;
		}
		if (bcmp(bootp->bp_chaddr, haddr, halen)) {
			DPRINT("wrong hwaddr");
			continue;
		}
		if (bootp->bp_xid != xid) {
			DPRINT("wrong xid");
			continue;
		}

		/*
		 * OK, we have a valid reply.
		 * Decode the vendor data.
		 */
		if (bcmp(bootp->bp_vend, vm_rfc1048, 4)) {
			printf("nfs_boot: reply missing options\n");
			continue;
		}
		/* Default these to "unspecified". */
		netmask.s_addr = 0;
		gateway.s_addr = 0;
		mydomain    = myname    = rootpath = NULL;
		mydomainlen = mynamelen = rootpathlen = 0;
		p = &bootp->bp_vend[4];
		limit = ((char*)bootp) + recvlen;
		while (p < limit) {
			tag = *p++;
			if (tag == TAG_END)
				break;
			if (tag == TAG_PAD)
				continue;
			len = *p++;
			if (len == 0)
				continue;
			if ((p + len) > limit) {
				printf("nfs_boot: option %d too long\n", tag);
				break;
			}
			switch (tag) {
			case TAG_SUBNET_MASK:
				bcopy(p, &netmask, 4);
				break;
			case TAG_GATEWAY:
				/* Routers */
				bcopy(p, &gateway, 4);
				break;
			case TAG_HOST_NAME:
				if (len >= sizeof(hostname)) {
					printf("nfs_boot: host name >=%d bytes",
					       sizeof(hostname));
					break;
				}
				myname = p;
				mynamelen = len;
				break;
			case TAG_DOMAIN_NAME:
				if (len >= sizeof(domainname)) {
					printf("nfs_boot: domain name >=%d bytes",
					       sizeof(domainname));
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
			default:
				break;
			}
			p += len;
		}

		if (rootpath == NULL) {
			printf("nfs_boot: No root path offered\n");
			continue;
		}

		/* OK, the reply has everything we need. */
		break;
	}
	rootpath[rootpathlen] = '\0';

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
		printf("nfs_boot: my_addr=0x%x\n", INTOHL(nd->nd_myip));
	nd->nd_mask = netmask;
	if (nd->nd_mask.s_addr)
		printf("nfs_boot: my_mask=0x%x\n", INTOHL(nd->nd_mask));
	nd->nd_gwip = gateway;
	if (nd->nd_gwip.s_addr)
		printf("nfs_boot: gateway=0x%x\n", INTOHL(nd->nd_gwip));

	/*
	 * Store the information about our NFS root mount.
	 * The caller will print it, so be silent here.
	 */
	{
		struct nfs_dlmount *ndm = &nd->nd_root;

		/* Server IP address. */
		sin = (struct sockaddr_in *) &ndm->ndm_saddr;
		bzero((caddr_t)sin, sizeof(*sin));
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr = bootp->bp_siaddr;
		/* Server name. */
		strncpy(ndm->ndm_host, bootp->bp_sname, BP_SNAME_LEN-1);
		len = strlen(ndm->ndm_host);
		if ((len + 1 + rootpathlen + 1) > sizeof(ndm->ndm_host)) {
			/* Show the server IP address numerically. */
			sprintf(ndm->ndm_host, "0x%8x",
			        INTOHL(bootp->bp_siaddr));
			len = strlen(ndm->ndm_host);
		}
		ndm->ndm_host[len++] = ':';
		strncpy(ndm->ndm_host + len,
		    rootpath, rootpathlen + 1);
	}
	error = 0;

out:
	if (bootp)
		free(bootp, M_DEVBUF);
	if (nam)
		m_freem(nam);
	return (error);
}
