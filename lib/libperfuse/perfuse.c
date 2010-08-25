/*  $NetBSD: perfuse.c,v 1.1 2010/08/25 07:16:00 manu Exp $ */

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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <puffs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LIBPERFUSE
#include "perfuse.h"
#include "perfuse_if.h"
#include "perfuse_priv.h"

int perfuse_diagflags = 0; /* global used only in DPRINTF/DERR/DWARN */

static struct perfuse_state *init_state(void);
static int get_fd(const char *);

static struct perfuse_state *
init_state(void)
{
	struct perfuse_state *ps;

	if ((ps = malloc(sizeof(*ps))) == NULL)
		DERR(EX_OSERR, "malloc failed");

	(void)memset(ps, 0, sizeof(*ps));
	ps->ps_max_write = UINT_MAX;
	ps->ps_max_readahead = UINT_MAX;
	
	return ps;
}


static int
get_fd(data)
	const char *data;
{
	char *string;
	const char fdopt[] = "fd=";
	char *lastp;
	char *opt;
	int fd = -1;
	
	if ((string = strdup(data)) == NULL) 
		return -1;
		
	for (opt = strtok_r(string, ",", &lastp);
	     opt != NULL;
	     opt = strtok_r(NULL, ",", &lastp)) {
		if (strncmp(opt, fdopt, strlen(fdopt)) == 0) {
			fd =  atoi(opt + strlen(fdopt));
			break;
		}
	}

	/* 
	 * No file descriptor found
	 */ 
	if (fd == -1)
		errno = EINVAL;

	free(string);
	return fd;

}

int
perfuse_open(path, flags, mode)
	const char *path;
	int flags;
	mode_t mode;
{
	int s;
	struct sockaddr_un sun;
	struct sockaddr *sa;

	if (strcmp(path, _PATH_FUSE) != 0)
		return open(path, flags, mode);

	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1) {
#ifdef PERFUSE_DEBUG
		printf("%s:%d socket failed: %s", 
		       __func__, __LINE__, strerror(errno));
#endif
		return -1;
	}

	sa = (struct sockaddr *)(void *)&sun;
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_LOCAL;
	(void)strcpy(sun.sun_path, path);

	if (connect(s, sa, (socklen_t)sun.sun_len) == -1) {
#ifdef PERFUSE_DEBUG
		printf("%s:%d connect failed: %s", 
		       __func__, __LINE__, strerror(errno));
#endif
		close(s);
		return -1;
	}

	return s;
}


