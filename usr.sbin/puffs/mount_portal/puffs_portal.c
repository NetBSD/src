/*	$NetBSD: puffs_portal.c,v 1.5 2007/07/09 09:28:21 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
 * Development was supported by the Finnish Cultural Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <mntopts.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "portald.h"

struct portal_node {
	char *path;
	int fd;
};

static void usage(void);

PUFFSOP_PROTOS(portal);

#define PORTAL_ROOT NULL
#define METADATASIZE (sizeof(int) + sizeof(size_t))

qelem q;
int readcfg, sigchild;
const char *cfg;

static void
usage()
{

	errx(1, "usage: %s [-o options] /path/portal.conf mount_point",
	    getprogname());
}

static void
sighup(int sig)
{

	readcfg = 1;
}

static void
sigcry(int sig)
{

	sigchild = 1;
}

static void
portal_loopfn(struct puffs_usermount *pu)
{

	if (readcfg)
		conf_read(&q, cfg);
	readcfg = 0;

	if (sigchild) {
		sigchild = 0;
		while (waitpid(-1, NULL, WNOHANG) != -1)
			continue;
	}
}

#define PUFBUF_FD	1
#define PUFBUF_DATA	2

#define CMSIZE (sizeof(struct cmsghdr) + sizeof(int))

/* receive file descriptor produced by our child process */
static int
readfd(struct puffs_framebuf *pufbuf, int fd, int *done)
{
	struct cmsghdr *cmp;
	struct msghdr msg;
	struct iovec iov;
	ssize_t n;
	int error, rv;

	rv = 0;
	cmp = emalloc(CMSG_LEN(sizeof(int)));

	iov.iov_base = &error;
	iov.iov_len = sizeof(int);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_control = cmp;
	msg.msg_controllen = CMSG_LEN(sizeof(int));

	n = recvmsg(fd, &msg, 0);
	if (n == -1) {
		rv = errno;
		goto out;
	}
	if (n == 0) {
		rv = ECONNRESET;
		goto out;
	}

	/* the data for the server */
	puffs_framebuf_putdata_atoff(pufbuf, 0, &error, sizeof(int));
	puffs_framebuf_putdata_atoff(pufbuf, sizeof(int),
	    CMSG_DATA(cmp), sizeof(int));
	*done = 1;

 out:
	free(cmp);
	return rv;
}

/*
 * receive data from provider
 *
 * XXX: should read directly into the buffer and adjust offsets
 * instead of doing memcpy
 */
static int
readdata(struct puffs_framebuf *pufbuf, int fd, int *done)
{
	char buf[1024];
	size_t max;
	ssize_t n;
	size_t moved;

	/* don't override metadata */
	if (puffs_framebuf_telloff(pufbuf) == 0)
		puffs_framebuf_seekset(pufbuf, METADATASIZE);
	puffs_framebuf_getdata_atoff(pufbuf, sizeof(int), &max, sizeof(size_t));
	moved = puffs_framebuf_tellsize(pufbuf) - METADATASIZE;
	assert(max >= moved);
	max -= moved;

	do {
		n = read(fd, buf, MIN(sizeof(buf), max));
		if (n == 0) {
			if (moved)
				break;
			else
				return -1; /* caught by read */
		}
		if (n < 0) {
			if (moved)
				return 0;

			if (errno != EAGAIN)
				return errno;
			else
				return 0;
		}

		puffs_framebuf_putdata(pufbuf, buf, n);
		moved += n;
		max -= n;
	} while (max > 0);

	*done = 1;

	return 0;
}

static int
portal_frame_rf(struct puffs_usermount *pu, struct puffs_framebuf *pufbuf,
	int fd, int *done)
{
	int type;

	if (puffs_framebuf_getdata_atoff(pufbuf, 0, &type, sizeof(int)) == -1)
		return EINVAL;

	if (type == PUFBUF_FD)
		return readfd(pufbuf, fd, done);
	else if (type == PUFBUF_DATA)
		return readdata(pufbuf, fd, done);
	else
		abort();
}

