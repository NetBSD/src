/*  $NetBSD: perfuse.c,v 1.44 2022/02/23 21:54:40 andvar Exp $ */

/*-
 *  Copyright (c) 2010-2011 Emmanuel Dreyfus. All rights reserved.
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
#include <inttypes.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/extattr.h>
#include <sys/hash.h>
#include <sys/un.h>
#include <machine/vmparam.h>

#define LIBPERFUSE
#include "perfuse.h"
#include "perfuse_if.h"
#include "perfuse_priv.h"

int perfuse_diagflags = 0; /* global used only in DPRINTF/DERR/DWARN */
extern char **environ;

static struct perfuse_state *init_state(void);
static int get_fd(const char *);

static struct perfuse_state *
init_state(void)
{
	struct perfuse_state *ps;
	size_t len;
	char opts[1024];
	int i;

	if ((ps = malloc(sizeof(*ps))) == NULL)
		DERR(EX_OSERR, "%s:%d malloc failed", __func__, __LINE__);

	(void)memset(ps, 0, sizeof(*ps));
	ps->ps_max_write = UINT_MAX;
	ps->ps_max_readahead = UINT_MAX;
	TAILQ_INIT(&ps->ps_trace);

	ps->ps_nnidhash = PUFFS_PNODEBUCKETS;
	len = sizeof(*ps->ps_nidhash) * ps->ps_nnidhash;
	if ((ps->ps_nidhash = malloc(len)) == NULL)
		DERR(EX_OSERR, "%s:%d malloc failed", __func__, __LINE__);
	for (i = 0; i < ps->ps_nnidhash; i++)
		LIST_INIT(&ps->ps_nidhash[i]);
	
	/*
	 * Most of the time, access() is broken because the filesystem
	 * performs the check with root privileges. glusterfs will do that 
	 * if the Linux-specific setfsuid() is missing, for instance.
	 */
	ps->ps_flags |= PS_NO_ACCESS;

	/*
	 * This is a temporary way to toggle access and creat usage.
	 * It would be nice if that could be provided as mount options,
	 * but that will not be obvious to do.
 	 */
	if (getenv_r("PERFUSE_OPTIONS", opts, sizeof(opts)) != -1) {
		char *optname; 
		char *last;

		 for ((optname = strtok_r(opts, ",", &last));
		      optname != NULL;
		      (optname = strtok_r(NULL, ",", &last))) {
			if (strcmp(optname, "enable_access") == 0)
				ps->ps_flags &= ~PS_NO_ACCESS;

			if (strcmp(optname, "disable_access") == 0)
				ps->ps_flags |= PS_NO_ACCESS;

			if (strcmp(optname, "enable_creat") == 0)
				ps->ps_flags &= ~PS_NO_CREAT;

			if (strcmp(optname, "disable_creat") == 0)
				ps->ps_flags |= PS_NO_CREAT;
		}
	}
	
	
	return ps;
}


static int
get_fd(const char *data)
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

uint32_t 
perfuse_bufvar_from_env(const char *name, uint32_t defval)
{
	char valstr[1024];
	int e;
	uint32_t retval;

	if (getenv_r(name, valstr, sizeof(valstr)) == -1)
		return defval;

	retval = (uint32_t)strtoi(valstr, NULL, 0, 0, UINT32_MAX, &e);
	if (!e)
		return retval;

	DWARNC(e, "conversion from `%s' to uint32_t failed, using %u",
	    valstr, defval);
	return defval;
}