int
perfuse_mount(source, target, filesystemtype, mountflags, data)
	const char *source;
	const char *target;
	const char *filesystemtype;
	long mountflags;
	const void *data;
{
	int s;
#if 0
	struct sockaddr_un sun;
#endif
	size_t len;
	struct perfuse_mount_out pmo;

#ifdef PERFUSE_DEBUG
	printf("%s(\"%s\", \"%s\", \"%s\", 0x%lx, \"%s\")\n", __func__,
	       source, target, filesystemtype, mountflags, (const char *)data);
#endif

#if 0
	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
		err(EX_OSERR, "socket failed");

	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_LOCAL;
	(void)strcpy(sun.sun_path, _PATH_FUSE);

	if (connect(s, (struct sockaddr *)&sun, sun.sun_len) == -1)
		err(EX_UNAVAILABLE, "cannot connect to \"%s\"", _PATH_FUSE);
#endif
	if ((s = get_fd(data)) == -1)
		return -1;
	

	pmo.pmo_len = sizeof(pmo);
	pmo.pmo_len += source ? strlen(source) : 0;
	pmo.pmo_len += target ? strlen(target) : 0;
	pmo.pmo_len += filesystemtype ? strlen(filesystemtype) : 0;
	pmo.pmo_len += data ? strlen(data) : 0;
	pmo.pmo_error = 0;
	pmo.pmo_unique = (uint64_t)-1;

	(void)strcpy(pmo.pmo_magic, PERFUSE_MOUNT_MAGIC);
	pmo.pmo_source_len = source ? strlen(source) : 0;
	pmo.pmo_target_len = target ? strlen(target) : 0;
	pmo.pmo_filesystemtype_len = 
	    filesystemtype ? strlen(filesystemtype) : 0;
	pmo.pmo_mountflags = mountflags;
	pmo.pmo_data_len = data ? strlen(data) : 0;
	

	if (write(s, &pmo, sizeof(pmo)) != sizeof(pmo)) {
#ifdef PERFUSE_DEBUG
		printf("%s:%d short write", __func__, __LINE__);
#endif
		return -1;
	}
	
	if (source) {
		len = pmo.pmo_source_len;
		if (write(s, source, len) != (ssize_t)len) {
#ifdef PERFUSE_DEBUG
			printf("%s:%d short write", __func__, __LINE__);
#endif
			return -1;
		}
	}
	
	if (target) {
		len = pmo.pmo_target_len;
		if (write(s, target, len) != (ssize_t)len) {
#ifdef PERFUSE_DEBUG
			printf("%s:%d short write", __func__, __LINE__);
#endif
			return -1;
		}
	}
	
	if (filesystemtype) {
		len = pmo.pmo_filesystemtype_len;
		if (write(s, filesystemtype, len) != (ssize_t)len) {
#ifdef PERFUSE_DEBUG
			printf("%s:%d short write", __func__, __LINE__);
#endif
			return -1;
		}
	}
	
	if (data) {
		len = pmo.pmo_data_len;
		if (write(s, data, len) != (ssize_t)len) {
#ifdef PERFUSE_DEBUG
			printf("%s:%d short write", __func__, __LINE__);
#endif
			return -1;
		}
	}

	return 0;
}


uint64_t
perfuse_next_unique(pu)
	struct puffs_usermount *pu;
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return ps->ps_unique++;
} 

struct puffs_usermount *
perfuse_init(pc, pmi)
	struct perfuse_callbacks *pc;
	struct perfuse_mount_info *pmi;
{
	struct perfuse_state *ps;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	char name[] = "perfuse";
	unsigned int puffs_flags;
	struct puffs_node *pn_root;
	struct puffs_pathobj *po_root;

	ps = init_state();
	ps->ps_uid = pmi->pmi_uid;

	if (pmi->pmi_source)
		ps->ps_source = strdup(pmi->pmi_source);
	if (pmi->pmi_filesystemtype)
		ps->ps_filesystemtype = strdup(pmi->pmi_filesystemtype);
	ps->ps_target = strdup(pmi->pmi_target);
	ps->ps_mountflags = pmi->pmi_mountflags;

	/*
	 * Some options are forbidden for non root users
	 */
	if (ps->ps_uid != 0)
	    ps->ps_mountflags |= MNT_NOSUID|MNT_NODEV;