static int
portal_frame_wf(struct puffs_usermount *pu, struct puffs_framebuf *pufbuf,
	int fd, int *done)
{
	void *win;
	size_t pbsize, pboff, winlen;
	ssize_t n;
	int error;

	pboff = puffs_framebuf_telloff(pufbuf);
	pbsize = puffs_framebuf_tellsize(pufbuf);
	error = 0;

	do {
		assert(pbsize > pboff);
		winlen = pbsize - pboff;
		if (puffs_framebuf_getwindow(pufbuf, pboff, &win, &winlen)==-1)
			return errno;
		n = write(fd, win, winlen);
		if (n == 0) {
			if (pboff != 0)
				break;
			else
				return -1; /* caught by node_write */
		}
		if (n < 0) {
			if (pboff != 0)
				break;

			if (errno != EAGAIN)
				return errno;
			return 0;
		}

		pboff += n;
		puffs_framebuf_seekset(pufbuf, pboff);
	} while (pboff != pbsize);

	*done = 1;
	puffs_framebuf_putdata_atoff(pufbuf, 0, &pboff, sizeof(size_t));
	return error;
}

/* transfer file descriptor to master file server */
static int
sendfd(int s, int fd, int error)
{
	struct cmsghdr *cmp;
	struct msghdr msg;
	struct iovec iov;
	ssize_t n;
	int rv;

	rv = 0;
	cmp = emalloc(CMSG_LEN(sizeof(int)));

	iov.iov_base = &error;
	iov.iov_len = sizeof(int);

	cmp->cmsg_level = SOL_SOCKET;
	cmp->cmsg_type = SCM_RIGHTS;
	cmp->cmsg_len = CMSG_LEN(sizeof(int));

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_control = cmp;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	*(int *)CMSG_DATA(cmp) = fd;

	n = sendmsg(s, &msg, 0);
	if (n == -1)
		rv = errno;
	else if (n < sizeof(int))
		rv = EPROTO;

	free(cmp);
	return rv;
}

/*
 * Produce I/O file descriptor by forking (like original portald).
 * 
 * child: run provider and transfer produced fd to parent
 * parent: yield until child produces fd.  receive it and store it.
 */
