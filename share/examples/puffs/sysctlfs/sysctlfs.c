/*	$NetBSD: sysctlfs.c,v 1.4 2006/11/20 00:04:05 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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

/*
 * sysctlfs: mount sysctls as a file system tree
 *
 * ro for now
 *
 * XXXX: this is a very quick hack fs.  it's not even complete,
 * please don't use it as an example.  actually, this code is so bad that
 * it's nearly a laugh, it's nearly a laugh, but it's really a cry
 *
 * more important XXX: need to implement features into puffs so that I
 * can say "please don't cache my data"
 *
 * and The Final Cut: find /sysctl doesn't traverse the directories
 * correctly, but find -H /sysctl does.  *dumbfounded*.
 */

#include <sys/types.h>
#include <sys/sysctl.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

PUFFSVFS_PROTOS(sysctlfs)
PUFFSVN_PROTOS(sysctlfs)

#define N_HIERARCHY 16
struct sfsnode {
	struct sysctlnode sctln;
	struct sfsnode *dotdot;
	int name[N_HIERARCHY];
	size_t hierlen;
	ino_t myid;
	int refcount;
};

struct sfsnode rn;
struct timespec fstime;

ino_t nextid = 3;

#define ISADIR(a) ((SYSCTL_TYPE(a->sctln.sysctl_flags) == CTLTYPE_NODE))
#define SFS_MAXFILE 8192

int
main(int argc, char *argv[])
{
	struct puffs_usermount *pu;
	struct puffs_vfsops pvfs;
	struct puffs_vnops pvn;

	setprogname(argv[0]);

	if (argc < 2)
		errx(1, "usage: %s mountpath\n", getprogname());

	memset(&pvfs, 0, sizeof(struct puffs_vfsops));
	memset(&pvn, 0, sizeof(struct puffs_vnops));

	pvfs.puffs_mount = sysctlfs_mount;
	pvfs.puffs_unmount = puffs_vfsnop_unmount;
	pvfs.puffs_sync = puffs_vfsnop_sync;
	pvfs.puffs_statvfs = puffs_vfsnop_statvfs;

	pvn.puffs_lookup = sysctlfs_lookup;
	pvn.puffs_getattr = sysctlfs_getattr;
	pvn.puffs_readdir = sysctlfs_readdir;
	pvn.puffs_read = sysctlfs_read;
	pvn.puffs_write = sysctlfs_write;
	pvn.puffs_reclaim = sysctlfs_reclaim;

	if ((pu = puffs_mount(&pvfs, &pvn, argv[1], 0, "sysctlfs",
	    PUFFSFLAG_NOCACHE, 0)) == NULL)
		err(1, "mount");

	if (puffs_mainloop(pu) == -1)
		err(1, "mainloop");

	return 0;
}

int
sysctlfs_mount(struct puffs_usermount *pu, void **rootcookie)
{
	struct timeval tv_now;

	rn.dotdot = NULL;
	rn.hierlen = 0;
	rn.myid = 2;
	rn.sctln.sysctl_flags = CTLTYPE_NODE;
	rn.refcount = 2;
	*rootcookie = &rn;

	gettimeofday(&tv_now, NULL);
	TIMEVAL_TO_TIMESPEC(&tv_now, &fstime);

	return 0;
}

static void
doprint(struct sfsnode *sfs, char *buf, size_t bufsize)
{
	size_t sz;

	assert(!ISADIR(sfs));

	memset(buf, 0, bufsize);
	switch (SYSCTL_TYPE(sfs->sctln.sysctl_flags)) {
	case CTLTYPE_INT: {
		int i;
		sz = sizeof(int);
		if (sysctl(sfs->name, sfs->hierlen, &i, &sz,
		    NULL, 0) == -1)
			break;
		snprintf(buf, bufsize, "%d", i);
		break;
	}
	case CTLTYPE_QUAD: {
		quad_t q;
		sz = sizeof(q);
		if (sysctl(sfs->name, sfs->hierlen, &q, &sz, NULL, 0) == -1)
			break;
		snprintf(buf, bufsize, "%" PRId64, q);
		break;
	}
	case CTLTYPE_STRUCT:
		snprintf(buf, bufsize, "CTLTYPE_STRUCT: implement me and "
		    "score a cookie");
		break;
	case CTLTYPE_STRING: {
		sz = bufsize;
		if (sysctl(sfs->name, sfs->hierlen, buf, &sz,
		    NULL, 0) == -1)
			break;
		break;
	}
	default:
		snprintf(buf, bufsize, "invalid sysctl CTLTYPE");
		break;
	}
	if (buf[0] != 0)
		strlcat(buf, "\n", bufsize);
}

