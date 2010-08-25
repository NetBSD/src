/*  $NetBSD: msg.c,v 1.1 2010/08/25 07:18:01 manu Exp $ */

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
#include <sys/socket.h>
#include <sys/un.h>
#include <machine/vmparam.h>

#include "../../lib/libperfuse/perfuse_if.h"
#include "perfused.h"

static int xchg_pb_inloop(struct puffs_usermount *a, struct puffs_framebuf *,
	int, enum perfuse_xchg_pb_reply);
static int xchg_pb_early(struct puffs_usermount *a, struct puffs_framebuf *,
	int, enum perfuse_xchg_pb_reply);

int
perfuse_open_sock(void)
{
	int s;
	struct sockaddr_un sun;
	const struct sockaddr *sa;

	(void)unlink(_PATH_FUSE);

	if ((s = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
		err(EX_OSERR, "socket failed");

	sa = (const struct sockaddr *)(void *)&sun;
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_LOCAL;
	(void)strcpy(sun.sun_path, _PATH_FUSE); 
	
	if (bind(s, sa, (socklen_t )sun.sun_len) == -1)
		err(EX_OSERR, "cannot open \"%s\" socket", _PATH_FUSE);

	if (listen(s, 1) == -1)	
		err(EX_OSERR, "listen failed");

	return s;
}


void *
perfuse_recv_early(fd, len)
	int fd;
	size_t len;
{
	char *buf;

	if (len == 0)
		return NULL;

	if ((buf = malloc(len + 1)) == NULL)
		err(EX_OSERR, "malloc(%d) failed", len);

	if (read(fd, buf, len) != len) {
		DWARN("short read");
		return NULL;
	}

	buf[len] = '\0';
	return buf;
}


perfuse_msg_t *
perfuse_new_pb (pu, opc, opcode, payload_len, cred)
	struct puffs_usermount *pu;
	puffs_cookie_t opc;
	int opcode;
	size_t payload_len;
	const struct puffs_cred *cred;
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
	nodeid = (opc != 0) ? perfuse_get_ino(pu, opc) : PERFUSE_UNKNOWN_INO;

	if (puffs_framebuf_reserve_space(pb, len) != 0)
		DERR(EX_OSERR, "puffs_framebuf_reserve_space failed");

	if (puffs_framebuf_getwindow(pb, 0, &data, &len) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow failed");
	if (len != payload_len + sizeof(*fih))
		DERR(EX_SOFTWARE, "puffs_framebuf_getwindow short len");

	(void)memset(data, 0, len);
	fih = (struct fuse_in_header *)data;
	fih->len = len;
	fih->opcode = opcode;
	fih->unique = perfuse_next_unique(pu);
	fih->nodeid = nodeid;
	fih->uid = (uid_t)-1;
	fih->gid = (gid_t)-1;
	fih->pid = 0;
	if (cred != NULL) {
		(void)puffs_cred_getuid(cred, &fih->uid);
		(void)puffs_cred_getuid(cred, &fih->uid);
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
xchg_pb_early(pu, pb, fd, reply)
	struct puffs_usermount *pu;
	struct puffs_framebuf *pb;
	int fd;
	enum perfuse_xchg_pb_reply reply;
{
	int done;
	int error;

	done = 0;
	while (done == 0) {
		if ((error = perfuse_writeframe(pu, pb, fd, &done)) != 0)
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
		if ((error = perfuse_readframe(pu, pb, fd, &done)) != 0)
			return error;
	}

	return 0;
}

static int
xchg_pb_inloop(pu, pb, fd, reply)
	struct puffs_usermount *pu;
	struct puffs_framebuf *pb;
	int fd;
	enum perfuse_xchg_pb_reply reply;
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
perfuse_xchg_pb(pu, pm, expected_len, reply)
	struct puffs_usermount *pu;
	perfuse_msg_t *pm;
	size_t expected_len;
	enum perfuse_xchg_pb_reply reply;
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

	fih = perfuse_get_inhdr(pm);
	unique_in = fih->unique;
	nodeid = fih->nodeid;
	opcode = fih->opcode;

	if (perfuse_diagflags & PDF_FUSE)
		DPRINTF("> unique = %lld, nodeid = %lld, opcode = %s (%d)\n",
			unique_in, nodeid, perfuse_opname(opcode), opcode);

	if (perfuse_diagflags & PDF_DUMP)
		perfuse_hexdump((char *)fih, fih->len);

#endif /* PERFUSE_DEBUG */

	fd = (int)perfuse_getspecific(pu);

	if (perfuse_inloop(pu))
		error = xchg_pb_inloop(pu, pb, fd, reply);
	else
		error = xchg_pb_early(pu, pb, fd, reply);

	if (error)
		DERR(EX_SOFTWARE, "xchg_pb failed");

	if (reply == no_reply)
		return 0;

	foh = perfuse_get_outhdr((perfuse_msg_t *)(void *)pb);
#ifdef PERFUSE_DEBUG
	unique_out = foh->unique;	

	if (perfuse_diagflags & PDF_FUSE)
		DPRINTF("< unique = %lld, nodeid = %lld, opcode = %s (%d), "
			"error = %d\n", unique_out, nodeid, 
			perfuse_opname(opcode), opcode, error);

	if (perfuse_diagflags & PDF_DUMP)
		perfuse_hexdump((char *)foh, foh->len);

	if (unique_in != unique_out) {
		printf("%s: packet mismatch unique %lld vs %lld\n",
		     __func__, unique_in, unique_out);
		abort();
		errx(EX_SOFTWARE, "%s: packet mismatch unique %lld vs %lld\n",
		     __func__, unique_in, unique_out);
	}
#endif /* PERFUSE_DEBUG */
	
	if ((expected_len != PERFUSE_UNSPEC_REPLY_LEN) &&
	    (foh->len - sizeof(*foh) < expected_len) &&
	    (foh->error == 0)) {
		DERRX(EX_PROTOCOL, 
		     "Unexpected short reply: received %d bytes, expected %d",
		     foh->len - sizeof(*foh), expected_len);
	}

	if ((expected_len != 0) &&
	    (foh->len - sizeof(*foh) > expected_len))
		DWARNX("Unexpected long reply");
	
	/*
	 * Negative Linux errno... 
	 */
	foh->error = -foh->error;

	return foh->error;
}


struct fuse_in_header *
perfuse_get_inhdr(pm)
	perfuse_msg_t *pm;
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
perfuse_get_outhdr(pm)
	perfuse_msg_t *pm;
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
perfuse_get_inpayload(pm)
	perfuse_msg_t *pm;
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
perfuse_get_outpayload(pm)
	perfuse_msg_t *pm;
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
perfuse_readframe(pu, pufbuf, fd, done)
	struct puffs_usermount *pu;
	struct puffs_framebuf *pufbuf;
	int fd;
	int *done;
{
	struct fuse_out_header foh;
	size_t offset;
	size_t remain;
	ssize_t readen;
	void *data;

	offset = puffs_framebuf_telloff(pufbuf);

	/*
	 * Read the header 
	 */
	while (offset < sizeof(foh)) {
		remain = sizeof(foh) - offset;
		PUFFS_FRAMEBUF_GETWINDOW(pufbuf, offset, &data, &remain);

		switch (readen = recv(fd, data, remain, MSG_NOSIGNAL)) {
		case 0:
			DWARNX("%s: recv retunred 0", __func__);
			return ECONNRESET;
			/* NOTREACHED */
			break;
		case -1:
			if (errno == EAGAIN)
				return 0;
			DWARN("%s: recv retunred -1", __func__);
			return errno;
			/* NOTREACHED */
			break;
		default:
			break;
		}

		offset += readen;
		if (puffs_framebuf_seekset(pufbuf, offset) == -1)
			DERR(EX_OSERR, "puffs_framebuf_seekset failed");
	}

	
	/*
	 * We have a header, get remaing length to read
	 */
	if (puffs_framebuf_getdata_atoff(pufbuf, 0, &foh, sizeof(foh)) != 0)
		DERR(EX_SOFTWARE, "puffs_framebuf_getdata_atoff failed");
;
#ifdef PERFUSE_DEBUG
		if (foh.len > FUSE_BUFSIZE)
			DERRX(EX_SOFTWARE, "%s: foh.len = %d (this is huge!)", 
			      __func__, foh.len);
#endif

	/*
	 * If we have only readen the header so far, 
	 * this is time to reserve space.
	 */
	remain = foh.len - offset;
	if (offset == sizeof(foh))
		if (puffs_framebuf_reserve_space(pufbuf, remain) == -1)
			DERR(EX_OSERR, "puffs_framebuf_reserve_space failed");


	/*
	 * And read the remaining data
	 */ 
	while (remain != 0) {
		PUFFS_FRAMEBUF_GETWINDOW(pufbuf, offset, &data, &remain);

		switch (readen = recv(fd, data, remain, MSG_NOSIGNAL)) {
		case 0:
			DWARNX("%s: recv retunred 0", __func__);
			return ECONNRESET;
			/* NOTREACHED */
			break;
		case -1:
			if (errno == EAGAIN)
				return 0;
			DWARN("%s: recv retunred -1", __func__);
			return errno;
			/* NOTREACHED */
			break;
		default:
			break;
		}

		offset += readen;
		remain -= readen;

		if (puffs_framebuf_seekset(pufbuf, offset) == -1)
			DERR(EX_OSERR, "puffs_framebuf_seekset failed");
	}

	*done = 1;
	return 0;
}

/* ARGSUSED0 */
int
perfuse_writeframe(pu, pufbuf, fd, done)
	struct puffs_usermount *pu;
	struct puffs_framebuf *pufbuf;
	int fd;
	int *done;
{
	size_t offset;
	size_t len;
	ssize_t written;
	size_t remain;
	void *data;

	offset = puffs_framebuf_telloff(pufbuf);
	len = puffs_framebuf_tellsize(pufbuf) - offset;
	remain = len;

	while (remain != 0) {
		PUFFS_FRAMEBUF_GETWINDOW(pufbuf, offset, &data, &len);

		switch (written = send(fd, data, remain, MSG_NOSIGNAL)) {
		case 0:
			DWARNX("%s: send retunred 0", __func__);
			return ECONNRESET;
			/* NOTREACHED */
			break;
		case -1:
			if (errno == EAGAIN)
				return 0;
			DWARN("%s: send retunred -1", __func__);
			return errno;
			/* NOTREACHED */
			break;
		default:
			break;
		}

		remain -= written;
		offset += written;

		if (puffs_framebuf_seekset(pufbuf, offset) == -1)
			DERR(EX_OSERR, "puffs_framebuf_seekset failed");
	}

	*done = 1;
	return 0;
}

/* ARGSUSED0 */
int
perfuse_cmpframe(pu, pb1, pb2, match)
	struct puffs_usermount *pu;
	struct puffs_framebuf *pb1;
	struct puffs_framebuf *pb2;
	int *match;
{
	struct fuse_in_header *fih;
	struct fuse_out_header *foh;
	uint64_t unique_in;
	uint64_t unique_out;
	size_t len;

	len = sizeof(*fih);
	PUFFS_FRAMEBUF_GETWINDOW(pb1, 0, (void **)&fih, &len);
	unique_in = fih->unique;

	len = sizeof(*foh);
	PUFFS_FRAMEBUF_GETWINDOW(pb2, 0, (void **)&foh, &len);
	unique_out = foh->unique;

	return unique_in != unique_out;
}

/* ARGSUSED0 */
void
perfuse_gotframe(pu, pb)
	struct puffs_usermount *pu;
	struct puffs_framebuf *pb;
{
	struct fuse_out_header *foh;
	size_t len;

	len = sizeof(*foh);
	PUFFS_FRAMEBUF_GETWINDOW(pb, 0, (void **)&foh, &len);

	DWARNX("Unexpected frame: unique = %lld, error = %d", 
	       foh->unique, foh->error);
#ifdef PERFUSE_DEBUG
	perfuse_hexdump((char *)(void *)foh, foh->len);
#endif

	return;	
}

