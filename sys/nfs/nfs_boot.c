/*
 * Copyright (c) 1994 Adam Glass, Gordon Ross
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Adam Glass BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /cvsroot/src/sys/nfs/nfs_boot.c,v 1.1 1994/04/18 06:18:20 glass Exp $
 */

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>
#include <nfs/nfs.h>
#include <nfs/nfsdiskless.h>

/*
 * Support for NFS diskless booting, specifically getting information
 * about where to boot from, what pathnames, etc.
 *
 * We currently support the RPC bootparam protocol.
 *
 * We'd like to support BOOTP, but someone needs to write small kernel-ized
 * BOOTP client
 *
 */

/* from rfc951 to avoid bringing in cmu header file */

#define UDP_BOOTPSERVER 67
#define UDP_BOOTPCLIENT 68

#define BOOTP_REQUEST 1
#define BOOTP_REPLY   2

/* rfc1048 tag bytes, (from rfc1497), only the semi-useful bits */
#define TAG_PAD                0	/* [1] no data */
#define TAG_SUBNET_MASK        1        /* [4] subnet mask bytes */
#define TAG_GATEWAY_ADDR       3        /* [addr] gateway address */
#define TAG_DNS_ADDR           6        /* [addr] dns name server */
#define TAG_HOSTNAME          12        /* [n] hostname */
#define TAG_BOOT_SIZE         13        /* [2] boot file size?*/
#define TAG_DOMAIN_NAME       15        /* [n] domain name */
#define TAG_SWAP_ADDR         16        /* [addr] swap server */
#define TAG_ROOT_PATH         17        /* [n] root path */
#define TAG_END              255

#define BOOTP_ROOT 1
#define BOOTP_SWAP 2
#define BOOTP_COMPLETED (BOOTP_ROOT|BOOTP_SWAP)

struct bootp_msg {
	u_char bpm_op;		/* packet op code / message type */
	u_char bpm_htype;	/* hardware address type */
	u_char bpm_hlen;	/* hardware address length */
	u_char bpm_hops;	/* bootp hops XXX ugly*/
	u_long bpm_xid;		/* transaction ID */
	u_short bpm_secs;	/* seconds elapsed since boot */
	u_short bpm_unused;	
	struct in_addr bpm_ciaddr; /* client IP address */
	struct in_addr bpm_yiaddr; /* 'your' (client) IP address */
	struct in_addr bpm_siaddr; /* server IP address */
	struct in_addr bpm_giaddr; /* gateway IP address */
	u_char bpm_chaddr[16];	/* client hardware address */
	u_char bpm_sname[64];	/* optional server host name */
	u_char bpm_file[128];	/* boot file name */
	u_char bpm_vendor[64];	/* vendor-specific data */
};

static u_char vend_rfc1048[4] = {
	99, 130, 83, 99,
};

/*
 * Get a file handle given a path name.
 */
