/*	$NetBSD: ssshfs.c,v 1.9.2.1 2008/02/02 17:21:39 riz Exp $	*/

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
 * simple sshfs
 * (silly sshfs?  stupid sshfs?  snappy sshfs?  sucky sshfs?  seven sshfs???)
 * (sante sshfs?  severed (dreams) sshfs?  saucy sshfs?  sauerkraut sshfs?)
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <unistd.h>

#include "buffer.h"
#include "sftp.h"
#include "sftp-common.h"
#include "sftp-client.h"

PUFFSVFS_PROTOS(ssshfs)
PUFFSVN_PROTOS(ssshfs)

struct ssshnode {
	struct ssshnode *dotdot;
	int childcount;
	ino_t myid;

	char name[MAXPATHLEN+1];
	size_t namelen;

	SFTP_DIRENT **ents;
	int dcache;

	struct vattr va;
};
#define DCACHE_EXISTS	0x1
#define DCACHE_CHANGED	0x2

static struct sftp_conn *sftpc;
int in, out;

static struct ssshnode rn;
static ino_t nextid = 3;

extern int sftp_main(int argc, char *argv[]);

char *argpath; /* XXX: arg passing nightmare */

/*
 * uberquickhack one-person uidgid-mangler in case the target
 * system doesn't have the same uids and gids
 */
static int mangle = 0;
static uid_t uidmangle_from = 1323, uidmangle_to = 5988;
static uid_t gidmangle_from = 100, gidmangle_to = 806;

int
main(int argc, char *argv[])
{
	struct puffs_usermount *pu;
	struct puffs_vfsops pvfs;
	struct puffs_vnops pvn;
	char *mountpath;

	setprogname(argv[0]);

	if (argc < 3)
		errx(1, "usage: %s user@host:path mountpath", getprogname());

	memset(&pvfs, 0, sizeof(struct puffs_vfsops));
	memset(&pvn, 0, sizeof(struct puffs_vnops));

	pvfs.puffs_mount = ssshfs_mount;
	pvfs.puffs_unmount = ssshfs_unmount;
	pvfs.puffs_sync = puffs_vfsnop_sync; /* XXX! */
	pvfs.puffs_statvfs = puffs_vfsnop_statvfs;

	pvn.puffs_lookup = ssshfs_lookup;
	pvn.puffs_getattr = ssshfs_getattr;
	pvn.puffs_setattr = ssshfs_setattr;
	pvn.puffs_readdir = ssshfs_readdir;
	pvn.puffs_symlink = ssshfs_symlink;
	pvn.puffs_readlink = ssshfs_readlink;
	pvn.puffs_remove = ssshfs_remove;
	pvn.puffs_mkdir = ssshfs_mkdir;
	pvn.puffs_rmdir = ssshfs_rmdir;
	pvn.puffs_read = ssshfs_read;
	pvn.puffs_create = ssshfs_create;
	pvn.puffs_write = ssshfs_write;
	pvn.puffs_rename = ssshfs_rename;
	pvn.puffs_reclaim = ssshfs_reclaim;

	mountpath = argv[--argc]; /* urgh */

	sftp_main(argc, argv);

	if ((pu = puffs_mount(&pvfs, &pvn, mountpath, 0, "ssshfs",
	    PUFFS_KFLAG_NOCACHE, 0))==NULL)
		err(1, "mount");

	if (puffs_mainloop(pu, 0) == -1)
		err(1, "mainloop");

	return 0;
}

static void
buildpath(struct ssshnode *ssn, const struct ssshnode *ossn, const char *pcomp)
{
	size_t clen = strlen(pcomp);

	assert((ossn->namelen + clen + 1 + 1) <= MAXPATHLEN);

	memcpy(ssn->name, ossn->name, ossn->namelen);
	ssn->name[ossn->namelen] = '/';
	strcat(ssn->name, pcomp);
	ssn->namelen = ossn->namelen + clen + 1; /* not nil, but '/' */
	ssn->name[ssn->namelen] = 0;
}