static int
provide(struct puffs_cc *pcc, struct portal_node *portn,
	struct portal_cred *portc, char **v)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_framebuf *pufbuf;
	int s[2];
	int fd, error;
	int data;

	pufbuf = puffs_framebuf_make();
	if (pufbuf == NULL)
		return ENOMEM;

	data = PUFBUF_FD;
	if (puffs_framebuf_putdata(pufbuf, &data, sizeof(int)) == -1)
		return errno;

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, s) == -1) {
		puffs_framebuf_destroy(pufbuf);
		return errno;
	}

	switch (fork()) {
	case -1:
		return errno;
	case 0:
		error = activate_argv(portc, portn->path, v, &fd);
		sendfd(s[1], fd, error);
		exit(0);
	default:
		puffs_framev_addfd(pu, s[0], PUFFS_FBIO_READ);
		puffs_framev_enqueue_directreceive(pcc, s[0], pufbuf, 0);
		puffs_framev_removefd(pu, s[0], 0);
		close(s[0]);
		close(s[1]);
		if (puffs_framebuf_tellsize(pufbuf) != 2*sizeof(int)) {
			puffs_framebuf_destroy(pufbuf);
			return EIO;
		}

		puffs_framebuf_getdata_atoff(pufbuf, 0, &error, sizeof(int));
		puffs_framebuf_getdata_atoff(pufbuf, sizeof(int),
		    &fd, sizeof(int));
		puffs_framebuf_destroy(pufbuf);

		if (error)
			return error;

		data = 1;
		if (ioctl(fd, FIONBIO, &data) == -1)
			return errno;

		if (puffs_framev_addfd(pu, fd, PUFFS_FBIO_WRITE) == -1)
			return errno;

		portn->fd = fd;
		return 0;
	}
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	mntoptparse_t mp;
	int pflags, lflags, mntflags;
	int ch;

	setprogname(argv[0]);

	lflags = mntflags = pflags = 0;
	while ((ch = getopt(argc, argv, "o:s")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 's': /* stay on top */
			lflags |= PUFFSLOOP_NODAEMON;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	pflags |= PUFFS_KFLAG_NOCACHE | PUFFS_KFLAG_LOOKUP_FULLPNBUF;
	if (pflags & PUFFS_FLAG_OPDUMP)
		lflags |= PUFFSLOOP_NODAEMON;
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	PUFFSOP_INIT(pops);

	PUFFSOP_SETFSNOP(pops, unmount);
	PUFFSOP_SETFSNOP(pops, sync);
	PUFFSOP_SETFSNOP(pops, statvfs);

	PUFFSOP_SET(pops, portal, node, lookup);
	PUFFSOP_SET(pops, portal, node, getattr);
	PUFFSOP_SET(pops, portal, node, setattr);
	PUFFSOP_SET(pops, portal, node, open);
	PUFFSOP_SET(pops, portal, node, read);
	PUFFSOP_SET(pops, portal, node, write);
	PUFFSOP_SET(pops, portal, node, seek);
	PUFFSOP_SET(pops, portal, node, inactive);
	PUFFSOP_SET(pops, portal, node, reclaim);

	pu = puffs_init(pops, "portal", NULL, pflags);
	if (pu == NULL)
		err(1, "init");

	if (signal(SIGHUP, sighup) == SIG_ERR)
		warn("cannot set sighup handler");
	if (signal(SIGCHLD, sigcry) == SIG_ERR)
		err(1, "cannot set sigchild handler");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		err(1, "cannot ignore sigpipe");

	readcfg = 0;
	cfg = argv[0];
	if (*cfg != '/')
		errx(1, "need absolute path for config");
	q.q_forw = q.q_back = &q;
	if (conf_read(&q, cfg) == -1)
		err(1, "cannot read cfg \"%s\"", cfg);

	puffs_ml_setloopfn(pu, portal_loopfn);
	puffs_framev_init(pu, portal_frame_rf, portal_frame_wf, NULL, NULL);
	if (puffs_mount(pu,  argv[1], mntflags, PORTAL_ROOT) == -1)
		err(1, "mount");
	if (puffs_mainloop(pu, lflags) == -1)
		err(1, "mainloop");

	return 0;
}

static struct portal_node *
makenode(const char *path)
{
	struct portal_node *portn;

	portn = emalloc(sizeof(struct portal_node));
	portn->path = estrdup(path);
	portn->fd = -1;

	return portn;
}

static void
credtr(struct portal_cred *portc, const struct puffs_cred *puffc, int mode)
{
	memset(portc, 0, sizeof(struct portal_cred));

	portc->pcr_flag = mode;
	puffs_cred_getuid(puffc, &portc->pcr_uid);
	puffs_cred_getgid(puffc, &portc->pcr_gid);
	puffs_cred_getgroups(puffc, portc->pcr_groups,
	    (short *)&portc->pcr_ngroups);
}

/*
 * XXX: we could also simply already resolve the name at this stage
 * instead of deferring it to open.  But doing it in open is how the
 * original portald does it, and I don't want to introduce any funny
 * incompatibilities.
 */
int
portal_node_lookup(struct puffs_cc *pcc, void *opc, struct puffs_newinfo *pni,
	const struct puffs_cn *pcn)
{
	struct portal_node *portn;

	assert(opc == PORTAL_ROOT);

	if (pcn->pcn_nameiop != PUFFSLOOKUP_LOOKUP
	    && pcn->pcn_nameiop != PUFFSLOOKUP_CREATE)
		return EOPNOTSUPP;

	portn = makenode(pcn->pcn_name);
	puffs_newinfo_setcookie(pni, portn);
	puffs_newinfo_setvtype(pni, VREG);

	pcn->pcn_flags &= ~PUFFSLOOKUP_REQUIREDIR;
	pcn->pcn_consume = strlen(pcn->pcn_name) - pcn->pcn_namelen;

	return 0;
}

int fakeid = 3;

/* XXX: libpuffs'ize */
int
portal_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *va,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{
	struct timeval tv;
	struct timespec ts;

	puffs_vattr_null(va);
	if (opc == PORTAL_ROOT) {
		va->va_type = VDIR;
		va->va_mode = 0777;
		va->va_nlink = 2;
	} else {
		va->va_type = VREG;
		va->va_mode = 0666;
		va->va_nlink = 1;
	}
	va->va_uid = va->va_gid = 0;
	va->va_fileid = fakeid++;
	va->va_size = va->va_bytes = 0;
	va->va_gen = 0;
	va->va_rdev = PUFFS_VNOVAL;
	va->va_blocksize = DEV_BSIZE;

	gettimeofday(&tv, NULL);
	TIMEVAL_TO_TIMESPEC(&tv, &ts);
	va->va_atime = va->va_ctime = va->va_mtime = va->va_birthtime = ts;

	return 0;
}

/* for writing, just pretend we care */
int
portal_node_setattr(struct puffs_cc *pcc, void *opc, const struct vattr *va,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{

	return 0;
}

int
portal_node_open(struct puffs_cc *pcc, void *opc, int mode,
	const struct puffs_cred *pcr, const struct puffs_cid *pcid)
{
	struct portal_node *portn = opc;
	struct portal_cred portc;
	char **v;

	if (opc == PORTAL_ROOT)
		return 0;

	if (mode & O_NONBLOCK)
		return EOPNOTSUPP;

	v = conf_match(&q, portn->path);
	if (v == NULL)
		return ENOENT;

	credtr(&portc, pcr, mode);
	return provide(pcc, portn, &portc, v);
}

int
portal_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf, off_t offset,
	size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct portal_node *portn = opc;
	struct puffs_framebuf *pufbuf;
	size_t xfersize, winsize, boff;
	void *win;
	int rv, error;
	int data, dummy;

	assert(opc != PORTAL_ROOT);
	error = 0;

	/* if we can't (re-)enable it, treat it as EOF */
	rv = puffs_framev_enablefd(pu, portn->fd, PUFFS_FBIO_READ);
	if (rv == -1)
		return 0;

	pufbuf = puffs_framebuf_make();
	data = PUFBUF_DATA;
	puffs_framebuf_putdata(pufbuf, &data, sizeof(int));
	puffs_framebuf_putdata(pufbuf, resid, sizeof(size_t));

	/* if we are doing nodelay, do read directly */
	if (ioflag & PUFFS_IO_NDELAY) {
		rv = readdata(pufbuf, portn->fd, &dummy);
		if (rv != 0) {
			error = rv;
			goto out;
		}
	} else {
		rv = puffs_framev_enqueue_directreceive(pcc,
		    portn->fd, pufbuf, 0);

		if (rv == -1) {
			error = errno;
			goto out;
		}
	}

	xfersize = puffs_framebuf_tellsize(pufbuf) - METADATASIZE;
	if (xfersize == 0) {
		assert(ioflag & PUFFS_IO_NDELAY);
		error = EAGAIN;
		goto out;
	}

	*resid -= xfersize;
	boff = 0;
	while (xfersize > 0) {
		winsize = xfersize;
		rv = puffs_framebuf_getwindow(pufbuf, METADATASIZE,
		    &win, &winsize);
		assert(rv == 0);
		assert(winsize > 0);

		memcpy(buf + boff, win, winsize);
		xfersize -= winsize;
		boff += winsize;
	}

 out:
	puffs_framev_disablefd(pu, portn->fd, PUFFS_FBIO_READ);
	puffs_framebuf_destroy(pufbuf);

	/* a trickery, from readdata() */
	if (error == -1)
		return 0;
	return error;
}

int
portal_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf, off_t offset,
	size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct portal_node *portn = opc;
	struct puffs_framebuf *pufbuf;
	size_t written;
	int error, rv, dummy;

	assert(opc != PORTAL_ROOT);

	pufbuf = puffs_framebuf_make();
	puffs_framebuf_putdata(pufbuf, buf, *resid);

	error = 0;
	if (ioflag & PUFFS_IO_NDELAY) {
		rv = portal_frame_wf(puffs_cc_getusermount(pcc),
		    pufbuf, portn->fd, &dummy);
		if (rv) {
			error = rv;
			goto out;
		}
	} else  {
		rv = puffs_framev_enqueue_directsend(pcc, portn->fd, pufbuf, 0);
		if (rv == -1) {
			error = errno;
			goto out;
		}
	}

	rv = puffs_framebuf_getdata_atoff(pufbuf, 0, &written, sizeof(size_t));
	assert(rv == 0);
	assert(written <= *resid);
	*resid -= written;

 out:
	puffs_framebuf_destroy(pufbuf);
	if (error == -1)
		error = 0;
	return 0;
}

int
portal_node_seek(struct puffs_cc *pcc, void *opc, off_t oldoff, off_t newoff,
	const struct puffs_cred *pcr)
{
	struct portal_node *portn = opc;

	if (opc == PORTAL_ROOT || portn->fd == -1)
		return EOPNOTSUPP;

	if (lseek(portn->fd, newoff, SEEK_SET) == -1)
		return errno;
	return 0;
}

int
portal_node_inactive(struct puffs_cc *pcc, void *opc,
	const struct puffs_cid *pcid)
{

	if (opc == PORTAL_ROOT)
		return 0;

	puffs_setback(pcc, PUFFS_SETBACK_NOREF_N1);
	return 0;
}

int
portal_node_reclaim(struct puffs_cc *pcc, void *opc,
	const struct puffs_cid *pcid)
{
	struct portal_node *portn = opc;

	if (portn->fd != -1) {
		puffs_framev_removefd(puffs_cc_getusermount(pcc), portn->fd, 0);
		close(portn->fd);
	}
	free(portn->path);
	free(portn);

	return 0;
}
