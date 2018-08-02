/*	$KAME: sctp_sys_calls.c,v 1.10 2005/03/06 16:04:16 itojun Exp $ */
/*	$NetBSD: sctp_sys_calls.c,v 1.1 2018/08/02 08:40:48 rjs Exp $ */

/*
 * Copyright (C) 2002, 2003, 2004 Cisco Systems Inc,
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
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp.h>

#include <net/if_dl.h>

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a)		     \
	((*(const u_int32_t *)(const void *)(&(a)->s6_addr[0]) == 0) &&	\
	 (*(const u_int32_t *)(const void *)(&(a)->s6_addr[4]) == 0) &&	\
	 (*(const u_int32_t *)(const void *)(&(a)->s6_addr[8]) == ntohl(0x0000ffff)))
#endif

#define SCTP_CONTROL_VEC_SIZE_RCV	16384

#ifdef SCTP_DEBUG_PRINT_ADDRESS
static void
SCTPPrintAnAddress(struct sockaddr *a)
{
	char stringToPrint[256];
	u_short prt;
	char *srcaddr, *txt;

	if (a == NULL) {
		printf("NULL\n");
		return;
	}
	if (a->sa_family == AF_INET) {
		srcaddr = (char *)&((struct sockaddr_in *)a)->sin_addr;
		txt = "IPv4 Address: ";
		prt = ntohs(((struct sockaddr_in *)a)->sin_port);
	} else if (a->sa_family == AF_INET6) {
		srcaddr = (char *)&((struct sockaddr_in6 *)a)->sin6_addr;
		prt = ntohs(((struct sockaddr_in6 *)a)->sin6_port);
		txt = "IPv6 Address: ";
	} else if (a->sa_family == AF_LINK) {
		int i;
		char tbuf[200];
		u_char adbuf[200];
		struct sockaddr_dl *dl;

		dl = (struct sockaddr_dl *)a;
		strncpy(tbuf, dl->sdl_data, dl->sdl_nlen);
		tbuf[dl->sdl_nlen] = 0;
		printf("Intf:%s (len:%d)Interface index:%d type:%x(%d) ll-len:%d ",
		    tbuf, dl->sdl_nlen, dl->sdl_index, dl->sdl_type,
		    dl->sdl_type, dl->sdl_alen);
		memcpy(adbuf, LLADDR(dl), dl->sdl_alen);
		for (i = 0; i < dl->sdl_alen; i++){
			printf("%2.2x", adbuf[i]);
			if (i < (dl->sdl_alen - 1))
				printf(":");
		}
		printf("\n");
	/*	u_short	sdl_route[16];*/	/* source routing information */
		return;
	} else {
		return;
	}
	if (inet_ntop(a->sa_family, srcaddr, stringToPrint,
	    sizeof(stringToPrint))) {
		if (a->sa_family == AF_INET6) {
			printf("%s%s:%d scope:%d\n", txt, stringToPrint, prt,
			    ((struct sockaddr_in6 *)a)->sin6_scope_id);
		} else {
			printf("%s%s:%d\n", txt, stringToPrint, prt);
		}

	} else {
		printf("%s unprintable?\n", txt);
	}
}
#endif /* SCTP_DEBUG_PRINT_ADDRESS */

void
in6_sin6_2_sin(struct sockaddr_in *sin, struct sockaddr_in6 *sin6)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = AF_INET;
	sin->sin_port = sin6->sin6_port;
	sin->sin_addr.s_addr = sin6->sin6_addr.__u6_addr.__u6_addr32[3];
}