	PUFFSOP_INIT(pops);
	PUFFSOP_SET(pops, perfuse, fs, unmount);
	PUFFSOP_SET(pops, perfuse, fs, statvfs);
	PUFFSOP_SET(pops, perfuse, fs, sync);
	PUFFSOP_SET(pops, perfuse, node, lookup);
	PUFFSOP_SET(pops, perfuse, node, create);
	PUFFSOP_SET(pops, perfuse, node, mknod);
	PUFFSOP_SET(pops, perfuse, node, open);
	PUFFSOP_SET(pops, perfuse, node, close);
	PUFFSOP_SET(pops, perfuse, node, access);
	PUFFSOP_SET(pops, perfuse, node, getattr);
	PUFFSOP_SET(pops, perfuse, node, setattr);
	PUFFSOP_SET(pops, perfuse, node, poll);
#if 0 
	PUFFSOP_SET(pops, perfuse, node, mmap);
#endif
	PUFFSOP_SET(pops, perfuse, node, fsync);
	PUFFSOP_SET(pops, perfuse, node, seek);
	PUFFSOP_SET(pops, perfuse, node, remove);
	PUFFSOP_SET(pops, perfuse, node, link);
	PUFFSOP_SET(pops, perfuse, node, rename);
	PUFFSOP_SET(pops, perfuse, node, mkdir);
	PUFFSOP_SET(pops, perfuse, node, rmdir);
	PUFFSOP_SET(pops, perfuse, node, symlink);
	PUFFSOP_SET(pops, perfuse, node, readdir);
	PUFFSOP_SET(pops, perfuse, node, readlink);
	PUFFSOP_SET(pops, perfuse, node, reclaim);
	PUFFSOP_SET(pops, perfuse, node, inactive);
	PUFFSOP_SET(pops, perfuse, node, print);
	PUFFSOP_SET(pops, perfuse, node, advlock);
	PUFFSOP_SET(pops, perfuse, node, read);
	PUFFSOP_SET(pops, perfuse, node, write);

	puffs_flags = PUFFS_FLAG_BUILDPATH | PUFFS_FLAG_HASHPATH;
	if (perfuse_diagflags & PDF_PUFFS)
		puffs_flags |= PUFFS_FLAG_OPDUMP;

	if ((pu = puffs_init(pops, _PATH_PUFFS, name, ps, puffs_flags)) == NULL)
		DERR(EX_OSERR, "puffs_init failed");

	ps->ps_pu = pu;

	/*
	 * Setup filesystem root
	 */
	pn_root = perfuse_new_pn(pu, NULL);
	PERFUSE_NODE_DATA(pn_root)->pnd_ino = FUSE_ROOT_ID; 
	puffs_setroot(pu, pn_root);
	ps->ps_fsid = pn_root->pn_va.va_fsid;
	
	po_root = puffs_getrootpathobj(pu);
	if ((po_root->po_path = strdup("/")) == NULL)
		DERRX(EX_OSERR, "perfuse_mount_start() failed");
	
	po_root->po_len = 1;
	puffs_path_buildhash(pu, po_root);

	puffs_vattr_null(&pn_root->pn_va);
	pn_root->pn_va.va_type = VDIR;
	pn_root->pn_va.va_mode = 0755;
	
	ps->ps_root = pn_root;

	/* 
	 *  Callbacks
	 */
	ps->ps_new_msg = pc->pc_new_msg;
	ps->ps_xchg_msg = pc->pc_xchg_msg;
	ps->ps_destroy_msg = pc->pc_destroy_msg;
	ps->ps_get_inhdr = pc->pc_get_inhdr;
	ps->ps_get_inpayload = pc->pc_get_inpayload;
	ps->ps_get_outhdr = pc->pc_get_outhdr;
	ps->ps_get_outpayload = pc->pc_get_outpayload;

	return pu;
} 

void
perfuse_setspecific(pu, priv)
	struct puffs_usermount *pu;
	void *priv;
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);
	ps->ps_private = priv;	

	return;
}

void *
perfuse_getspecific(pu)
	struct puffs_usermount *pu;
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return ps->ps_private;
}

int
perfuse_inloop(pu)
	struct puffs_usermount *pu;
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return ps->ps_flags & PS_INLOOP;
}

int
perfuse_mainloop(pu)
	struct puffs_usermount *pu;
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	ps->ps_flags |= PS_INLOOP;
	if (puffs_mainloop(ps->ps_pu) != 0)
		DERR(EX_OSERR, "puffs_mainloop failed");
	DERR(EX_OSERR, "puffs_mainloop exit");

	/* NOTREACHED */
	return -1;
}

/* ARGSUSED0 */
uint64_t
perfuse_get_ino(pu, opc)
	struct puffs_usermount *pu;
	puffs_cookie_t opc;
{
	return PERFUSE_NODE_DATA(opc)->pnd_ino;
}