int
perfuse_open(const char *path, int flags, mode_t mode)
{
	int sv[2];
	struct sockaddr_un sun;
	struct sockaddr *sa;
	char progname[] = _PATH_PERFUSED;
	char minus_i[] = "-i";
	char fdstr[16];
	char *const argv[] = { progname, minus_i, fdstr, NULL};
	uint32_t opt;
	uint32_t optlen;
	int sock_type = SOCK_SEQPACKET;

	if (strcmp(path, _PATH_FUSE) != 0)
		return open(path, flags, mode);

	/* 
	 * Try SOCK_SEQPACKET then SOCK_DGRAM if unavailable
	 */
	if ((sv[0] = socket(PF_LOCAL, SOCK_SEQPACKET, 0)) == -1) {
		sock_type = SOCK_DGRAM;
                DWARNX("SEQPACKET local sockets unavailable, using less "
		       "reliable DGRAM sockets. Expect file operation hangs.");

		if ((sv[0] = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
#ifdef PERFUSE_DEBUG
			DWARN("%s: %d socket failed", __func__, __LINE__);
#endif
			return -1;
		}
	}

	/*
	 * Set a buffer length large enough so that enough FUSE packets
	 * will fit.
	 */
	opt = perfuse_bufvar_from_env("PERFUSE_BUFSIZE",
	    (uint32_t)(16 * FUSE_BUFSIZE));
	optlen = sizeof(opt);
	if (setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &opt, optlen) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	if (setsockopt(sv[0], SOL_SOCKET, SO_RCVBUF, &opt, optlen) != 0)
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
	if (socketpair(PF_LOCAL, sock_type, 0, sv) != 0) {
		DWARN("%s:%d: socketpair failed", __func__, __LINE__);
		return -1;
	}

	/*
	 * Set a buffer length large enough so that enough FUSE packets
	 * will fit.
	 */
	opt = perfuse_bufvar_from_env("PERFUSE_BUFSIZE",
	    (uint32_t)(16 * FUSE_BUFSIZE));
	optlen = sizeof(opt);
	if (setsockopt(sv[0], SOL_SOCKET, SO_SNDBUF, &opt, optlen) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	if (setsockopt(sv[0], SOL_SOCKET, SO_RCVBUF, &opt, optlen) != 0)
		DWARN("%s: setsockopt SO_RCVBUF to %d failed", __func__, opt);

	if (setsockopt(sv[1], SOL_SOCKET, SO_SNDBUF, &opt, optlen) != 0)
		DWARN("%s: setsockopt SO_SNDBUF to %d failed", __func__, opt);

	if (setsockopt(sv[1], SOL_SOCKET, SO_RCVBUF, &opt, optlen) != 0)
		DWARN("%s: setsockopt SO_RCVBUF to %d failed", __func__, opt);

	/*
	 * Request peer credentials. This musr be done before first 
	 * frame is sent.
	 */
	opt = 1;
	optlen = sizeof(opt);
	if (setsockopt(sv[1], SOL_LOCAL, LOCAL_CREDS, &opt, optlen) != 0)
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
		(void)close(sv[0]);
		(void)execve(argv[0], argv, environ);
#ifdef PERFUSE_DEBUG
		DWARN("%s:%d: execve failed", __func__, __LINE__);
#endif
		return -1;
		/* NOTREACHED */
		break;
	default:
		break;
	}
	
	(void)close(sv[1]);
	return sv[0];
}

int
perfuse_mount(const char *source, const char *target,
	const char *filesystemtype, long mountflags, const void *data)
{
	int s;
	size_t len;
	struct perfuse_mount_out *pmo;
	struct sockaddr_storage ss;
	struct sockaddr_un *sun;
	struct sockaddr *sa;
	socklen_t sa_len;
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
	pmo->pmo_len = (uint32_t)len;
	pmo->pmo_error = 0;
	pmo->pmo_unique = (uint64_t)-1;
	(void)strcpy(pmo->pmo_magic, PERFUSE_MOUNT_MAGIC);

	pmo->pmo_source_len = source ? (uint32_t)strlen(source) + 1 : 0;
	pmo->pmo_target_len = target ? (uint32_t)strlen(target) + 1: 0;
	pmo->pmo_filesystemtype_len = 
	    filesystemtype ? (uint32_t)strlen(filesystemtype) + 1 : 0;
	pmo->pmo_mountflags = (uint32_t)mountflags;
	pmo->pmo_data_len = data ? (uint32_t)strlen(data) + 1 : 0;
	pmo->pmo_sock_len = (uint32_t)sock_len;
	
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

	if (send(s, frame, len, MSG_NOSIGNAL) != (ssize_t)len) {
#ifdef PERFUSE_DEBUG
		DWARN("%s:%d sendto failed", __func__, __LINE__);
#endif
		return -1;
	}

	return 0;
}


uint64_t
perfuse_next_unique(struct puffs_usermount *pu)
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return ps->ps_unique++;
} 

static void
updatelimit(const char *func, int lim, const char *name)
{
	struct rlimit rl;

	/* Try infinity but that will fail unless we are root */
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
	if (setrlimit(lim, &rl) != -1)
		return;

	/* Get and set to the maximum allowed */
	if (getrlimit(lim, &rl) == -1)
		DERR(EX_OSERR, "%s: getrlimit %s failed", func, name);

	rl.rlim_cur = rl.rlim_max;
	if (setrlimit(lim, &rl) == -1)
		DERR(EX_OSERR, "%s: setrlimit %s to %ju failed", func,
		    name, (uintmax_t)rl.rlim_cur);
}

