/*	$NetBSD: puffs.c,v 1.3 2010/07/19 16:09:08 pooka Exp $	*/

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/wait.h>

#include <assert.h>
#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <puffs.h>
#include <puffsdump.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_fsmacros.h"

struct puffstestargs {
	uint8_t			*pta_pargs;
	size_t			pta_pargslen;

	int			pta_pflags;
	int			pta_servfd;
	pid_t			pta_childpid;

	char			pta_dev[MAXPATHLEN];
	char			pta_dir[MAXPATHLEN];

	int			pta_mntflags;
};

#define BUFSIZE (128*1024)
#define DTFS_DUMP "-o","dump"

struct thefds {
	int rumpfd;
	int servfd;
};

int vfs_toserv_ops[PUFFS_VFS_MAX];
int vn_toserv_ops[PUFFS_VN_MAX];

#ifdef PUFFSDUMP
static void __unused
dumpopcount(void)
{
	size_t i;

	printf("VFS OPS:\n");
	for (i = 0; i < MIN(puffsdump_vfsop_count, PUFFS_VFS_MAX); i++) {
		printf("\t%s: %d\n",
		    puffsdump_vfsop_revmap[i], vfs_toserv_ops[i]);
	}

	printf("VN OPS:\n");
	for (i = 0; i < MIN(puffsdump_vnop_count, PUFFS_VN_MAX); i++) {
		printf("\t%s: %d\n",
		    puffsdump_vnop_revmap[i], vn_toserv_ops[i]);
	}
}
#endif

/*
 * Threads which shovel data between comfd and /dev/puffs.
 * (cannot use polling since fd's are in different namespaces)
 */
static void *
readshovel(void *arg)
{
	struct putter_hdr *phdr;
	struct puffs_req *preq;
	struct thefds *fds = arg;
	char buf[BUFSIZE];
	int comfd, puffsfd;

	comfd = fds->servfd;
	puffsfd = fds->rumpfd;

	phdr = (void *)buf;
	preq = (void *)buf;

	/* use static thread id */
	rump_pub_lwp_alloc_and_switch(0, 10);

	for (;;) {
		ssize_t n;

		n = rump_sys_read(puffsfd, buf, sizeof(*phdr));
		if (n <= 0)
			break;

		assert(phdr->pth_framelen < BUFSIZE);
		n = rump_sys_read(puffsfd, buf+sizeof(*phdr), 
		    phdr->pth_framelen - sizeof(*phdr));
		if (n <= 0)
			break;

		/* Analyze request */
		if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VFS) {
			assert(preq->preq_optype < PUFFS_VFS_MAX);
			vfs_toserv_ops[preq->preq_optype]++;
		} else if (PUFFSOP_OPCLASS(preq->preq_opclass) == PUFFSOP_VN) {
			assert(preq->preq_optype < PUFFS_VN_MAX);
			vn_toserv_ops[preq->preq_optype]++;
		}

		n = phdr->pth_framelen;
		if (write(comfd, buf, n) != n)
			break;
	}

	return NULL;
}

static void *
writeshovel(void *arg)
{
	struct thefds *fds = arg;
	struct putter_hdr *phdr;
	char buf[BUFSIZE];
	size_t toread;
	int comfd, puffsfd;

	/* use static thread id */
	rump_pub_lwp_alloc_and_switch(0, 11);

	comfd = fds->servfd;
	puffsfd = fds->rumpfd;

	phdr = (struct putter_hdr *)buf;

	for (;;) {
		uint64_t off;
		ssize_t n;

		/*
		 * Need to write everything to the "kernel" in one chunk,
		 * so make sure we have it here.
		 */
		off = 0;
		toread = sizeof(struct putter_hdr);
		assert(toread < BUFSIZE);
		do {
			n = read(comfd, buf+off, toread);
			if (n <= 0) {
				break;
			}
			off += n;
			if (off >= sizeof(struct putter_hdr))
				toread = phdr->pth_framelen - off;
			else
				toread = off - sizeof(struct putter_hdr);
		} while (toread);

		n = rump_sys_write(puffsfd, buf, phdr->pth_framelen);
		if ((size_t)n != phdr->pth_framelen)
			break;
	}

	return NULL;
}

static void
rumpshovels(int rumpfd, int servfd)
{
	struct thefds *fds;
	pthread_t pt;
	int rv;

	if ((rv = rump_init()) == -1)
		err(1, "rump_init");

	fds = malloc(sizeof(*fds));
	fds->rumpfd = rumpfd;
	fds->servfd = servfd;
	if (pthread_create(&pt, NULL, readshovel, fds) == -1)
		err(1, "read shovel");
	pthread_detach(pt);
	if (pthread_create(&pt, NULL, writeshovel, fds) == -1)
		err(1, "write shovel");
	pthread_detach(pt);
}