static int
nfs_boot_getfh(sa, path, fhp)
	struct sockaddr *sa;		/* server address */
	char *path;
	u_char *fhp;
{
	/* The RPC structures */
	struct sdata {
		u_long  len;
		u_char  path[4];	/* longer, of course */
	} *sdata;
	struct rdata {
		u_long	errno;
		u_char	fh[NFS_FHSIZE];
	} *rdata;
	struct sockaddr_in *sin;
	struct mbuf *m;
	int error, mlen, slen;

	/*
	 * Validate address family.
	 * Sorry, this is INET specific...
	 */
	if (sa->sa_family != AF_INET)
		return EAFNOSUPPORT;

	slen = strlen(path);
	if (slen > (MLEN-4))
		slen = (MLEN-4);
	mlen = 4 + ((slen + 3) & ~3); /* XXX ??? */

	m = m_get(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;
	m->m_len = mlen;
	sdata = mtod(m, struct sdata *);
	sdata->len = htonl(slen);
	bcopy(path, sdata->path, slen);

	/* Do RPC to mountd. */
	error = krpc_call(sa, RPCPROG_MNT, RPCMNT_VER1, RPCMNT_MOUNT,
			  &m, sizeof(*rdata));
	if (error) 
		return error;

	rdata = mtod(m, struct rdata *);
	error = ntohl(rdata->errno);
	if (!error)
		bcopy(rdata->fh, fhp, NFS_FHSIZE);
	m_freem(m);
	return error;
}

/*
 * Receive a bootp reply
 */
static int bootp_receive(so, msg)
	struct socket *so;
	struct bootp_msg *msg;
{
	int error, rcvflag = 0;
	struct mbuf *m;
	struct uio auio;

	auio.uio_resid = (1<<16);

	error = soreceive(so, NULL, &auio, &m, NULL, &rcvflag);
	if (error)
		return error;
	if (m->m_len < sizeof(*msg)) {
		m = m_pullup(m, sizeof(*msg));
		if (m == NULL)
			return ENOBUFS;
	}
	if (((1<<16) - auio.uio_resid) != sizeof(*msg))
		return EMSGSIZE;
	m_copydata(m, 0, sizeof(*msg), (caddr_t) msg);
	return 0;
}

/*
 * Send a bootp request
 */
static int bootp_send(so, nam, start, msg)
	struct socket *so;
	struct mbuf *nam;
	time_t start;
	struct bootp_msg *msg;
{
	int error;
	struct mbuf *m;
	struct bootp_msg *bpm;

	MGETHDR(m, M_WAIT, MT_DATA);
	m->m_len = MHLEN;
	if (m->m_len < sizeof(*msg)) {
		MCLGET(m, M_WAIT);
		m->m_len = min(sizeof(*msg), MCLBYTES);
	}
	m->m_pkthdr.len = sizeof(*msg);
	m->m_pkthdr.rcvif = NULL;
	bpm = mtod(m, struct bootp_msg *);
	bcopy((caddr_t) msg, (caddr_t) bpm, sizeof(*bpm));
	bpm->bpm_secs = time.tv_sec - start;

	error = sosend(so, nam, NULL, m, NULL, 0);
	if (error)
		return error;
}

/*
 * Fill in as much of the nfs_diskless struct using the results
 * of bootp requests.
 */
int nfs_boot_bootp(diskless, rpath, spath)
	struct nfs_diskless *diskless;
	char *rpath, *spath;
{
	int error, bootp_timeout = 30, *opt_val, have, need;
	time_t start;
	struct socket *so;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct mbuf *m, *nam;
	struct timeval *tv;
	struct bootp_msg outgoing, incoming;

	sa = &diskless->myif.ifra_broadaddr;
	if (sa->sa_family != AF_INET)
		return EAFNOSUPPORT;

	/*
	 * Create socket and set its recieve timeout.
	 */
	if (error = socreate(AF_INET, &so, SOCK_DGRAM, 0))
		return error;
	nam = m_get(M_WAIT, MT_SONAME);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t) sin, sizeof(*sin));
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);
	sin->sin_port = htons(UDP_BOOTPCLIENT);
	nam->m_len = sizeof(struct sockaddr_in);
	if (error = sobind(so, nam))
		goto out;
	m = m_get(M_WAIT, MT_SOOPTS);
	tv = mtod(m, struct timeval *);
	m->m_len = sizeof(*tv);
	tv->tv_sec = 5;
	tv->tv_usec = 0;
	if ((error = sosetopt(so, SOL_SOCKET, SO_RCVTIMEO, m)))
		goto out;
	m = m_get(M_WAIT, MT_SOOPTS);
	opt_val = mtod(m, int *);
	m->m_len = sizeof(*opt_val);
	*opt_val = 1;
	if ((error = sosetopt(so, SOL_SOCKET, SO_BROADCAST, m))) 
		goto out;

	/*
	 * Setup socket address for the server.
	 */
	nam = m_get(M_WAIT, MT_SONAME);
	sin = mtod(nam, struct sockaddr_in *);
	bcopy((caddr_t)sa, (caddr_t)sin, sizeof(struct sockaddr));
	sin->sin_port = htons(UDP_BOOTPSERVER);
	nam->m_len = sizeof(struct sockaddr);
	
	bzero((caddr_t) &outgoing, sizeof(outgoing));
	outgoing.bpm_op = BOOTP_REQUEST;
	outgoing.bpm_htype = 1;
	outgoing.bpm_hlen = 6;
	outgoing.bpm_hops = 0;
	sin = (struct sockaddr_in *) &diskless->myif.ifra_addr;
	bcopy((caddr_t) &sin->sin_addr, (caddr_t) &outgoing.bpm_ciaddr, 4);
	bcopy((caddr_t) &sin->sin_addr, (caddr_t) &outgoing.bpm_yiaddr, 4);
	bcopy((caddr_t) vend_rfc1048, (caddr_t) outgoing.bpm_vendor, 4);
	outgoing.bpm_xid = time.tv_usec;
	outgoing.bpm_vendor[4] = TAG_END;
	start = time.tv_sec;

	have = 0;
	while (bootp_timeout-- && (have != BOOTP_COMPLETED)) {
		if ((have & BOOTP_ROOT) == 0)
			strcpy(outgoing.bpm_file, "root");
		else if ((have & BOOTP_SWAP) == 0)
			strcpy(outgoing.bpm_file, "swap");
		if (error = bootp_send(so, nam, start, &outgoing)) {
			goto out;
		}
		error = bootp_receive(so, &incoming);
		if (error == EWOULDBLOCK) 
			continue;
		if (error)
			goto out;

		if (outgoing.bpm_xid != incoming.bpm_xid)
			continue;
		if ((have & BOOTP_ROOT) == 0) {
			sin = (struct sockaddr_in *) &diskless->root_saddr;
			sin->sin_family = AF_INET;
			sin->sin_len = sizeof(*sin);
			bcopy((caddr_t) &incoming.bpm_siaddr,
			      (caddr_t) &sin->sin_addr, sizeof(sin->sin_addr));
			strcpy(diskless->root_hostnam, incoming.bpm_sname);
			strcpy(rpath, incoming.bpm_file);
			have |= BOOTP_ROOT;
			outgoing.bpm_xid++;
		}
		else if ((have & BOOTP_SWAP) == 0) {
			sin = (struct sockaddr_in *) &diskless->swap_saddr;
			sin->sin_family = AF_INET;
			sin->sin_len = sizeof(*sin);
			bcopy((caddr_t) &incoming.bpm_siaddr,
			      (caddr_t) &sin->sin_addr, sizeof(sin->sin_addr));
			strcpy(diskless->swap_hostnam, incoming.bpm_sname);
			strcpy(spath, incoming.bpm_file);
			have |= BOOTP_SWAP;
		}
	}
		
	if (have != BOOTP_COMPLETED)
		error = ETIMEDOUT;
 out:
	if (nam)
		m_freem(nam);
	soclose(so);

	return error;
}