static void
buildvattr(struct ssshnode *ssn, const Attrib *a)
{
	struct vattr *va = &ssn->va;

	if (a->flags & SSH2_FILEXFER_ATTR_SIZE) {
		va->va_size = a->size;
		va->va_bytes = a->size;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_UIDGID) {
		if (a->uid == uidmangle_to && mangle)
			va->va_uid = uidmangle_from;
		else
			va->va_uid = a->uid;

		if (a->gid == gidmangle_to && mangle)
			va->va_gid = gidmangle_from;
		else
			va->va_gid = a->gid;
	}
	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
		va->va_mode = a->perm;
	if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		va->va_atime.tv_sec = a->atime;
		va->va_mtime.tv_sec = a->mtime;
	}

	va->va_type = puffs_mode2vt(va->va_mode);
}

static Attrib *
vattrtoAttrib(const struct vattr *va)
{
	static Attrib a; /* XXX, but sftp isn't threadsafe either */

	memset(&a, 0, sizeof(a));

	if (va->va_size != PUFFS_VNOVAL) {
		a.size = va->va_size;
		a.flags |= SSH2_FILEXFER_ATTR_SIZE;
	}

	if (va->va_uid != PUFFS_VNOVAL) {
		if (va->va_uid == uidmangle_from && mangle)
			a.uid = uidmangle_to;
		else
			a.uid = va->va_uid;

		if (va->va_gid == gidmangle_from && mangle)
			a.gid = gidmangle_to;
		else
			a.gid = va->va_gid;

		a.flags |= SSH2_FILEXFER_ATTR_UIDGID;
	}

	if (va->va_mode != PUFFS_VNOVAL) {
		a.perm = va->va_mode & 0777;
		a.flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
	}

	if (va->va_atime.tv_sec != PUFFS_VNOVAL) {
		a.atime = va->va_atime.tv_sec;
		a.mtime = va->va_mtime.tv_sec;
		a.flags |= SSH2_FILEXFER_ATTR_ACMODTIME;
	}

	return &a;
}

static int
dircache(struct ssshnode *ssn)
{

	assert(ssn->va.va_type == VDIR);

	if ((ssn->dcache & DCACHE_EXISTS)
	    && ((ssn->dcache & DCACHE_CHANGED) == 0))
		return 0;

	if (ssn->dcache & DCACHE_EXISTS)
		free_sftp_dirents(ssn->ents);
	ssn->dcache &= ~DCACHE_EXISTS;

	if (do_readdir(sftpc, ssn->name, &ssn->ents) != 0)
		return 1;

	ssn->dcache |= DCACHE_EXISTS;
	ssn->dcache &= ~DCACHE_CHANGED;

	return 0;
}

static struct ssshnode *
makenewnode(struct ssshnode *ossn, const char *pcomp, const char *longname)
{
	struct ssshnode *newssn;
	int links;

	newssn = emalloc(sizeof(struct ssshnode));
	memset(newssn, 0, sizeof(struct ssshnode));

	newssn->dotdot = ossn;
	newssn->myid = nextid++;

	buildpath(newssn, ossn, pcomp);

	ossn->childcount++;
	newssn->childcount = 0;
	newssn->va.va_fileid = newssn->myid;
	newssn->va.va_blocksize = 512;
	newssn->va.va_size = 0;
	newssn->va.va_bytes = 0;

	/* XXX: only way I know how (didn't look into the protocol, though) */
	if (longname && (sscanf(longname, "%*s%d", &links) == 1))
		newssn->va.va_nlink = links;
	else
		newssn->va.va_nlink = 1;

	return newssn;
}

int
ssshfs_mount(struct puffs_usermount *pu, void **rootcookie)
{

	sftpc = do_init(in, out, 1<<15, 1);
	if (sftpc == NULL) {
		printf("can't init sftpc\n");
		return EBUSY;
	}

	rn.childcount = 1;
	rn.myid = 2;

	memset(rn.name, 0, sizeof(rn.name));

	if (argpath)
		strcpy(rn.name, argpath);
	else {
		char *dotpath;
		dotpath = do_realpath(sftpc, ".");
		if (!dotpath)
			return ENOENT;
		strcpy(rn.name, dotpath);
	}
	rn.namelen = strlen(rn.name);

	rn.va.va_type = VDIR;
	rn.va.va_mode = 0777;
	rn.va.va_nlink = 1024; /* XXXX */

	*rootcookie = &rn;

	return 0;
}

int
ssshfs_unmount(struct puffs_usermount *pu, int flags, pid_t pid)
{

	close(in);
	close(out);
	return 0;
}

