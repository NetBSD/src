/*	$NetBSD: t_basic.c,v 1.2 2010/07/06 15:42:24 pooka Exp $	*/

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/socket.h>

#include <assert.h>
#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <puffs.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "../../h_macros.h"

struct puffs_args {
	uint8_t			*us_pargs;
	size_t			us_pargslen;

	int			us_pflags;
	int			us_servfd;
	pid_t			us_childpid;
};

#define BUFSIZE (64*1024)

struct thefds {
	int rumpfd;
	int servfd;
};

/*
 * Threads which shovel data between comfd and /dev/puffs.
 * (cannot use polling since fd's are in different namespaces)
 */
static void *
readshovel(void *arg)
{
	struct thefds *fds = arg;
	char buf[BUFSIZE];
	ssize_t n;
	int error, comfd, puffsfd;

	comfd = fds->servfd;
	puffsfd = fds->rumpfd;

	/* use static thread id */
	rump_pub_lwp_alloc_and_switch(0, 0);

	for (;;) {
		ssize_t n, n2;

		n = rump_sys_read(puffsfd, buf, BUFSIZE);
		if (n <= 0) {
			break;
		}

		while (n) {
			n2 = write(comfd, buf, n);
			if (n2 == -1)
				err(1, "readshovel failed write: %d");
			if (n2 == 0)
				break;
			n -= n2;
		}
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
	int error, comfd, puffsfd;

	/* use static thread id */
	rump_pub_lwp_alloc_and_switch(0, 0);

	comfd = fds->servfd;
	puffsfd = fds->rumpfd;

	phdr = (struct putter_hdr *)buf;

	for (;;) {
		off_t off;
		ssize_t n;

		/*
		 * Need to write everything to the "kernel" in one chunk,
		 * so make sure we have it here.
		 */
		off = 0;
		toread = sizeof(struct putter_hdr);
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
		if (n != phdr->pth_framelen)
			goto out;
	}

 out:
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

static int
parseargs(int argc, char *argv[],
	struct puffs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	pid_t childpid;
	pthread_t pt;
	int *pflags = &args->us_pflags;
	char comfd[16];
	int sv[2];
	size_t len;
	ssize_t n;
	int rv;

	if (argc < 2)
		errx(1, "invalid usage");

	/* Create sucketpair for communication with the real file server */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) == -1)
		err(1, "socketpair");

	switch ((childpid = fork())) {
	case 0:
		close(sv[1]);
		snprintf(comfd, sizeof(sv[0]), "%d", sv[0]);
		if (setenv("PUFFS_COMFD", comfd, 1) == -1)
			err(1, "setenv");

		argv++;
		if (execvp(argv[0], argv) == -1)
			err(1, "execvp here");
		/*NOTREACHED*/
	case -1:
		err(1, "fork");
		/*NOTREACHED*/
	default:
		close(sv[0]);
		break;
	}

	/* read args */
	if ((n = read(sv[1], &len, sizeof(len))) != sizeof(len))
		err(1, "mp 1 %zd", n);
	if (len > MAXPATHLEN)
		err(1, "mntpath > MAXPATHLEN");
	if ((size_t)read(sv[1], canon_dir, len) != len)
		err(1, "mp 2");
	if (read(sv[1], &len, sizeof(len)) != sizeof(len))
		err(1, "fn 1");
	if (len > MAXPATHLEN)
		err(1, "devpath > MAXPATHLEN");
	if ((size_t)read(sv[1], canon_dev, len) != len)
		err(1, "fn 2");
	if (read(sv[1], mntflags, sizeof(*mntflags)) != sizeof(*mntflags))
		err(1, "mntflags");
	if (read(sv[1], &args->us_pargslen, sizeof(args->us_pargslen)) != sizeof(args->us_pargslen))
		err(1, "puffs_args len");
	args->us_pargs = malloc(args->us_pargslen);
	if (args->us_pargs == NULL)
		err(1, "malloc");
	if (read(sv[1], args->us_pargs, args->us_pargslen) != args->us_pargslen)
		err(1, "puffs_args");
	if (read(sv[1], pflags, sizeof(*pflags)) != sizeof(*pflags))
		err(1, "pflags");

	args->us_childpid = childpid;
	args->us_servfd = sv[1];

	return 0;
}

ATF_TC(mount);
ATF_TC_HEAD(mount, tc)
{

	atf_tc_set_md_var(tc, "descr", "puffs+dtfs un/mount test");
}

char *dtfsargv[] = {
	"dtfs",
	"BUILT_AT_RUNTIME",
	"dtfs",
	"/mp",
	NULL
};

ATF_TC_BODY(mount, tc)
{
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	char dtfs_path[MAXPATHLEN];
	struct puffs_args pargs;
	int mntflag;
	int rv;
	int fd;

	/* build dtfs exec path for atf */
	sprintf(dtfs_path, "%s/h_dtfs/h_dtfs",
	    atf_tc_get_config_var(tc, "srcdir"));
	dtfsargv[1] = dtfs_path;

	rv = parseargs(__arraycount(dtfsargv), dtfsargv,
	    &pargs, &mntflag, canon_dev, canon_dir);
	if (rv)
		atf_tc_fail("comfd parseargs");

	rump_init();
	fd = rump_sys_open("/dev/puffs", O_RDWR);
	if (fd == -1)
		atf_tc_fail_errno("open puffs fd");
#if 0
	pargs->pa_fd = fd;
#else
	assert(fd == 0); /* XXX: FIXME */
#endif

	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("mkdir mountpoint");

	if (rump_sys_mount(MOUNT_PUFFS, "/mp", 0,
	    pargs.us_pargs, pargs.us_pargslen) == -1)
		atf_tc_fail_errno("mount");

	rumpshovels(fd, pargs.us_servfd);

	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, mount);

	return atf_no_error();
}