int
sctp_connectx(int sd, struct sockaddr *addrs, int addrcnt,
		sctp_assoc_t *id)
{
	int i, ret, cnt;
	struct sockaddr *at;
	struct sctp_connectx_addrs sca;
#if 0
	char *cpto;
#endif
	size_t len;
	
	at = addrs;
	cnt = 0;
	len = 0;
	/* validate all the addresses and get the size */
	for (i = 0; i < addrcnt; i++) {
		if (at->sa_family == AF_INET) {
			len += at->sa_len;
		} else if (at->sa_family == AF_INET6){
			if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)at)->sin6_addr)){
				len += sizeof(struct sockaddr_in);
#if 0
				in6_sin6_2_sin((struct sockaddr_in *)cpto,
				    (struct sockaddr_in6 *)at);
				cpto = ((caddr_t)cpto + sizeof(struct sockaddr_in));
				len += sizeof(struct sockaddr_in);
#endif
			} else {
				len += at->sa_len;
			}
		} else {
			errno = EINVAL;
			return (-1);
		}
		at = (struct sockaddr *)((caddr_t)at + at->sa_len);
		cnt++;
        }
	/* do we have any? */
	if (cnt == 0) {
		errno = EINVAL;
		return(-1);
	}

	sca.cx_num = cnt;
	sca.cx_len = len;
	sca.cx_addrs = addrs;
	ret = ioctl(sd, SIOCCONNECTX, (void *)&sca);
	if ((ret == 0) && (id != NULL)) {
		memcpy(id, &sca.cx_num, sizeof(sctp_assoc_t));
	}
	return (ret);
}

int
sctp_bindx(int sd, struct sockaddr *addrs, int addrcnt, int flags)
{
	struct sctp_getaddresses *gaddrs;
	struct sockaddr *sa;
	int i, sz, fam, argsz;

	if ((flags != SCTP_BINDX_ADD_ADDR) && 
	    (flags != SCTP_BINDX_REM_ADDR)) {
		errno = EFAULT;
		return(-1);
	}
	argsz = (sizeof(struct sockaddr_storage) +
	    sizeof(struct sctp_getaddresses));
	gaddrs = (struct sctp_getaddresses *)calloc(1, argsz);
	if (gaddrs == NULL) {
		errno = ENOMEM;
		return(-1);
	}
	gaddrs->sget_assoc_id = 0;
	sa = addrs;
	for (i = 0; i < addrcnt; i++) {
		sz = sa->sa_len;
		fam = sa->sa_family;
		((struct sockaddr_in *)&addrs[i])->sin_port = ((struct sockaddr_in *)sa)->sin_port;
		if ((fam != AF_INET) && (fam != AF_INET6)) {
			errno = EINVAL;
			return(-1);
		}
		memcpy(gaddrs->addr, sa, sz);
		if (setsockopt(sd, IPPROTO_SCTP, flags, gaddrs,
		    (unsigned int)argsz) != 0) {
			free(gaddrs);
			return(-1);
		}
		memset(gaddrs, 0, argsz);
		sa = (struct sockaddr *)((caddr_t)sa + sz);
	}
	free(gaddrs);
	return(0);
}


int
sctp_opt_info(int sd, sctp_assoc_t id, int opt, void *arg, socklen_t *size)
{
	if ((opt == SCTP_RTOINFO) || 
 	    (opt == SCTP_ASSOCINFO) || 
	    (opt == SCTP_PRIMARY_ADDR) || 
	    (opt == SCTP_SET_PEER_PRIMARY_ADDR) || 
	    (opt == SCTP_PEER_ADDR_PARAMS) || 
	    (opt == SCTP_STATUS) || 
	    (opt == SCTP_GET_PEER_ADDR_INFO)) { 
		*(sctp_assoc_t *)arg = id;
		return(getsockopt2(sd, IPPROTO_SCTP, opt, arg, size));
	} else {
		errno = EOPNOTSUPP;
		return(-1);
	}
}