int
ssshfs_lookup(struct puffs_usermount *pu, void *opc, void **newnode,
	enum vtype *newtype, voff_t *newsize, dev_t *newrdev,
	const struct puffs_cn *pcn)
{
	struct ssshnode *ssn = opc;
	struct ssshnode *newssn;
	struct SFTP_DIRENT *de;
	int i;

	if (pcn->pcn_flags & PUFFS_ISDOTDOT) {
		*newnode = ssn->dotdot;
		*newtype = VDIR;
		return 0;
	}

	dircache(ssn);

	for (i = 0, de = ssn->ents[0]; de; de = ssn->ents[i++])
		if (strcmp(de->filename, pcn->pcn_name) == 0)
			break;

	if (!de)
		return ENOENT;

	newssn = makenewnode(ssn, de->filename, de->longname);
	buildvattr(newssn, &de->a);

	*newnode = newssn;
	*newtype = newssn->va.va_type;
	*newsize = newssn->va.va_size;

	return 0;
}

int
ssshfs_getattr(struct puffs_usermount *pu, void *opc, struct vattr *va,
	const struct puffs_cred *pcr, pid_t pid)
{
	struct ssshnode *ssn = opc;

	memcpy(va, &ssn->va, sizeof(struct vattr));

	return 0;
}

int
ssshfs_setattr(struct puffs_usermount *pu, void *opc, const struct vattr *va,
	const struct puffs_cred *pcr, pid_t pid)
{
	struct ssshnode *ssn = opc;
	Attrib *a;
	int rv;

	puffs_setvattr(&ssn->va, va);
	a = vattrtoAttrib(va);

	/* XXX: compensate for lack of granulatity of SSH2_FILEXFER */
	if ((a->flags & SSH2_FILEXFER_ATTR_ACMODTIME)
	    && (va->va_mtime.tv_sec == PUFFS_VNOVAL))
		a->mtime = ssn->va.va_mtime.tv_sec;

	rv = do_setstat(sftpc, ssn->name, a);
	if (rv)
		return EIO;

	return 0;
}

int do_creatfile(struct sftp_conn *conn, char *path, Attrib *a); /* XXX */
int
ssshfs_create(struct puffs_usermount *pu, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct ssshnode *ssd = opc, *newssn;
	Attrib *a;
	int rv;

	newssn = makenewnode(ssd, pcn->pcn_name, NULL);
	puffs_setvattr(&newssn->va, va);
	a = vattrtoAttrib(va);
	if ((rv = do_creatfile(sftpc, newssn->name, a)) != 0) {
		/* XXX: free newssn */
		return EIO;
	}

	ssd->dcache |= DCACHE_CHANGED;
	ssd->va.va_nlink++;

	*newnode = newssn;
	return 0;
}

int
ssshfs_readdir(struct puffs_usermount *pu, void *opc, struct dirent *dent,
	const struct puffs_cred *pcr, off_t *readoff, size_t *reslen)
{
	struct ssshnode *ssn = opc;
	struct SFTP_DIRENT *de;

	dircache(ssn);

	for (de = ssn->ents[*readoff]; de; de = ssn->ents[++(*readoff)]) {
		if (!puffs_nextdent(&dent, de->filename, nextid++,
		    puffs_vtype2dt(ssn->va.va_type), reslen))
			return 0;
	}

	return 0;
}

int
ssshfs_read(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	size_t x1, x2;
	struct ssshnode *ssn = opc;
	int rv;

	if (offset > ssn->va.va_size)
		return 0;

	x1 = *resid;
	if (offset + *resid > ssn->va.va_size)
		x1 = ssn->va.va_size - offset;

	if (x1 == 0)
		return 0;

	x2 = x1;
	rv = do_readfile(sftpc, ssn->name, buf, offset, &x1);
	if (rv)
		return EIO;

	*resid -= (x2 - x1);

	return 0;
}

int
ssshfs_write(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *cred,
	int ioflag)
{
	struct ssshnode *ssn = opc;
	size_t origres;
	int rv;

	origres = *resid;
	rv = do_writefile(sftpc, ssn->name, buf, offset, resid,
	    ioflag & PUFFS_IO_APPEND);

	if (ioflag & PUFFS_IO_APPEND) {
		ssn->va.va_size += origres - *resid;
	} else {
		if (offset + (origres - *resid) > ssn->va.va_size)
			ssn->va.va_size = offset + (origres - *resid);
	}

	if (rv)
		return EIO;

	return 0;
}

