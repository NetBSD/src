/*  $NetBSD: msg.c,v 1.21.10.1 2014/08/24 08:42:06 martin Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <paths.h>
#include <puffs.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <machine/vmparam.h>

#include "perfused.h"

static int xchg_pb_inloop(struct puffs_usermount *a, struct puffs_framebuf *,
	int, enum perfuse_xchg_pb_reply);
static int xchg_pb_early(struct puffs_usermount *a, struct puffs_framebuf *,
	int, enum perfuse_xchg_pb_reply);

int
perfused_open_sock(void)
{
	int s;
	struct sockaddr_un sun;
	const struct sockaddr *sa;
	uint32_t opt;
	int sock_type = SOCK_SEQPACKET;

	(void)unlink(_PATH_FUSE);

	/*
	 * Try SOCK_SEQPACKET and fallback to SOCK_DGRAM 
	 * if unavaible
	 */
	if ((s = socket(PF_LOCAL, SOCK_SEQPACKET, 0)) == -1) {
		warnx("SEQPACKET local sockets unavailable, using less "
		      "reliable DGRAM sockets. Expect file operation hangs.");

		sock_type = SOCK_DGRAM;
		if ((s = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1)
			err(EX_OSERR, "socket failed");
	}

	sa = (const struct sockaddr *)(void *)&sun;
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_LOCAL;
	(void)strcpy(sun.sun_path, _PATH_FUSE); 

	/*
	 * Set a buffer lentgh large enough so that a few FUSE packets
	 * will fit. 
	 * XXX We will have to find how many packets we need
	 */
	opt = 4 * FUSE_BUFSIZE;
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	opt = 4 * FUSE_BUFSIZE;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_RCVBUF to %d failed", __func__, opt);

	/*
	 * Request peer credentials
	 */
	opt = 1;
	if (setsockopt(s, 0, LOCAL_CREDS, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt LOCAL_CREDS failed", __func__);
	
	if (bind(s, sa, (socklen_t )sun.sun_len) == -1)
		err(EX_OSERR, "cannot open \"%s\" socket", _PATH_FUSE);

	if (sock_type == SOCK_DGRAM) {
		if (connect(s, sa, (socklen_t )sun.sun_len) == -1)
			err(EX_OSERR, "cannot open \"%s\" socket", _PATH_FUSE);
	}

	return s;
}


void *
perfused_recv_early(int fd, struct sockcred *sockcred, size_t sockcred_len)
{
	struct fuse_out_header foh;
	size_t len;
	char *buf;
	struct msghdr msg;
	char cmsg_buf[sizeof(struct cmsghdr) + SOCKCREDSIZE(NGROUPS_MAX)];
	struct cmsghdr *cmsg = (struct cmsghdr *)(void *)&cmsg_buf;
	struct sockcred *sc = (struct sockcred *)(void *)(cmsg + 1);
	struct iovec iov;
	
	len = sizeof(foh);

	/*
	 * We use the complicated recvmsg because we want peer creds.
	 */
	iov.iov_base = &foh;
	iov.iov_len = len;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg;
	msg.msg_controllen = sizeof(cmsg_buf);
	msg.msg_flags = 0;

	if (recvmsg(fd, &msg, MSG_NOSIGNAL|MSG_PEEK) != (ssize_t)len) {
		DWARN("short recv (header)");
		return NULL;
	}

	if (cmsg->cmsg_type != SCM_CREDS) {
		DWARNX("No SCM_CREDS");
		return NULL;
	}

	if (sockcred != NULL)
		(void)memcpy(sockcred, sc,
			     MIN(cmsg->cmsg_len - sizeof(*cmsg), sockcred_len));
		

	len = foh.len;
	if ((buf = malloc(len)) == NULL)
		err(EX_OSERR, "malloc(%zd) failed", len);

	if (recv(fd, buf, len, MSG_NOSIGNAL) != (ssize_t)len) {
		DWARN("short recv (frame)");
		return NULL;
	}

	return buf;
}


perfuse_msg_t *
perfused_new_pb (struct puffs_usermount *pu, puffs_cookie_t opc, int opcode,
    size_t payload_len, const struct puffs_cred *cred)
{
	struct puffs_framebuf *pb;
	struct fuse_in_header *fih;
	struct puffs_cc *pcc;
	uint64_t nodeid;
	void *data;
	size_t len;

	if ((pb = puffs_framebuf_make()) == NULL)
		DERR(EX_OSERR, "puffs_framebuf_make failed");

	len = payload_len + sizeof(*fih);
	if (opc != 0)
		nodeid = perfuse_get_nodeid(pu, opc);
	else
		nodeid = PERFUSE_UNKNOWN_NODEID;

	if (puffs_framebuf_reserve_space(pb, len) != 0)
		DERR(EX_OSERR, "puffs_framebuf_reserve_space failed");

	if (puffs_framebuf_getwindow(pb, 0, &data, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != payload_len + sizeof(*fih))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short len");

	(void)memset(data, 0, len);
	fih = (struct fuse_in_header *)data;
	fih->len = (uint32_t)len;
	fih->opcode = opcode;
	fih->unique = perfuse_next_unique(pu);
	fih->nodeid = nodeid;
	fih->pid = 0;

	/*	
	 * NULL creds is taken as FUSE root. This currently happens for:
	 * - mount	root cred assumed
	 * - umount	root cred assumed
	 * - inactive	kernel cred used
	 * - statvfs	root cred assumed
	 * - poll	checks have been done at open() time
	 * - addvlock	checks have been done at open() time
	 */
	if ((cred != NULL) && !puffs_cred_isjuggernaut(cred)) {
		if (puffs_cred_getuid(cred, &fih->uid) != 0)
			DERRX(EX_SOFTWARE, "puffs_cred_getuid failed");
		if (puffs_cred_getgid(cred, &fih->gid) != 0)
			DERRX(EX_SOFTWARE, "puffs_cred_getgid failed");
	} else {
		fih->uid = (uid_t)0;
		fih->gid = (gid_t)0;
	}

	if ((pcc = puffs_cc_getcc(pu)) != NULL)
		(void)puffs_cc_getcaller(pcc, (pid_t *)&fih->pid, NULL);

	return (perfuse_msg_t *)(void *)pb;
}

/*
 * framebuf send/receive primitives based on pcc are
 * not available until puffs mainloop is entered.
 * This xchg_pb_inloop() variant allow early communication.
 */
static int
xchg_pb_early(struct puffs_usermount *pu, struct puffs_framebuf *pb, int fd,
    enum perfuse_xchg_pb_reply reply)
{
	int done;
	int error;

	done = 0;
	while (done == 0) {
		if ((error = perfused_writeframe(pu, pb, fd, &done)) != 0)
			return error;
	}

	if (reply == no_reply) {
		puffs_framebuf_destroy(pb);
		return 0;
	} else {
		puffs_framebuf_recycle(pb);
	}

	done = 0;
	while (done == 0) {
		if ((error = perfused_readframe(pu, pb, fd, &done)) != 0)
			return error;
	}

	return 0;
}

static int
xchg_pb_inloop(struct puffs_usermount *pu, struct puffs_framebuf *pb, int fd,
    enum perfuse_xchg_pb_reply reply)
{
	struct puffs_cc *pcc;
	int error;

	if (reply == no_reply) {
		error = puffs_framev_enqueue_justsend(pu, fd, pb, 0, 0);
	} else {
		pcc = puffs_cc_getcc(pu);
		error = puffs_framev_enqueue_cc(pcc, fd, pb, 0);
	}

	return error;
}

int
perfused_xchg_pb(struct puffs_usermount *pu, perfuse_msg_t *pm,
    size_t expected_len, enum perfuse_xchg_pb_reply reply)
{
	struct puffs_framebuf *pb = (struct puffs_framebuf *)(void *)pm;
	int fd;
	int error;
	struct fuse_out_header *foh;
#ifdef PERFUSE_DEBUG
	struct fuse_in_header *fih;
	uint64_t nodeid;
	int opcode;
	uint64_t unique_in;
	uint64_t unique_out;

	fih = perfused_get_inhdr(pm);
	unique_in = fih->unique;
	nodeid = fih->nodeid;
	opcode = fih->opcode;

	if (perfuse_diagflags & PDF_FUSE)
		DPRINTF("> unique = %"PRId64", nodeid = %"PRId64", "
			"opcode = %s (%d)\n",
			unique_in, nodeid, perfuse_opname(opcode), opcode);

	if (perfuse_diagflags & PDF_DUMP)
		perfused_hexdump((char *)fih, fih->len);

#endif /* PERFUSE_DEBUG */

	fd = (int)(intptr_t)perfuse_getspecific(pu);

	if (perfuse_inloop(pu))
		error = xchg_pb_inloop(pu, pb, fd, reply);
	else
		error = xchg_pb_early(pu, pb, fd, reply);

	if (error)
		DERR(EX_SOFTWARE, "xchg_pb failed");

	if (reply == no_reply)
		return 0;

	foh = perfused_get_outhdr((perfuse_msg_t *)(void *)pb);
#ifdef PERFUSE_DEBUG
	unique_out = foh->unique;	

	if (perfuse_diagflags & PDF_FUSE)
		DPRINTF("< unique = %"PRId64", nodeid = %"PRId64", "
			"opcode = %s (%d), "
			"error = %d\n", unique_out, nodeid, 
			perfuse_opname(opcode), opcode, foh->error);

	if (perfuse_diagflags & PDF_DUMP)
		perfused_hexdump((char *)foh, foh->len);

	if (unique_in != unique_out) {
		printf("%s: packet mismatch unique %"PRId64" vs %"PRId64"\n",
		     __func__, unique_in, unique_out);
		abort();
		errx(EX_SOFTWARE, "%s: packet mismatch unique "
		     "%"PRId64" vs %"PRId64"\n",
		     __func__, unique_in, unique_out);
	}
#endif /* PERFUSE_DEBUG */
	
	if ((expected_len != PERFUSE_UNSPEC_REPLY_LEN) &&
	    (foh->len - sizeof(*foh) < expected_len) &&
	    (foh->error == 0)) {
		DERRX(EX_PROTOCOL, 
		     "Unexpected short reply: received %zd bytes, expected %zd",
		     foh->len - sizeof(*foh), expected_len);
	}

	if ((expected_len != 0) &&
	    (foh->len - sizeof(*foh) > expected_len))
		DWARNX("Unexpected long reply");
	
	/*
	 * Negative Linux errno... 
	 */
	if (foh->error <= 0) {
		foh->error = -foh->error;
	} else {
		DWARNX("FUSE resturns positive errno %d", foh->error);
		foh->error = 0;
	}

	return foh->error;
}


struct fuse_in_header *
perfused_get_inhdr(perfuse_msg_t *pm)
{
	struct puffs_framebuf *pb;
	struct fuse_in_header *fih;
	void *hdr;
	size_t len;

	pb = (struct puffs_framebuf *)(void *)pm;
	len = sizeof(*fih);
	if (puffs_framebuf_getwindow(pb, 0, &hdr, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != sizeof(*fih))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short header");
	
	fih = (struct fuse_in_header *)hdr;

	return fih;
}

struct fuse_out_header *
perfused_get_outhdr(perfuse_msg_t *pm)
{
	struct puffs_framebuf *pb;
	struct fuse_out_header *foh;
	void *hdr;
	size_t len;

	pb = (struct puffs_framebuf *)(void *)pm;
	len = sizeof(*foh);
	if (puffs_framebuf_getwindow(pb, 0, &hdr, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != sizeof(*foh))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short header");
	
	foh = (struct fuse_out_header *)hdr;

	return foh;
}

char *
perfused_get_inpayload(perfuse_msg_t *pm)
{
	struct puffs_framebuf *pb;
	struct fuse_in_header *fih;
	void *hdr;
	void *payload;
	size_t len;

	pb = (struct puffs_framebuf *)(void *)pm;
	len = sizeof(*fih);
	if (puffs_framebuf_getwindow(pb, 0, &hdr, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != sizeof(*fih))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short header");
	
	fih = (struct fuse_in_header *)hdr;

	len = fih->len - sizeof(*fih);
	if (puffs_framebuf_getwindow(pb, sizeof(*fih), &payload, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != fih->len - sizeof(*fih))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short header");

	return (char *)payload;
}

char *
perfused_get_outpayload(perfuse_msg_t *pm)
{
	struct puffs_framebuf *pb;
	struct fuse_out_header *foh;
	void *hdr;
	void *payload;
	size_t len;

	pb = (struct puffs_framebuf *)(void *)pm;
	len = sizeof(*foh);
	if (puffs_framebuf_getwindow(pb, 0, &hdr, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != sizeof(*foh))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short header");
	
	foh = (struct fuse_out_header *)hdr;

	len = foh->len - sizeof(*foh);
	if (puffs_framebuf_getwindow(pb, sizeof(*foh), &payload, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != foh->len - sizeof(*foh))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short header");

	return (char *)payload;
}

#define PUFFS_FRAMEBUF_GETWINDOW(pb, offset, data, len) 		     \
	do {								     \
		int pfg_error;						     \
		size_t pfg_len = *(len);				     \
									     \
		pfg_error = puffs_framebuf_getwindow(pb, offset, data, len); \
		if (pfg_error != 0)					     \
			DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");\
									     \
		if (*(len) != pfg_len)					     \
			DERRX(EX_SOFTWARE, "puffs_framebuf_getwindow size"); \
	} while (0 /* CONSTCOND */);

/* ARGSUSED0 */
int
perfused_readframe(struct puffs_usermount *pu, struct puffs_framebuf *pufbuf,
    int fd, int *done)
{
	struct fuse_out_header foh;
	size_t len;
	ssize_t readen;
	void *data;

	/*
	 * Read the header 
	 */
	len = sizeof(foh);
	PUFFS_FRAMEBUF_GETWINDOW(pufbuf, 0, &data, &len);

	switch (readen = recv(fd, data, len, MSG_NOSIGNAL|MSG_PEEK)) {
	case 0:
		DPRINTF("Filesystem exit\n");
		/* NOTREACHED */
		exit(0);
		break;
	case -1:
		if (errno == EAGAIN)
			return 0;
		DWARN("%s: recv returned -1", __func__);
		return errno;
		/* NOTREACHED */
		break;
	default:
		break;
	}

#ifdef PERFUSE_DEBUG
	if (readen != (ssize_t)len)
		DERRX(EX_SOFTWARE, "%s: short recv %zd/%zd",
		      __func__, readen, len);
#endif

	/*
	 * We have a header, get remaing length to read
	 */
	if (puffs_framebuf_getdata_atoff(pufbuf, 0, &foh, sizeof(foh)) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getdata_atoff failed");

	len = foh.len;

#ifdef PERFUSE_DEBUG
	if (len > (size_t)FUSE_BUFSIZE)
		DERRX(EX_SOFTWARE, "%s: foh.len = %zu", __func__, len);
#endif

	/*
	 * This is time to reserve space.
	 */
	if (puffs_framebuf_reserve_space(pufbuf, len) == -1)
		DERR(EX_OSERR, "puffs_framebuf_reserve_space failed");

	/*
	 * And read the remaining data
	 */ 
	PUFFS_FRAMEBUF_GETWINDOW(pufbuf, 0, &data, &len);

	switch (readen = recv(fd, data, len, MSG_NOSIGNAL)) {
	case 0:
		DWARNX("%s: recv returned 0", __func__);
		perfused_panic();
	case -1:
		if (errno == EAGAIN)
			return 0;
		DWARN("%s: recv returned -1", __func__);
		return errno;
	default:
		break;
	}

#ifdef PERFUSE_DEBUG
	if (readen != (ssize_t)len)
		DERRX(EX_SOFTWARE, "%s: short recv %zd/%zd",
		      __func__, readen, len);
#endif

	*done = 1;
	return 0;
}

/* ARGSUSED0 */
int
perfused_writeframe(struct puffs_usermount *pu, struct puffs_framebuf *pufbuf,
    int fd, int *done)
{
	size_t len;
	ssize_t written;
	void *data;

	len = puffs_framebuf_tellsize(pufbuf);
	PUFFS_FRAMEBUF_GETWINDOW(pufbuf, 0, &data, &len);

	switch (written = send(fd, data, len, MSG_NOSIGNAL)) {
	case 0:
#ifdef PERFUSE_DEBUG
		DERRX(EX_SOFTWARE, "%s: send returned 0", __func__);
#else
		return ECONNRESET;
#endif
		/* NOTREACHED */
		break;
	case -1:
		DWARN("%s: send returned -1, errno = %d", __func__, errno);
		switch(errno) {
		case EAGAIN:
		case ENOBUFS:
		case EMSGSIZE:
			return 0;
			break;
		case EPIPE:
			perfused_panic();
			/* NOTREACHED */
			break;
		default:
			return errno;
			break;
		}
		/* NOTREACHED */
		break;
	default:
		break;
	}

#ifdef PERFUSE_DEBUG
	if (written != (ssize_t)len)
		DERRX(EX_SOFTWARE, "%s: short send %zd/%zd",
		      __func__, written, len);
#endif

	*done = 1;
	return 0;
}

/* ARGSUSED0 */
int
perfused_cmpframe(struct puffs_usermount *pu, struct puffs_framebuf *pb1,
    struct puffs_framebuf *pb2, int *match)
{
	struct fuse_in_header *fih;
	struct fuse_out_header *foh;
	uint64_t unique_in;
	uint64_t unique_out;
	size_t len;

	len = sizeof(*fih);
	PUFFS_FRAMEBUF_GETWINDOW(pb1, 0, (void **)(void *)&fih, &len);
	unique_in = fih->unique;

	len = sizeof(*foh);
	PUFFS_FRAMEBUF_GETWINDOW(pb2, 0, (void **)(void *)&foh, &len);
	unique_out = foh->unique;

	return unique_in != unique_out;
}

/* ARGSUSED0 */
void
perfused_gotframe(struct puffs_usermount *pu, struct puffs_framebuf *pb)
{
	struct fuse_out_header *foh;
	size_t len;

	len = sizeof(*foh);
	PUFFS_FRAMEBUF_GETWINDOW(pb, 0, (void **)(void *)&foh, &len);

	DWARNX("Unexpected frame: unique = %"PRId64", error = %d", 
	       foh->unique, foh->error);
#ifdef PERFUSE_DEBUG
	perfused_hexdump((char *)(void *)foh, foh->len);
#endif

	return;	
}

void
perfused_fdnotify(struct puffs_usermount *pu, int fd, int what)
{
	if (fd != (int)(intptr_t)perfuse_getspecific(pu))
		DERRX(EX_SOFTWARE, "%s: unexpected notification for fd = %d",
		      __func__, fd); 

	if ((what != PUFFS_FBIO_READ) && (what != PUFFS_FBIO_WRITE))
		DERRX(EX_SOFTWARE, "%s: unexpected notification what = 0x%x",
		      __func__, what); 

	if (perfuse_unmount(pu) != 0)
		DWARN("unmount() failed");

	if (shutdown(fd, SHUT_RDWR) != 0)
		DWARN("shutdown() failed");

	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("Exit");
	
	exit(0);
}

void
perfused_umount(struct puffs_usermount *pu)
{
	int fd;

	fd = (int)(intptr_t)perfuse_getspecific(pu);

	if (shutdown(fd, SHUT_RDWR) != 0)
		DWARN("shutdown() failed");

	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("unmount");

	return;
}