static int
getlinks(struct sfsnode *sfs)
{
	struct sysctlnode sn[128];
	struct sysctlnode qnode;
	size_t sl;

	if (!ISADIR(sfs))
		return 1;

	memset(&qnode, 0, sizeof(qnode));
	sl = 128 * sizeof(struct sysctlnode);
	qnode.sysctl_flags = SYSCTL_VERSION;
	sfs->name[sfs->hierlen] = CTL_QUERY;

	if (sysctl(sfs->name, sfs->hierlen + 1, sn, &sl,
	    &qnode, sizeof(qnode)) == -1)
		return 0;

	return (sl / sizeof(sn[0])) + 2;
}

static int
getsize(struct sfsnode *sfs)
{
	char buf[SFS_MAXFILE];

	if (ISADIR(sfs))
		return getlinks(sfs) * 16; /* totally arbitrary */

	doprint(sfs, buf, sizeof(buf));
	return strlen(buf);
}

/* fast & loose */
int
sysctlfs_lookup(struct puffs_usermount *pu, void *opc, void **newnode,
	enum vtype *newtype, voff_t *newsize, dev_t *newrdev,
	const struct puffs_cn *pcn)
{
	struct sysctlnode sn[128];
	struct sysctlnode qnode;
	struct sfsnode *sfs_dir = opc;
	struct sfsnode *sfs_new;
	size_t sl;
	int i;

	assert(ISADIR(sfs_dir));

	if (pcn->pcn_flags & PUFFS_ISDOTDOT) {
		*newnode = sfs_dir->dotdot;
		*newtype = VDIR;
		return 0;
	}

	/* get all and compare.. yes, this is a tad silly, but ... */
	if (sfs_dir->hierlen == N_HIERARCHY)
		return ENOMEM; /* just something */

	memset(&qnode, 0, sizeof(qnode));
	sl = 128 * sizeof(struct sysctlnode);
	qnode.sysctl_flags = SYSCTL_VERSION;
	sfs_dir->name[sfs_dir->hierlen] = CTL_QUERY;

	if (sysctl(sfs_dir->name, sfs_dir->hierlen + 1, sn, &sl,
	    &qnode, sizeof(qnode)) == -1)
		return ENOENT;

	for (i = 0; i < sl / sizeof(struct sysctlnode); i++)
		if (strcmp(sn[i].sysctl_name, pcn->pcn_name) == 0)
			goto found;
	return ENOENT;

 found:
	sfs_new = emalloc(sizeof(struct sfsnode));	
	sfs_new->sctln = sn[i];
	sfs_new->dotdot = sfs_dir;
	sfs_dir->refcount++;
	memcpy(sfs_new->name, sfs_dir->name, sfs_dir->hierlen * sizeof(int));
	sfs_new->hierlen = sfs_dir->hierlen + 1;
	sfs_new->name[sfs_dir->hierlen] = sn[i].sysctl_num;
	sfs_new->myid = nextid++;

	*newnode = sfs_new;
	if (ISADIR(sfs_new)) {
		*newtype = VDIR;
		*newsize = 0;
	} else {
		*newtype = VREG;
		*newsize = getsize(sfs_new);
	}

	return 0;
}