int
ssshfs_readlink(struct puffs_usermount *pu, void *opc,
	const struct puffs_cred *cred, char *linkvalue, size_t *linklen)
{
	struct ssshnode *ssn = opc;
	char *res;

	if (sftp_proto_version(sftpc) < 3)
		return EOPNOTSUPP;

	res = do_readlink(sftpc, ssn->name);
	if (!res)
		return EIO;
	*linklen = strlen(res);
	memcpy(linkvalue, res, *linklen);

	return 0;
}

int
ssshfs_remove(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct ssshnode *ssn = targ, *ssd = opc;
	int rv;

	if (ssn->va.va_type == VDIR)
		return EISDIR;

	if ((rv = do_rm(sftpc, ssn->name)) != 0)
		return EIO;

	ssd->dcache |= DCACHE_CHANGED;
	ssd->va.va_nlink--;

	return 0;
}

int
ssshfs_mkdir(struct puffs_usermount *pu, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct ssshnode *ssd = opc, *newssn;
	Attrib *a;
	int rv;

	newssn = makenewnode(ssd, pcn->pcn_name, NULL);
	newssn->va.va_nlink++;
	puffs_setvattr(&newssn->va, va);
	a = vattrtoAttrib(va);
	if ((rv = do_mkdir(sftpc, newssn->name, a)) != 0) {
		/* XXX: free newssn */
		return EIO;
	}

	ssd->dcache |= DCACHE_CHANGED;
	ssd->va.va_nlink++;

	*newnode = newssn;
	return 0;
}

int
ssshfs_rmdir(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct ssshnode *ssn = targ, *ssd = opc;
	int rv;

	if (ssn->va.va_type != VDIR)
		return ENOTDIR;

	if ((rv = do_rmdir(sftpc, ssn->name)) != 0)
		return EIO;

	ssd->dcache |= DCACHE_CHANGED;
	ssd->va.va_nlink--;

	return 0;
}

int
ssshfs_symlink(struct puffs_usermount *pu, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va,
	const char *link_target)
{
	struct ssshnode *ssd = opc, *newssn;
	Attrib *a;
	int rv;

	if (sftp_proto_version(sftpc) < 3)
		return EOPNOTSUPP;

	newssn = makenewnode(ssd, pcn->pcn_name, NULL);
	puffs_setvattr(&newssn->va, va);
	a = vattrtoAttrib(va);
	if ((rv = do_symlink(sftpc, __UNCONST(link_target), newssn->name)) != 0)
		return EIO;

	ssd->dcache |= DCACHE_CHANGED;
	ssd->va.va_nlink++;

	*newnode = newssn;
	return 0;
}

int
ssshfs_rename(struct puffs_usermount *pu, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct ssshnode *ssd_src = opc, *ssd_dest = targ_dir;
	struct ssshnode *ssn_file = src;
	char newname[MAXPATHLEN+1];

	strcpy(newname, ssd_dest->name);
	strcat(newname, "/");
	strcat(newname, pcn_targ->pcn_name);

	if (targ)
		if (do_rm(sftpc, newname))
			return EIO;

	if (do_rename(sftpc, ssn_file->name, newname))
		return EIO;

	/* ok, commit */
	ssn_file->namelen = strlen(newname);
	ssd_src->childcount--;
	ssd_src->va.va_nlink--;
	ssd_dest->childcount++;
	ssd_dest->va.va_nlink++;

	ssd_src->dcache |= DCACHE_CHANGED;
	ssd_dest->dcache |= DCACHE_CHANGED;

	return 0;
}

int
ssshfs_reclaim(struct puffs_usermount *pu, void *opc, pid_t pid)
{
	struct ssshnode *ssn, *ssn_next;

	for (ssn = opc; ssn != &rn; ssn = ssn_next) {
		if (ssn->childcount == 0) {
			if (ssn->dcache & DCACHE_EXISTS)
				free_sftp_dirents(ssn->ents);
			ssn_next = ssn->dotdot;
			ssn_next->childcount--;
			free(ssn);
		} else
			break;
	}

	return 0;
}
