/*	$NetBSD: puffs_portal.c,v 1.1 2007/07/02 18:35:14 pooka Exp $	*/

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

qelem q;
int readcfg;
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
portal_loopfn(struct puffs_usermount *pu)
{

	if (readcfg)
		conf_read(&q, cfg);
	readcfg = 0;
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
	PUFFSOP_SET(pops, portal, node, inactive);
	PUFFSOP_SET(pops, portal, node, reclaim);

	pu = puffs_init(pops, "portal", NULL, pflags);
	if (pu == NULL)
		err(1, "init");

	if (signal(SIGHUP, sighup) == SIG_ERR)
		warn("cannot set sighup handler");
	readcfg = 0;
	cfg = argv[0];
	if (*cfg != '/')
		errx(1, "need absolute path for config");
	q.q_forw = q.q_back = &q;
	if (conf_read(&q, cfg) == -1)
		err(1, "cannot read cfg \"%s\"", cfg);

	puffs_ml_setloopfn(pu, portal_loopfn);
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
	int fd;

	if (opc == PORTAL_ROOT)
		return 0;

	v = conf_match(&q, portn->path);
	if (v == NULL)
		return ENOENT;

	credtr(&portc, pcr, mode);
	if (activate_argv(&portc, portn->path, v, &fd) != 0)
		return ENOENT;

	portn->fd = fd;

	return 0;
}

int
portal_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf, off_t offset,
	size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct portal_node *portn = opc;
	ssize_t n;

	assert(opc != PORTAL_ROOT);

	n = read(portn->fd, buf, *resid);
	if (n == -1)
		return errno;

	*resid -= n;
	return 0;
}

int
portal_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf, off_t offset,
	size_t *resid, const struct puffs_cred *pcr, int ioflag)
{
	struct portal_node *portn = opc;
	ssize_t n;

	assert(opc != PORTAL_ROOT);

	n = write(portn->fd, buf, *resid);
	if (n == -1)
		return errno;

	*resid -= n;
	return 0;
}

int
portal_node_inactive(struct puffs_cc *pcc, void *opc,
	const struct puffs_cid *pcid)
{

	puffs_setback(pcc, PUFFS_SETBACK_NOREF_N1);
	return 0;
}

int
portal_node_reclaim(struct puffs_cc *pcc, void *opc,
	const struct puffs_cid *pcid)
{
	struct portal_node *portn = opc;

	if (portn->fd != -1)
		close(portn->fd);
	free(portn->path);
	free(portn);

	return 0;
}