int
sctp_getpaddrs(int sd, sctp_assoc_t id, struct sockaddr **raddrs)
{
	struct sctp_getaddresses *addrs;
	struct sockaddr *sa;
	struct sockaddr *re;
	sctp_assoc_t asoc;
	caddr_t lim;
	unsigned int siz;
	int cnt;

	if (raddrs == NULL) {
		errno = EFAULT;
		return(-1);
	}
	asoc = id;
	siz = sizeof(sctp_assoc_t);  
	if (getsockopt2(sd, IPPROTO_SCTP, SCTP_GET_REMOTE_ADDR_SIZE,
	    &asoc, &siz) != 0) {
		return(-1);
	}
	siz = (unsigned int)asoc;
	siz += sizeof(struct sctp_getaddresses);
	addrs = calloc((unsigned long)1, (unsigned long)siz);
	if (addrs == NULL) {
		errno = ENOMEM;
		return(-1);
	}
	memset(addrs, 0, (size_t)siz);
	addrs->sget_assoc_id = id;
	/* Now lets get the array of addresses */
	if (getsockopt2(sd, IPPROTO_SCTP, SCTP_GET_PEER_ADDRESSES,
	    addrs, &siz) != 0) {
		free(addrs);
		return(-1);
	}
	re = (struct sockaddr *)&addrs->addr[0];
	*raddrs = re;
	cnt = 0;
	sa = (struct sockaddr *)&addrs->addr[0];
	lim = (caddr_t)addrs + siz;
	while ((caddr_t)sa < lim) {
		cnt++;
		sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
		if (sa->sa_len == 0)
			break;
	}
	return(cnt);
}

void sctp_freepaddrs(struct sockaddr *addrs)
{
	/* Take away the hidden association id */
	void *fr_addr;
	fr_addr = (void *)((caddr_t)addrs - sizeof(sctp_assoc_t));
	/* Now free it */
	free(fr_addr);
}

int
sctp_getladdrs (int sd, sctp_assoc_t id, struct sockaddr **raddrs)
{
	struct sctp_getaddresses *addrs;
	struct sockaddr *re;
	caddr_t lim;
	struct sockaddr *sa;
	int size_of_addresses;
	unsigned int siz;
	int cnt;

	if (raddrs == NULL) {
		errno = EFAULT;
		return(-1);
	}
	size_of_addresses = 0;
	siz = sizeof(int);  
	if (getsockopt2(sd, IPPROTO_SCTP, SCTP_GET_LOCAL_ADDR_SIZE,
	    &size_of_addresses, &siz) != 0) {
		return(-1);
	}
	if (size_of_addresses == 0) {
		errno = ENOTCONN;
		return(-1);
	}
	siz = size_of_addresses + sizeof(struct sockaddr_storage);
	siz += sizeof(struct sctp_getaddresses);
	addrs = calloc((unsigned long)1, (unsigned long)siz);
	if (addrs == NULL) {
		errno = ENOMEM;
		return(-1);
	}
	memset(addrs, 0, (size_t)siz);
	addrs->sget_assoc_id = id;
	/* Now lets get the array of addresses */
	if (getsockopt2(sd, IPPROTO_SCTP, SCTP_GET_LOCAL_ADDRESSES, addrs,
	    &siz) != 0) {
		free(addrs);
		return(-1);
	}
	re = (struct sockaddr *)&addrs->addr[0];
	*raddrs = re;
	cnt = 0;
	sa = (struct sockaddr *)&addrs->addr[0];
	lim = (caddr_t)addrs + siz;
	while ((caddr_t)sa < lim) {
		cnt++;
		sa = (struct sockaddr *)((caddr_t)sa + sa->sa_len);
		if (sa->sa_len == 0)
			break;
	}
	return(cnt);
}

void sctp_freeladdrs(struct sockaddr *addrs)
{
	/* Take away the hidden association id */
	void *fr_addr;
	fr_addr = (void *)((caddr_t)addrs - sizeof(sctp_assoc_t));
	/* Now free it */
	free(fr_addr);
}