int
sysctlfs_getattr(struct puffs_usermount *pu, void *opc, struct vattr *va,
	const struct puffs_cred *pcr, pid_t pid)
{
	struct sfsnode *sfs = opc;

	memset(va, 0, sizeof(struct vattr));

	if (ISADIR(sfs)) {
		va->va_type = VDIR;
		va->va_mode = 0777;
	} else {
		va->va_type = VREG;
		va->va_mode = 0666;
	}
	va->va_nlink = getlinks(sfs);
	va->va_fsid = pu->pu_fsidx.__fsid_val[0];
	va->va_fileid = sfs->myid;
	va->va_size = getsize(sfs);
	va->va_gen = 1;
	va->va_rdev = PUFFS_VNOVAL;
	va->va_blocksize = 512;
	va->va_filerev = 1;

	va->va_atime = va->va_mtime = va->va_ctime = va->va_birthtime = fstime;

	return 0;
}

int
sysctlfs_readdir(struct puffs_usermount *pu, void *opc, struct dirent *dent,
	const struct puffs_cred *pcr, off_t *readoff, size_t *reslen)
{
	struct sysctlnode sn[128];
	struct sysctlnode qnode;
	struct sfsnode *sfs_dir = opc;
	size_t sl;
	enum vtype vt;
	int i;

 again:
	if (*readoff == DENT_DOT || *readoff == DENT_DOTDOT) {
		puffs_gendotdent(&dent, sfs_dir->myid, *readoff, reslen);
		(*readoff)++;
		goto again;
	}

	memset(&qnode, 0, sizeof(qnode));
	sl = 128 * sizeof(struct sysctlnode);
	qnode.sysctl_flags = SYSCTL_VERSION;
	sfs_dir->name[sfs_dir->hierlen] = CTL_QUERY;

	if (sysctl(sfs_dir->name, sfs_dir->hierlen + 1, sn, &sl,
	    &qnode, sizeof(qnode)) == -1)
		return ENOENT;

	for (i = DENT_ADJ(*readoff);
	    i < sl / sizeof(struct sysctlnode);
	    i++, (*readoff)++) {
		if (SYSCTL_TYPE(sn[i].sysctl_flags) == CTLTYPE_NODE)
			vt = VDIR;
		else
			vt = VREG;
		if (!puffs_nextdent(&dent, sn[i].sysctl_name, nextid++,
		    vt, reslen))
			return 0;
	}

	return 0;
}

int
sysctlfs_read(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	char localbuf[SFS_MAXFILE];
	int xfer;
	struct sfsnode *sfs = opc;

	if (ISADIR(sfs))
		return EISDIR;

	doprint(sfs, localbuf, sizeof(localbuf));
	xfer = MIN(*resid, strlen(localbuf) - offset);

	if (xfer < 0)
		return EINVAL;

	memcpy(buf, localbuf + offset, xfer);
	*resid -= xfer;

	return 0;
}

int
sysctlfs_write(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *cred,
	int ioflag)
{
	struct sfsnode *sfs = opc;
	long long ll;
	int i, rv;

	if (ISADIR(sfs))
		return EISDIR;

	if (offset != 0)
		return EINVAL;

	if (ioflag & PUFFS_IO_APPEND)
		return EINVAL;

	switch (SYSCTL_TYPE(sfs->sctln.sysctl_flags)) {
	case CTLTYPE_INT:
		if (sscanf((const char *)buf, "%d", &i) != 1)
			return EINVAL;
		rv = sysctl(sfs->name, sfs->hierlen, NULL, NULL,
		    &i, sizeof(int));
		break;
	case CTLTYPE_QUAD:
		if (sscanf((const char *)buf, "%lld", &ll) != 1)
			return EINVAL;
		rv =  sysctl(sfs->name, sfs->hierlen, NULL, NULL,
		    &ll, sizeof(long long));
		break;
	case CTLTYPE_STRING:
		rv = sysctl(sfs->name, sfs->hierlen, NULL, NULL, buf, *resid);
		break;
	default:
		rv = EINVAL;
		break;
	}

	if (rv)
		return rv;

	*resid = 0;
	return 0;
}

int
sysctlfs_reclaim(struct puffs_usermount *pu, void *opc, pid_t pid)
{
	struct sfsnode *sfs = opc;

	/*
	 * refcount nodes so that we don't accidentally release dotdot
	 * while we could still reference it in lookup
	 */
	if (--sfs->dotdot->refcount == 0)
		free(sfs->dotdot);
	if (--sfs->refcount == 0)
		free(sfs);

	return 0;
}