/*
 * Called with a nfs_diskless struct in which a interface ip addr,
 * broadcast addr, and netmask have been specified.,
 *
 * The responsibility of this routine is to fill out the rest of the
 * struct using whatever mechanism.
 *
 */
int nfs_boot(diskless)
	struct nfs_diskless *diskless;
{
	int error;
	u_short port;
	struct sockaddr_in *sin;
	char root_path[MAXPATHLEN], swap_path[MAXPATHLEN];

	error = nfs_boot_bootp(diskless, root_path, swap_path);
	if (error)
		return error;

	sin = (struct sockaddr_in *) &diskless->root_saddr;
	error = nfs_boot_getfh(&diskless->root_saddr, root_path,
			       diskless->root_fh);
	if (error)
		return error;
	error = krpc_portmap(&diskless->root_saddr,
			     NFS_PROG, NFS_VER2, &sin->sin_port);
	if (error)
		return error;

	sin = (struct sockaddr_in *) &diskless->swap_saddr;
	error = nfs_boot_getfh(&diskless->swap_saddr, swap_path,
			       diskless->swap_fh);
	if (error)
		return error;
	error = krpc_portmap(&diskless->swap_saddr,
			     NFS_PROG, NFS_VER2, &sin->sin_port);
	if (error)
		return error;

	printf("root on %x:%s\n", diskless->root_hostnam, root_path);
	printf("swap on %x:%s\n", diskless->swap_hostnam, swap_path);
	diskless->root_args.addr = &diskless->root_saddr;
	diskless->swap_args.addr = &diskless->swap_saddr;
	diskless->root_args.sotype = diskless->swap_args.sotype = SOCK_DGRAM;
	diskless->root_args.proto = diskless->swap_args.proto = 0;
	diskless->root_args.flags = diskless->swap_args.flags = 0;
	diskless->root_args.wsize = diskless->swap_args.wsize = NFS_WSIZE;
	diskless->root_args.rsize = diskless->swap_args.rsize = NFS_RSIZE;
	diskless->root_args.timeo = diskless->swap_args.timeo = NFS_TIMEO;
	diskless->root_args.retrans = diskless->swap_args.retrans =
		NFS_RETRANS;
	diskless->root_args.hostname = diskless->root_hostnam;
	diskless->swap_args.hostname = diskless->swap_hostnam;
	return 0;
}