ssize_t
sctp_sendmsg(int s, 
	     const void *data, 
	     size_t len,
	     const struct sockaddr *to,
	     socklen_t tolen __attribute__((unused)),
	     u_int32_t ppid,
	     u_int32_t flags,
	     u_int16_t stream_no,
	     u_int32_t timetolive,
	     u_int32_t context)
{
	int sz;
	struct msghdr msg;
	struct iovec iov[2];
	char controlVector[256];
	struct sctp_sndrcvinfo *s_info;
	struct cmsghdr *cmsg;
	struct sockaddr *who=NULL;
	union {
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
	} addr;

#if 0
	fprintf(io, "sctp_sendmsg(sd:%d, data:%x, len:%d, to:%x, tolen:%d, ppid:%x, flags:%x str:%d ttl:%d ctx:%x\n",
	    s, (u_int)data, (int)len, (u_int)to, (int)tolen, ppid, flags,
	    (int)stream_no, (int)timetolive, (u_int)context);
	fflush(io);
#endif
	if (to) {
		if (to->sa_len == 0) {
			/*
			 * For the lazy app, that did not
			 * set sa_len, we attempt to set for them.
			 */
			switch (to->sa_family) {
			case AF_INET:
				memcpy(&addr, to, sizeof(struct sockaddr_in));
				addr.in.sin_len = sizeof(struct sockaddr_in);
				break;
			case AF_INET6:
				memcpy(&addr, to, sizeof(struct sockaddr_in6));
				addr.in6.sin6_len = sizeof(struct sockaddr_in6);
				break;
			default:
				errno = EAFNOSUPPORT;
				return -1;
			}
		} else {
			memcpy (&addr, to, to->sa_len);
		}
		who = (struct sockaddr *)&addr;
	}
	iov[0].iov_base = (void *)(unsigned long)data;
	iov[0].iov_len = len;
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;

	if (to) {
		msg.msg_name = (caddr_t)who;
		msg.msg_namelen = who->sa_len;
	} else {
		msg.msg_name = (caddr_t)NULL;
		msg.msg_namelen = 0;
	}
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)controlVector;

	cmsg = (struct cmsghdr *)controlVector;

	cmsg->cmsg_level = IPPROTO_SCTP;
	cmsg->cmsg_type = SCTP_SNDRCV;
	cmsg->cmsg_len = CMSG_LEN (sizeof(struct sctp_sndrcvinfo) );
	s_info = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);

	s_info->sinfo_stream = stream_no;
	s_info->sinfo_ssn = 0;
	s_info->sinfo_flags = flags;
	s_info->sinfo_ppid = ppid;
	s_info->sinfo_context = context;
	s_info->sinfo_assoc_id = 0;
	s_info->sinfo_timetolive = timetolive;
	errno = 0;
	msg.msg_controllen = cmsg->cmsg_len;
	sz = sendmsg(s, &msg, 0);
	return(sz);
}

sctp_assoc_t
sctp_getassocid(int sd, struct sockaddr *sa)
{
	struct sctp_paddrparams sp;
	socklen_t siz;

	/* First get the assoc id */
	siz = sizeof(struct sctp_paddrparams);
	memset(&sp, 0, sizeof(sp));
	memcpy((caddr_t)&sp.spp_address, sa, sa->sa_len);
	errno = 0;
	if (getsockopt2(sd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &sp, &siz) != 0)
		return((sctp_assoc_t)0);
	/* We depend on the fact that 0 can never be returned */
	return(sp.spp_assoc_id);
}



ssize_t
sctp_send(int sd, const void *data, size_t len,
	  const struct sctp_sndrcvinfo *sinfo,
	  int flags)
{
	int sz;
	struct msghdr msg;
	struct iovec iov[2];
	struct sctp_sndrcvinfo *s_info;
	char controlVector[256];
	struct cmsghdr *cmsg;

	iov[0].iov_base = (void *)(unsigned long)data;
	iov[0].iov_len = len;
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)controlVector;
  
	cmsg = (struct cmsghdr *)controlVector;

	cmsg->cmsg_level = IPPROTO_SCTP;
	cmsg->cmsg_type = SCTP_SNDRCV;
	cmsg->cmsg_len = CMSG_LEN (sizeof(struct sctp_sndrcvinfo) );
	s_info = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
	/* copy in the data */
	*s_info = *sinfo;
	errno = 0;
	msg.msg_controllen = cmsg->cmsg_len;
	sz = sendmsg(sd, &msg, flags);
	return(sz);
}


