/*  $NetBSD: perfuse.c,v 1.9 2010/09/29 08:01:10 manu Exp $ */

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
#include <machine/vmparam.h>

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
	TAILQ_INIT(&ps->ps_pnd);
	
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
	int sv[2];
	struct sockaddr_un sun;
	struct sockaddr *sa;
	char progname[] = _PATH_PERFUSED;
	char minus_i[] = "-i";
	char fdstr[16];
	char *const argv[] = { progname, minus_i, fdstr, NULL};
	char *const envp[] = { NULL };
	uint32_t opt;

	if (strcmp(path, _PATH_FUSE) != 0)
		return open(path, flags, mode);

	if ((sv[0] = socket(PF_LOCAL, PERFUSE_SOCKTYPE, 0)) == -1) {
#ifdef PERFUSE_DEBUG
		DWARN("%s:%d socket failed: %s", __func__, __LINE__);
#endif
		return -1;
	}

	/*
	 * Set a buffer lentgh large enough so that any FUSE packet
	 * will fit.
	 */
	opt = FUSE_BUFSIZE;
	if (setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	opt = FUSE_BUFSIZE;
	if (setsockopt(sv[0], SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_RCVBUF to %d failed", __func__, opt);

	sa = (struct sockaddr *)(void *)&sun;
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_LOCAL;
	(void)strcpy(sun.sun_path, path);

	if (connect(sv[0], sa, (socklen_t)sun.sun_len) == 0) 
		return sv[0];


	/*
	 * Attempt to run perfused on our own
	 * if it does not run yet; In that case
	 * we will talk using a socketpair 
	 * instead of /dev/fuse.
	 */
	if (socketpair(PF_LOCAL, PERFUSE_SOCKTYPE, 0, sv) != 0) {
		DWARN("%s:%d: socketpair failed", __func__, __LINE__);
		return -1;
	}

	/*
	 * Set a buffer lentgh large enough so that any FUSE packet
	 * will fit.
	 */
	opt = FUSE_BUFSIZE;
	if (setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	opt = FUSE_BUFSIZE;
	if (setsockopt(sv[0], SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_RCVBUF to %d failed", __func__, opt);

	opt = FUSE_BUFSIZE;
	if (setsockopt(sv[1], SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	opt = FUSE_BUFSIZE;
	if (setsockopt(sv[1], SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt SO_RCVBUF to %d failed", __func__, opt);

	/*
	 * Request peer credentials. This musr be done before first 
	 * frame is sent.
	 */
	opt = 1;
	if (setsockopt(sv[1], 0, LOCAL_CREDS, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt LOCAL_CREDS failed", __func__);

	(void)sprintf(fdstr, "%d", sv[1]);

	switch(fork()) {
	case -1:
#ifdef PERFUSE_DEBUG
		DWARN("%s:%d: fork failed", __func__, __LINE__);
#endif
		return -1;
		/* NOTREACHED */
		break;
	case 0:
		(void)execve(argv[0], argv, envp);
#ifdef PERFUSE_DEBUG
		DWARN("%s:%d: execve failed", __func__, __LINE__);
#endif
		return -1;
		/* NOTREACHED */
		break;
	default:
		break;
	}
	
	return sv[0];
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
	size_t len;
	struct perfuse_mount_out *pmo;
#if (PERFUSE_SOCKTYPE == SOCK_DGRAM)
	struct sockaddr_storage ss;
	struct sockaddr_un *sun;
	struct sockaddr *sa;
	socklen_t sa_len;
#endif
	size_t sock_len;
	char *frame;
	char *cp;

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("%s(\"%s\", \"%s\", \"%s\", 0x%lx, \"%s\")\n",
			__func__, source, target, filesystemtype, 
			mountflags, (const char *)data);
#endif

	if ((s = get_fd(data)) == -1)
		return -1;

	/*
	 * If we are connected to /dev/fuse, we need a second
	 * socket to get replies from perfused.
	 * XXX This socket is not removed at exit time yet
	 */
	sock_len = 0;
#if (PERFUSE_SOCKTYPE == SOCK_DGRAM)
	sa = (struct sockaddr *)(void *)&ss;
	sun = (struct sockaddr_un *)(void *)&ss;
	sa_len = sizeof(ss);
	if ((getpeername(s, sa, &sa_len) == 0) &&
	    (sa->sa_family = AF_LOCAL) &&
	    (strcmp(sun->sun_path, _PATH_FUSE) == 0)) {

		sun->sun_len = sizeof(*sun);
		sun->sun_family = AF_LOCAL;
		(void)sprintf(sun->sun_path, "%s/%s-%d",
			      _PATH_TMP, getprogname(), getpid());
		
		if (bind(s, sa, (socklen_t)sa->sa_len) != 0)
			DERR(EX_OSERR, "%s:%d bind to \"%s\" failed",
			     __func__, __LINE__, sun->sun_path);

		sock_len = strlen(sun->sun_path) + 1;
	}
#endif /* PERFUSE_SOCKTYPE */
		
	len = sizeof(*pmo);
	len += source ? (uint32_t)strlen(source) + 1 : 0;
	len += target ? (uint32_t)strlen(target) + 1 : 0;
	len += filesystemtype ? (uint32_t)strlen(filesystemtype) + 1 : 0;
	len += data ? (uint32_t)strlen(data) + 1 : 0;
	len += sock_len;

	if ((frame = malloc(len)) == NULL) {
#ifdef PERFUSE_DEBUG
		if (perfuse_diagflags & PDF_MISC)
			DWARN("%s:%d malloc failed", __func__, __LINE__);
#endif
		return -1;
	}

	pmo = (struct perfuse_mount_out *)(void *)frame;
	pmo->pmo_len = len;
	pmo->pmo_error = 0;
	pmo->pmo_unique = (uint64_t)-1;
	(void)strcpy(pmo->pmo_magic, PERFUSE_MOUNT_MAGIC);

	pmo->pmo_source_len = source ? (uint32_t)strlen(source) + 1 : 0;
	pmo->pmo_target_len = target ? (uint32_t)strlen(target) + 1: 0;
	pmo->pmo_filesystemtype_len = 
	    filesystemtype ? (uint32_t)strlen(filesystemtype) + 1 : 0;
	pmo->pmo_mountflags = (uint32_t)mountflags;
	pmo->pmo_data_len = data ? (uint32_t)strlen(data) + 1 : 0;
	pmo->pmo_sock_len = sock_len;
	
	cp = (char *)(void *)(pmo + 1);

	if (source) {
		(void)strcpy(cp, source);
		cp += pmo->pmo_source_len;
	}
	
	if (target) {
		(void)strcpy(cp, target);
		cp += pmo->pmo_target_len;
	}

	if (filesystemtype) {
		(void)strcpy(cp, filesystemtype);
		cp += pmo->pmo_filesystemtype_len;
	}
	
	if (data) {
		(void)strcpy(cp, data);
		cp += pmo->pmo_data_len;
	}

	if (sock_len != 0) {
		(void)strcpy(cp, sun->sun_path);
		cp += pmo->pmo_sock_len;
	}

	if (send(s, frame, len, MSG_NOSIGNAL) != len) {
#ifdef PERFUSE_DEBUG
		DWARN("%s:%d sendto failed", __func__, __LINE__);
#endif
		return -1;
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
	ps->ps_owner_uid = pmi->pmi_uid;

	if (pmi->pmi_source)
		ps->ps_source = strdup(pmi->pmi_source);
	if (pmi->pmi_filesystemtype)
		ps->ps_filesystemtype = strdup(pmi->pmi_filesystemtype);
	ps->ps_target = strdup(pmi->pmi_target);
	ps->ps_mountflags = pmi->pmi_mountflags;

	/*
	 * Some options are forbidden for non root users
	 */
	if (ps->ps_owner_uid != 0)
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

	puffs_flags = PUFFS_KFLAG_WTCACHE;

	if (perfuse_diagflags & PDF_PUFFS)
		puffs_flags |= PUFFS_FLAG_OPDUMP;

	if (perfuse_diagflags & PDF_FILENAME)
		puffs_flags |= PUFFS_FLAG_BUILDPATH;

	if ((pu = puffs_init(pops, _PATH_PUFFS, name, ps, puffs_flags)) == NULL)
		DERR(EX_OSERR, "puffs_init failed");

	ps->ps_pu = pu;

	/*
	 * Setup filesystem root
	 */
	pn_root = perfuse_new_pn(pu, NULL);
	PERFUSE_NODE_DATA(pn_root)->pnd_ino = FUSE_ROOT_ID; 
	PERFUSE_NODE_DATA(pn_root)->pnd_parent = pn_root;
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

int
perfuse_unmount(pu)
	struct puffs_usermount *pu;
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return unmount(ps->ps_target, MNT_FORCE);
}