struct puffs_usermount *
perfuse_init(struct perfuse_callbacks *pc, struct perfuse_mount_info *pmi)
{
	struct perfuse_state *ps;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	const char *source = _PATH_PUFFS;
	char *fstype;
	unsigned int puffs_flags;
	struct puffs_node *pn_root;
	struct puffs_pathobj *po_root;

	/*
	 * perfused can grow quite large, let assume there's enough ram ...
	 */
	updatelimit(__func__, RLIMIT_DATA, "RLIMIT_DATA");
	updatelimit(__func__, RLIMIT_AS, "RLIMIT_AS");

	ps = init_state();
	ps->ps_owner_uid = pmi->pmi_uid;

	if (pmi->pmi_source) {
		if ((ps->ps_source = strdup(pmi->pmi_source)) == NULL)
			DERR(EX_OSERR, "%s: strdup failed", __func__);

		source = ps->ps_source;
	}

	if (pmi->pmi_filesystemtype) {
		size_t len;

		ps->ps_filesystemtype = strdup(pmi->pmi_filesystemtype);
		if (ps->ps_filesystemtype == NULL)
			DERR(EX_OSERR, "%s: strdup failed", __func__);

		len = sizeof("perfuse|") + strlen(ps->ps_filesystemtype) + 1;
		if ((fstype = malloc(len)) == NULL)
			DERR(EX_OSERR, "%s: malloc failed", __func__);

		(void)sprintf(fstype, "perfuse|%s", ps->ps_filesystemtype);
	} else {
		if ((fstype = strdup("perfuse")) == NULL)
			DERR(EX_OSERR, "%s: strdup failed", __func__);
	}

	if ((ps->ps_target = strdup(pmi->pmi_target)) == NULL)
		DERR(EX_OSERR, "%s: strdup failed", __func__);

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
	PUFFSOP_SET(pops, perfuse, node, fsync);
	PUFFSOP_SET(pops, perfuse, node, remove);
	PUFFSOP_SET(pops, perfuse, node, link);
	PUFFSOP_SET(pops, perfuse, node, rename);
	PUFFSOP_SET(pops, perfuse, node, mkdir);
	PUFFSOP_SET(pops, perfuse, node, rmdir);
	PUFFSOP_SET(pops, perfuse, node, symlink);
	PUFFSOP_SET(pops, perfuse, node, readdir);
	PUFFSOP_SET(pops, perfuse, node, readlink);
	PUFFSOP_SET(pops, perfuse, node, reclaim);
	PUFFSOP_SET(pops, perfuse, node, reclaim2);
	PUFFSOP_SET(pops, perfuse, node, inactive);
	PUFFSOP_SET(pops, perfuse, node, print);
	PUFFSOP_SET(pops, perfuse, node, pathconf);
	PUFFSOP_SET(pops, perfuse, node, advlock);
	PUFFSOP_SET(pops, perfuse, node, read);
	PUFFSOP_SET(pops, perfuse, node, write);
#ifdef PUFFS_EXTNAMELEN
	PUFFSOP_SET(pops, perfuse, node, getextattr);
	PUFFSOP_SET(pops, perfuse, node, setextattr);
	PUFFSOP_SET(pops, perfuse, node, listextattr);
	PUFFSOP_SET(pops, perfuse, node, deleteextattr);
#endif /* PUFFS_EXTNAMELEN */
#ifdef PUFFS_KFLAG_CACHE_FS_TTL
	PUFFSOP_SET(pops, perfuse, node, getattr_ttl);
	PUFFSOP_SET(pops, perfuse, node, setattr_ttl);
#endif /* PUFFS_KFLAG_CACHE_FS_TTL */
#ifdef PUFFS_SETATTR_FAF
	PUFFSOP_SET(pops, perfuse, node, write2);
#endif /* PUFFS_SETATTR_FAF */
#ifdef PUFFS_OPEN_IO_DIRECT 
	PUFFSOP_SET(pops, perfuse, node, open2);
#endif /* PUFFS_OPEN_IO_DIRECT */
#ifdef PUFFS_HAVE_FALLOCATE
	PUFFSOP_SET(pops, perfuse, node, fallocate);
#endif /* PUFFS_HAVE_FALLOCATE */

	/*
	 * PUFFS_KFLAG_NOCACHE_NAME is required so that we can see changes
	 * done by other machines in networked filesystems. In later
	 * NetBSD releases we use the alternative PUFFS_KFLAG_CACHE_FS_TTL, 
	 * which implement name cache with a filesystem-provided TTL.
	 */
#ifdef PUFFS_KFLAG_CACHE_FS_TTL
	puffs_flags = PUFFS_KFLAG_CACHE_FS_TTL;
#else
	puffs_flags = PUFFS_KFLAG_NOCACHE_NAME;
#endif

	/*
	 * Do not lookuo .. 
	 * That means we keep all parent vnode active
	 */
#ifdef PUFFS_KFLAG_CACHE_DOTDOT
	puffs_flags |= PUFFS_KFLAG_CACHE_DOTDOT;
#endif
	
	/* 
	 * It would be nice to avoid useless inactive, and only
	 * get them on file open for writing (PUFFS does 
	 * CLOSE/WRITE/INACTIVE, therefore actual close must be
	 * done at INACTIVE time). Unfortunatley, puffs_setback
	 * crashes when called on OPEN, therefore leave it for 
	 * another day.
	 */
#ifdef notyet
	puffs_flags |= PUFFS_FLAG_IAONDEMAND;
#endif

	/*
	 * FUSE filesystem do not expect [amc]time and size
	 * updates to be sent by the kernel, they do the
	 * updates on their own after other operations.
	 */
#ifdef PUFFS_KFLAG_NOFLUSH_META
	puffs_flags |= PUFFS_KFLAG_NOFLUSH_META;
#endif

	if (perfuse_diagflags & PDF_PUFFS)
		puffs_flags |= PUFFS_FLAG_OPDUMP;

	if ((pu = puffs_init(pops, source, fstype, ps, puffs_flags)) == NULL)
		DERR(EX_OSERR, "%s: puffs_init failed", __func__);

	puffs_setncookiehash(pu, PUFFS_PNODEBUCKETS);

	ps->ps_pu = pu;

	/*
	 * Setup filesystem root
	 */
	pn_root = perfuse_new_pn(pu, "", NULL);
	PERFUSE_NODE_DATA(pn_root)->pnd_nodeid = FUSE_ROOT_ID; 
	PERFUSE_NODE_DATA(pn_root)->pnd_parent_nodeid = FUSE_ROOT_ID;
	perfuse_node_cache(ps, pn_root);
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
	pn_root->pn_va.va_fileid = FUSE_ROOT_ID;
	
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
	ps->ps_umount = pc->pc_umount;

	pc->pc_fsreq = *perfuse_fsreq;

	return pu;
} 