ssize_t
sctp_sendx(int sd, const void *msg, size_t len, 
	   struct sockaddr *addrs, int addrcnt,
	   struct sctp_sndrcvinfo *sinfo,
	   int flags)
{
	int i, ret, cnt, saved_errno;
	int add_len;
	struct sockaddr *at;
	struct sctp_connectx_addrs sca;

	len = 0;
	at = addrs;
	cnt = 0;
	/* validate all the addresses and get the size */
	for (i = 0; i < addrcnt; i++) {
		if (at->sa_family == AF_INET) {
			add_len = sizeof(struct sockaddr_in);
		} else if (at->sa_family == AF_INET6) {
			add_len = sizeof(struct sockaddr_in6);
		} else {
			errno = EINVAL;
			return (-1);
		}
		len += add_len;
		at = (struct sockaddr *)((caddr_t)at + add_len);
		cnt++;
	}
	/* do we have any? */
	if (cnt == 0) {
		errno = EINVAL;
		return(-1);
	}

	sca.cx_num = cnt;
	sca.cx_len = len;
	sca.cx_addrs = addrs;
	ret = ioctl(sd, SIOCCONNECTXDEL, (void *)&sca);
	if (ret != 0) {
		return(ret);
	}
	sinfo->sinfo_assoc_id = sctp_getassocid(sd, addrs);
	if (sinfo->sinfo_assoc_id == 0) {
		printf("Huh, can't get associd? TSNH!\n");
		(void)setsockopt(sd, IPPROTO_SCTP, SCTP_CONNECT_X_COMPLETE, (void *)addrs,
				 (unsigned int)addrs->sa_len);
		errno = ENOENT;
		return (-1);
	}
	ret = sctp_send(sd, msg, len, sinfo, flags);
	saved_errno = errno;
	(void)setsockopt(sd, IPPROTO_SCTP, SCTP_CONNECT_X_COMPLETE, (void *)addrs,
			 (unsigned int)addrs->sa_len);

	errno = saved_errno;
	return (ret);
}

ssize_t
sctp_sendmsgx(int sd, 
	      const void *msg, 
	      size_t len,
	      struct sockaddr *addrs,
	      int addrcnt,
	      u_int32_t ppid,
	      u_int32_t flags,
	      u_int16_t stream_no,
	      u_int32_t timetolive,
	      u_int32_t context)
{
	struct sctp_sndrcvinfo sinfo;
    
	memset((void *) &sinfo, 0, sizeof(struct sctp_sndrcvinfo));
	sinfo.sinfo_ppid       = ppid;
	sinfo.sinfo_flags      = flags;
	sinfo.sinfo_ssn        = stream_no;
	sinfo.sinfo_timetolive = timetolive;
	sinfo.sinfo_context    = context;
	return sctp_sendx(sd, msg, len, addrs, addrcnt, &sinfo, 0);
}

ssize_t
sctp_recvmsg (int s, 
	      void *dbuf, 
	      size_t len,
	      struct sockaddr *from,
	      socklen_t *fromlen,
	      struct sctp_sndrcvinfo *sinfo,
	      int *msg_flags)
{
	struct sctp_sndrcvinfo *s_info;
	ssize_t sz;
	struct msghdr msg;
	struct iovec iov[2];
	char controlVector[2048];
	struct cmsghdr *cmsg;
	iov[0].iov_base = dbuf;
	iov[0].iov_len = len;
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;
	msg.msg_name = (caddr_t)from;
	msg.msg_namelen = *fromlen;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (caddr_t)controlVector;
	msg.msg_controllen = sizeof(controlVector);
	errno = 0;
	sz = recvmsg(s, &msg, 0);

	s_info = NULL;
	len = sz;
	*msg_flags = msg.msg_flags;
	*fromlen = msg.msg_namelen;
	if ((msg.msg_controllen) && sinfo) {
		/* parse through and see if we find
		 * the sctp_sndrcvinfo (if the user wants it).
		 */
		cmsg = (struct cmsghdr *)controlVector;
		while (cmsg) {
			if (cmsg->cmsg_level == IPPROTO_SCTP) {
				if (cmsg->cmsg_type == SCTP_SNDRCV) {
					/* Got it */
					s_info = (struct sctp_sndrcvinfo *)CMSG_DATA(cmsg);
					/* Copy it to the user */
					*sinfo = *s_info;
					break;
				}
			}
			cmsg = CMSG_NXTHDR(&msg, cmsg);
		}
	}
	return(sz);
}