/* XXX: we don't support size */
int
puffs_fstest_newfs(const atf_tc_t *tc, void **argp,
	const char *image, off_t size)
{
	struct puffstestargs *args;
	char dtfs_path[MAXPATHLEN];
	char *dtfsargv[5];
	pid_t childpid;
	int *pflags;
	char comfd[16];
	int sv[2];
	int mntflags;
	size_t len;
	ssize_t n;

	*argp = NULL;

	args = malloc(sizeof(*args));
	if (args == NULL)
		return errno;

	pflags = &args->pta_pflags;

	/* build dtfs exec path from atf test dir */
	sprintf(dtfs_path, "%s/../puffs/h_dtfs/h_dtfs",
	    atf_tc_get_config_var(tc, "srcdir"));
	dtfsargv[0] = dtfs_path;
	dtfsargv[1] = __UNCONST("-i");
	dtfsargv[2] = __UNCONST("dtfs");
	dtfsargv[3] = __UNCONST("fictional");
	dtfsargv[4] = NULL;

	/* Create sucketpair for communication with the real file server */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) == -1)
		return errno;

	switch ((childpid = fork())) {
	case 0:
		close(sv[1]);
		snprintf(comfd, sizeof(sv[0]), "%d", sv[0]);
		if (setenv("PUFFS_COMFD", comfd, 1) == -1)
			return errno;

		if (execvp(dtfsargv[0], dtfsargv) == -1)
			return errno;
	case -1:
		return errno;
	default:
		close(sv[0]);
		break;
	}

	/* read args */
	if ((n = read(sv[1], &len, sizeof(len))) != sizeof(len))
		err(1, "mp 1 %zd", n);
	if (len > MAXPATHLEN)
		err(1, "mntpath > MAXPATHLEN");
	if ((size_t)read(sv[1], args->pta_dir, len) != len)
		err(1, "mp 2");
	if (read(sv[1], &len, sizeof(len)) != sizeof(len))
		err(1, "fn 1");
	if (len > MAXPATHLEN)
		err(1, "devpath > MAXPATHLEN");
	if ((size_t)read(sv[1], args->pta_dev, len) != len)
		err(1, "fn 2");
	if (read(sv[1], &mntflags, sizeof(mntflags)) != sizeof(mntflags))
		err(1, "mntflags");
	if (read(sv[1], &args->pta_pargslen, sizeof(args->pta_pargslen)) != sizeof(args->pta_pargslen))
		err(1, "puffstest_args len");
	args->pta_pargs = malloc(args->pta_pargslen);
	if (args->pta_pargs == NULL)
		err(1, "malloc");
	if (read(sv[1], args->pta_pargs, args->pta_pargslen) != (ssize_t)args->pta_pargslen)
		err(1, "puffstest_args");
	if (read(sv[1], pflags, sizeof(*pflags)) != sizeof(*pflags))
		err(1, "pflags");

	args->pta_childpid = childpid;
	args->pta_servfd = sv[1];
	strlcpy(args->pta_dev, image, sizeof(args->pta_dev));

	*argp = args;

	return 0;
}

int
puffs_fstest_mount(const atf_tc_t *tc, void *arg, const char *path, int flags)
{
	struct puffstestargs *pargs = arg;
	int fd;

	rump_init();
	fd = rump_sys_open("/dev/puffs", O_RDWR);
	if (fd == -1)
		return fd;

#if 0
	pa->pa_fd = fd;
#else
	assert(fd == 0); /* XXX: FIXME */
#endif

	if (rump_sys_mkdir(path, 0777) == -1)
		return -1;

	if (rump_sys_mount(MOUNT_PUFFS, path, flags,
	    pargs->pta_pargs, pargs->pta_pargslen) == -1) {
		printf("%d\n", errno);
		return -1;
	}

	rumpshovels(fd, pargs->pta_servfd);

	return 0;
}

int
puffs_fstest_delfs(const atf_tc_t *tc, void *arg)
{
	struct puffstestargs *pargs = arg;
	int status;

	if (waitpid(pargs->pta_childpid, &status, WNOHANG) > 0)
		return 0;
	kill(pargs->pta_childpid, SIGTERM);
	usleep(10);
	if (waitpid(pargs->pta_childpid, &status, WNOHANG) > 0)
		return 0;
	kill(pargs->pta_childpid, SIGKILL);
	usleep(500);
	if (waitpid(pargs->pta_childpid, &status, 0) == -1)
		return errno;
	return 0;
}

int
puffs_fstest_unmount(const atf_tc_t *tc, const char *path, int flags)
{
	int rv;

	rv = rump_sys_unmount(path, flags);
	if (rv)	
		return rv;

	return rump_sys_rmdir(path);
}