void
perfuse_setspecific(struct puffs_usermount *pu, void *priv)
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);
	ps->ps_private = priv;	

	return;
}

void *
perfuse_getspecific(struct puffs_usermount *pu)
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return ps->ps_private;
}

int
perfuse_inloop(struct puffs_usermount *pu)
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return ps->ps_flags & PS_INLOOP;
}

int
perfuse_mainloop(struct puffs_usermount *pu)
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	ps->ps_flags |= PS_INLOOP;
	if (puffs_mainloop(ps->ps_pu) != 0) {
		DERR(EX_OSERR, "%s: failed", __func__);
		return -1;
	}

	/* 
	 * Normal exit after unmount
	 */
	return 0;
}

/* ARGSUSED0 */
uint64_t
perfuse_get_nodeid(struct puffs_usermount *pu, puffs_cookie_t opc)
{
	return PERFUSE_NODE_DATA(opc)->pnd_nodeid;
}

int
perfuse_unmount(struct puffs_usermount *pu)
{
	struct perfuse_state *ps;

	ps = puffs_getspecific(pu);

	return unmount(ps->ps_target, MNT_FORCE);
}

void         
perfuse_fsreq(struct puffs_usermount *pu, perfuse_msg_t *pm)
{
	struct perfuse_state *ps;
	struct fuse_out_header *foh;
	     
	ps = puffs_getspecific(pu);
	foh = GET_OUTHDR(ps, pm);

	/*
	 * There are some operations we may use in a  Fire and Forget way,
	 * because the kernel does not await a reply, but FUSE still
	 * sends a reply. This happens for fsyc, setattr (for metadata
	 * associated with a fsync) and write (for VOP_PUTPAGES). Ignore
	 * if it was fine, warn or abort otherwise.
	 */   
	switch (foh->error) {
	case 0:
		break;
	case -ENOENT:
		/* File disappeared during a FAF operation */
		break;
	case -ENOTCONN: /* FALLTHROUGH */
	case -EAGAIN: /* FALLTHROUGH */
	case -EMSGSIZE:
		DWARN("operation unique = %"PRId64" failed", foh->unique);
		break;
	default:
		DWARNX("Unexpected frame: unique = %"PRId64", error = %d",
		        foh->unique, foh->error);
		/* NOTREACHED */
		break;
	}

	ps->ps_destroy_msg(pm);

	return;
}