ssize_t 
sctp_recvv(int sd,
    const struct iovec *iov,
    int iovlen,
    struct sockaddr *from,
    socklen_t * fromlen,
    void *info,
    socklen_t * infolen,
    unsigned int *infotype,
    int *flags)
{
	char cmsgbuf[SCTP_CONTROL_VEC_SIZE_RCV];
	struct msghdr msg;
	struct cmsghdr *cmsg;
	ssize_t ret;
	struct sctp_rcvinfo *rcvinfo;
	struct sctp_nxtinfo *nxtinfo;

	if (((info != NULL) && (infolen == NULL)) ||
	    ((info == NULL) && (infolen != NULL) && (*infolen != 0)) ||
	    ((info != NULL) && (infotype == NULL))) {
		errno = EINVAL;
		return (-1);
	}
	if (infotype) {
		*infotype = SCTP_RECVV_NOINFO;
	}
	msg.msg_name = from;
	if (fromlen == NULL) {
		msg.msg_namelen = 0;
	} else {
		msg.msg_namelen = *fromlen;
	}
	msg.msg_iov = __UNCONST(iov);
	msg.msg_iovlen = iovlen;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;
	ret = recvmsg(sd, &msg, *flags);
	*flags = msg.msg_flags;
	if ((ret > 0) &&
	    (msg.msg_controllen > 0) &&
	    (infotype != NULL) &&
	    (infolen != NULL) &&
	    (*infolen > 0)) {
		rcvinfo = NULL;
		nxtinfo = NULL;
		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_level != IPPROTO_SCTP) {
				continue;
			}
			if (cmsg->cmsg_type == SCTP_RCVINFO) {
				rcvinfo = (struct sctp_rcvinfo *)CMSG_DATA(cmsg);
				if (nxtinfo != NULL) {
					break;
				} else {
					continue;
				}
			}
			if (cmsg->cmsg_type == SCTP_NXTINFO) {
				nxtinfo = (struct sctp_nxtinfo *)CMSG_DATA(cmsg);
				if (rcvinfo != NULL) {
					break;
				} else {
					continue;
				}
			}
		}
		if (rcvinfo != NULL) {
			if ((nxtinfo != NULL) && (*infolen >= sizeof(struct sctp_recvv_rn))) {
				struct sctp_recvv_rn *rn_info;

				rn_info = (struct sctp_recvv_rn *)info;
				rn_info->recvv_rcvinfo = *rcvinfo;
				rn_info->recvv_nxtinfo = *nxtinfo;
				*infolen = (socklen_t) sizeof(struct sctp_recvv_rn);
				*infotype = SCTP_RECVV_RN;
			} else if (*infolen >= sizeof(struct sctp_rcvinfo)) {
				memcpy(info, rcvinfo, sizeof(struct sctp_rcvinfo));
				*infolen = (socklen_t) sizeof(struct sctp_rcvinfo);
				*infotype = SCTP_RECVV_RCVINFO;
			}
		} else if (nxtinfo != NULL) {
			if (*infolen >= sizeof(struct sctp_nxtinfo)) {
				memcpy(info, nxtinfo, sizeof(struct sctp_nxtinfo));
				*infolen = (socklen_t) sizeof(struct sctp_nxtinfo);
				*infotype = SCTP_RECVV_NXTINFO;
			}
		}
	}
	return (ret);
}

ssize_t
sctp_sendv(int sd,
    const struct iovec *iov, int iovcnt,
    struct sockaddr *addrs, int addrcnt,
    void *info, socklen_t infolen, unsigned int infotype,
    int flags)
{
	ssize_t ret;
	int i;
	socklen_t addr_len;
	struct msghdr msg;
	in_port_t port;
	struct sctp_sendv_spa *spa_info;
	struct cmsghdr *cmsg;
	char *cmsgbuf;
	struct sockaddr *addr;
	struct sockaddr_in *addr_in;
	struct sockaddr_in6 *addr_in6;
	void *assoc_id_ptr;
	sctp_assoc_t assoc_id;

	if ((addrcnt < 0) ||
	    (iovcnt < 0) ||
	    ((addrs == NULL) && (addrcnt > 0)) ||
	    ((addrs != NULL) && (addrcnt == 0)) ||
	    ((iov == NULL) && (iovcnt > 0)) ||
	    ((iov != NULL) && (iovcnt == 0))) {
		errno = EINVAL;
		return (-1);
	}
	cmsgbuf = malloc(CMSG_SPACE(sizeof(struct sctp_sndinfo)) +
	    CMSG_SPACE(sizeof(struct sctp_prinfo)) +
	    CMSG_SPACE(sizeof(struct sctp_authinfo)) +
	    (size_t)addrcnt * CMSG_SPACE(sizeof(struct in6_addr)));
	if (cmsgbuf == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	assoc_id_ptr = NULL;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = 0;
	cmsg = (struct cmsghdr *)cmsgbuf;
	switch (infotype) {
	case SCTP_SENDV_NOINFO:
		if ((infolen != 0) || (info != NULL)) {
			free(cmsgbuf);
			errno = EINVAL;
			return (-1);
		}
		break;
	case SCTP_SENDV_SNDINFO:
		if ((info == NULL) || (infolen < sizeof(struct sctp_sndinfo))) {
			free(cmsgbuf);
			errno = EINVAL;
			return (-1);
		}
		cmsg->cmsg_level = IPPROTO_SCTP;
		cmsg->cmsg_type = SCTP_SNDINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndinfo));
		memcpy(CMSG_DATA(cmsg), info, sizeof(struct sctp_sndinfo));
		msg.msg_controllen += CMSG_SPACE(sizeof(struct sctp_sndinfo));
		cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct sctp_sndinfo)));
		assoc_id_ptr = &(((struct sctp_sndinfo *)info)->snd_assoc_id);
		break;
	case SCTP_SENDV_PRINFO:
		if ((info == NULL) || (infolen < sizeof(struct sctp_prinfo))) {
			free(cmsgbuf);
			errno = EINVAL;
			return (-1);
		}
		cmsg->cmsg_level = IPPROTO_SCTP;
		cmsg->cmsg_type = SCTP_PRINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_prinfo));
		memcpy(CMSG_DATA(cmsg), info, sizeof(struct sctp_prinfo));
		msg.msg_controllen += CMSG_SPACE(sizeof(struct sctp_prinfo));
		cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct sctp_prinfo)));
		break;
	case SCTP_SENDV_AUTHINFO:
		if ((info == NULL) || (infolen < sizeof(struct sctp_authinfo))) {
			free(cmsgbuf);
			errno = EINVAL;
			return (-1);
		}
		cmsg->cmsg_level = IPPROTO_SCTP;
		cmsg->cmsg_type = SCTP_AUTHINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_authinfo));
		memcpy(CMSG_DATA(cmsg), info, sizeof(struct sctp_authinfo));
		msg.msg_controllen += CMSG_SPACE(sizeof(struct sctp_authinfo));
		cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct sctp_authinfo)));
		break;
	case SCTP_SENDV_SPA:
		if ((info == NULL) || (infolen < sizeof(struct sctp_sendv_spa))) {
			free(cmsgbuf);
			errno = EINVAL;
			return (-1);
		}
		spa_info = (struct sctp_sendv_spa *)info;
		if (spa_info->sendv_flags & SCTP_SEND_SNDINFO_VALID) {
			cmsg->cmsg_level = IPPROTO_SCTP;
			cmsg->cmsg_type = SCTP_SNDINFO;
			cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndinfo));
			memcpy(CMSG_DATA(cmsg), &spa_info->sendv_sndinfo, sizeof(struct sctp_sndinfo));
			msg.msg_controllen += CMSG_SPACE(sizeof(struct sctp_sndinfo));
			cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct sctp_sndinfo)));
			assoc_id_ptr = &(spa_info->sendv_sndinfo.snd_assoc_id);
		}
		if (spa_info->sendv_flags & SCTP_SEND_PRINFO_VALID) {
			cmsg->cmsg_level = IPPROTO_SCTP;
			cmsg->cmsg_type = SCTP_PRINFO;
			cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_prinfo));
			memcpy(CMSG_DATA(cmsg), &spa_info->sendv_prinfo, sizeof(struct sctp_prinfo));
			msg.msg_controllen += CMSG_SPACE(sizeof(struct sctp_prinfo));
			cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct sctp_prinfo)));
		}
		if (spa_info->sendv_flags & SCTP_SEND_AUTHINFO_VALID) {
			cmsg->cmsg_level = IPPROTO_SCTP;
			cmsg->cmsg_type = SCTP_AUTHINFO;
			cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_authinfo));
			memcpy(CMSG_DATA(cmsg), &spa_info->sendv_authinfo, sizeof(struct sctp_authinfo));
			msg.msg_controllen += CMSG_SPACE(sizeof(struct sctp_authinfo));
			cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct sctp_authinfo)));
		}
		break;
	default:
		free(cmsgbuf);
		errno = EINVAL;
		return (-1);
	}
	addr = addrs;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	for (i = 0; i < addrcnt; i++) {
		switch (addr->sa_family) {
		case AF_INET:
			addr_len = (socklen_t) sizeof(struct sockaddr_in);
			addr_in = (struct sockaddr_in *)addr;
			if (addr_in->sin_len != addr_len) {
				free(cmsgbuf);
				errno = EINVAL;
				return (-1);
			}
			if (i == 0) {
				port = addr_in->sin_port;
			} else {
				if (port == addr_in->sin_port) {
					cmsg->cmsg_level = IPPROTO_SCTP;
					cmsg->cmsg_type = SCTP_DSTADDRV4;
					cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_addr));
					memcpy(CMSG_DATA(cmsg), &addr_in->sin_addr, sizeof(struct in_addr));
					msg.msg_controllen += CMSG_SPACE(sizeof(struct in_addr));
					cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct in_addr)));
				} else {
					free(cmsgbuf);
					errno = EINVAL;
					return (-1);
				}
			}
			break;
		case AF_INET6:
			addr_len = (socklen_t) sizeof(struct sockaddr_in6);
			addr_in6 = (struct sockaddr_in6 *)addr;
			if (addr_in6->sin6_len != addr_len) {
				free(cmsgbuf);
				errno = EINVAL;
				return (-1);
			}
			if (i == 0) {
				port = addr_in6->sin6_port;
			} else {
				if (port == addr_in6->sin6_port) {
					cmsg->cmsg_level = IPPROTO_SCTP;
					cmsg->cmsg_type = SCTP_DSTADDRV6;
					cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_addr));
					memcpy(CMSG_DATA(cmsg), &addr_in6->sin6_addr, sizeof(struct in6_addr));
					msg.msg_controllen += CMSG_SPACE(sizeof(struct in6_addr));
					cmsg = (struct cmsghdr *)((caddr_t)cmsg + CMSG_SPACE(sizeof(struct in6_addr)));
				} else {
					free(cmsgbuf);
					errno = EINVAL;
					return (-1);
				}
			}
			break;
		default:
			free(cmsgbuf);
			errno = EINVAL;
			return (-1);
		}
		if (i == 0) {
			msg.msg_name = addr;
			msg.msg_namelen = addr_len;
		}
		addr = (struct sockaddr *)((caddr_t)addr + addr_len);
	}
	if (msg.msg_controllen == 0) {
		msg.msg_control = NULL;
	}
	msg.msg_iov = __UNCONST(iov);
	msg.msg_iovlen = iovcnt;
	msg.msg_flags = 0;
	ret = sendmsg(sd, &msg, flags);
	free(cmsgbuf);
	if ((ret >= 0) && (addrs != NULL) && (assoc_id_ptr != NULL)) {
		assoc_id = sctp_getassocid(sd, addrs);
		memcpy(assoc_id_ptr, &assoc_id, sizeof(assoc_id));
	}
	return (ret);
}

int
sctp_peeloff(int sd, sctp_assoc_t assoc_id)
{
	int ret;
	uint32_t val;

	val = assoc_id;
	ret = ioctl(sd, SIOCPEELOFF, &val);
	if (ret == -1)
		return ret;
	else
		return (int) val;
}

