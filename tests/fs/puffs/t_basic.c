/*	$NetBSD: t_basic.c,v 1.5 2010/07/11 12:33:38 pooka Exp $	*/

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/socket.h>

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

#include "../../h_macros.h"

struct puffs_args {
	uint8_t			*us_pargs;
	size_t			us_pargslen;

	int			us_pflags;
	int			us_servfd;
	pid_t			us_childpid;
};

#define BUFSIZE (64*1024)
#define DTFS_DUMP "-o","dump"

struct thefds {
	int rumpfd;
	int servfd;
};

int vfs_toserv_ops[PUFFS_VFS_MAX];
int vn_toserv_ops[PUFFS_VN_MAX];

/*
 * Do a synchronous operation.  When this returns, all FAF operations
 * have at least been delivered to the file system.
 *
 * XXX: is this really good enough considering puffs(9)-issued
 * callback operations?
 */
static void
syncbar(const char *fs)
{
	struct statvfs svb;

	rump_sys_statvfs1(fs, &svb, ST_WAIT);
}

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
		off_t off;
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
		if (n != phdr->pth_framelen)
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

static int
parseargs(int argc, char *argv[],
	struct puffs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	pid_t childpid;
	int *pflags = &args->us_pflags;
	char comfd[16];
	int sv[2];
	size_t len;
	ssize_t n;

	/* Create sucketpair for communication with the real file server */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) == -1)
		err(1, "socketpair");

	switch ((childpid = fork())) {
	case 0:
		close(sv[1]);
		snprintf(comfd, sizeof(sv[0]), "%d", sv[0]);
		if (setenv("PUFFS_COMFD", comfd, 1) == -1)
			atf_tc_fail_errno("setenv");

		if (execvp(argv[0], argv) == -1)
			atf_tc_fail_errno("execvp");
		/*NOTREACHED*/
	case -1:
		atf_tc_fail_errno("fork");
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

#define dtfsmountv(a, b)						\
    struct puffs_args pa;						\
    dtfsmount1(a, __arraycount(b), b, atf_tc_get_config_var(tc, "srcdir"), &pa)
static void
dtfsmount1(const char *mp, int optcount, char *opts[], const char *srcdir,
	struct puffs_args *pa)
{
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	char dtfs_path[MAXPATHLEN], mountpath[MAXPATHLEN];
	char **dtfsargv;
	int mntflag;
	int rv, i;
	int fd;

	dtfsargv = malloc(sizeof(char *) * (optcount+3));

	/* build dtfs exec path for atf */
	sprintf(dtfs_path, "%s/h_dtfs/h_dtfs", srcdir);
	dtfsargv[0] = dtfs_path;

	for (i = 0; i < optcount; i++) {
		dtfsargv[i+1] = opts[i];
	}

	strlcpy(mountpath, mp, sizeof(mountpath));
	dtfsargv[optcount+1] = mountpath;
	dtfsargv[optcount+2] = NULL;

	rv = parseargs(optcount+3, dtfsargv,
	    pa, &mntflag, canon_dev, canon_dir);
	if (rv)
		atf_tc_fail("comfd parseargs");

	rump_init();
	fd = rump_sys_open("/dev/puffs", O_RDWR);
	if (fd == -1)
		atf_tc_fail_errno("open puffs fd");
#if 0
	pa->pa_fd = fd;
#else
	assert(fd == 0); /* XXX: FIXME */
#endif

	if (rump_sys_mkdir(mp, 0777) == -1)
		atf_tc_fail_errno("mkdir mountpoint");

	if (rump_sys_mount(MOUNT_PUFFS, mp, 0,
	    pa->us_pargs, pa->us_pargslen) == -1)
		atf_tc_fail_errno("mount");

	rumpshovels(fd, pa->us_servfd);
}

ATF_TC(mount);
ATF_TC_HEAD(mount, tc)
{

	atf_tc_set_md_var(tc, "descr", "puffs+dtfs un/mount test");
}

ATF_TC_BODY(mount, tc)
{
	char *myopts[] = {
		"dtfs",
	};

	dtfsmountv("/mp", myopts);
	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TC(root_reg);
ATF_TC_HEAD(root_reg, tc)
{
	atf_tc_set_md_var(tc, "descr", "root is a regular file");
}

ATF_TC_BODY(root_reg, tc)
{
	char *myopts[] = {
		"-r","reg",
		"dtfs",
	};
	int fd, rv;

	dtfsmountv("/mp", myopts);

	fd = rump_sys_open("/mp", O_RDWR);
	if (fd == -1)
		atf_tc_fail_errno("open root");
	if (rump_sys_write(fd, &fd, sizeof(fd)) != sizeof(fd))
		atf_tc_fail_errno("write to root");
	rv = rump_sys_mkdir("/mp/test", 0777);
	ATF_REQUIRE(errno == ENOTDIR);
	ATF_REQUIRE(rv == -1);
	rump_sys_close(fd);

	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TC(root_lnk);
ATF_TC_HEAD(root_lnk, tc)
{

	atf_tc_set_md_var(tc, "descr", "root is a symbolic link");
}

#define LINKSTR "/path/to/nowhere"
ATF_TC_BODY(root_lnk, tc)
{
	char *myopts[] = {
		"-r", "lnk " LINKSTR,
		"-s",
		"dtfs",
	};
	char buf[PATH_MAX];
	ssize_t len;

	dtfsmountv("/mp", myopts);

	if ((len = rump_sys_readlink("/mp", buf, sizeof(buf)-1)) == -1)
		atf_tc_fail_errno("readlink");
	buf[len] = '\0';

	ATF_REQUIRE_STREQ(buf, LINKSTR);

#if 0 /* XXX: unmount uses FOLLOW */
	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
#endif

	/*
	 * XXX2: due to atf issue #53, we must make sure the child dies
	 * before we exit.
	 */
	if (kill(pa.us_childpid, SIGTERM) == -1)
		err(1, "kill");
}

ATF_TC(root_fifo);
ATF_TC_HEAD(root_fifo, tc)
{

	atf_tc_set_md_var(tc, "descr", "root is a symbolic link");
}

#define MAGICSTR "nakit ja muusiperunat maustevoilla"
static void *
dofifow(void *arg)
{
	int fd = (int)(uintptr_t)arg;
	char buf[512];

	printf("writing\n");
	strcpy(buf, MAGICSTR);
	if (rump_sys_write(fd, buf, strlen(buf)+1) != strlen(buf)+1)
		atf_tc_fail_errno("write to fifo");

	return NULL;
}

ATF_TC_BODY(root_fifo, tc)
{
	char *myopts[] = {
		"-r", "fifo",
		"dtfs",
	};
	pthread_t pt;
	char buf[512];
	int fd;

	dtfsmountv("/mp", myopts);
	fd = rump_sys_open("/mp", O_RDWR);
	if (fd == -1)
		atf_tc_fail_errno("open fifo");

	pthread_create(&pt, NULL, dofifow, (void *)(uintptr_t)fd);

	printf("reading\n");
	memset(buf, 0, sizeof(buf));
	if (rump_sys_read(fd, buf, sizeof(buf)) == -1)
		atf_tc_fail_errno("read fifo");

	ATF_REQUIRE_STREQ(buf, MAGICSTR);

	rump_sys_close(fd);
	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TC(root_chrdev);
ATF_TC_HEAD(root_chrdev, tc)
{

	atf_tc_set_md_var(tc, "descr", "root is /dev/null");
}

ATF_TC_BODY(root_chrdev, tc)
{
	char *myopts[] = {
		"-r", "chr 2 0",
		"dtfs",
	};
	ssize_t rv;
	char buf[512];
	int fd;

	dtfsmountv("/mp", myopts);
	fd = rump_sys_open("/mp", O_RDWR);
	if (fd == -1)
		atf_tc_fail_errno("open null");

	rv = rump_sys_write(fd, buf, sizeof(buf));
	ATF_REQUIRE(rv == sizeof(buf));

	rv = rump_sys_read(fd, buf, sizeof(buf));
	ATF_REQUIRE(rv == 0);

	rump_sys_close(fd);
	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

/*
 * Inactive/reclaim tests
 */

ATF_TC(inactive_basic);
ATF_TC_HEAD(inactive_basic, tc)
{

	atf_tc_set_md_var(tc, "descr", "inactive gets called");
}

ATF_TC_BODY(inactive_basic, tc)
{
	char *myopts[] = {
		"-i",
		"dtfs",
	};
	int fd;

	dtfsmountv("/mp", myopts);
	fd = rump_sys_open("/mp/file", O_CREAT | O_RDWR, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create");

	/* one for /mp */
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], 1);

	rump_sys_close(fd);

	/* one for /mp/file */
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], 2);

	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");

	/* one for /mp again */
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], 3);
}

ATF_TC(inactive_reclaim);
ATF_TC_HEAD(inactive_reclaim, tc)
{

	atf_tc_set_md_var(tc, "descr", "inactive/reclaim gets called");
}

ATF_TC_BODY(inactive_reclaim, tc)
{
	char *myopts[] = {
		"-i",
		"dtfs",
	};
	int fd;

	dtfsmountv("/mp", myopts);
	fd = rump_sys_open("/mp/file", O_CREAT | O_RDWR, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create");

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], 1);

	if (rump_sys_unlink("/mp/file") == -1)
		atf_tc_fail_errno("remove");

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], 2);
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 0);

	rump_sys_close(fd);
	syncbar("/mp");

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], 4);
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 1);

	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TC(reclaim_hardlink);
ATF_TC_HEAD(reclaim_hardlink, tc)
{

	atf_tc_set_md_var(tc, "descr", "reclaim gets called only after "
	    "final link is gone");
}

ATF_TC_BODY(reclaim_hardlink, tc)
{
	char *myopts[] = {
		"-i",
		"dtfs",
	};
	int fd;
	int ianow;

	dtfsmountv("/mp", myopts);
	fd = rump_sys_open("/mp/file", O_CREAT | O_RDWR, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create");

	if (rump_sys_link("/mp/file", "/mp/anotherfile") == -1)
		atf_tc_fail_errno("create link");
	rump_sys_close(fd);

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 0);

	/* unlink first hardlink */
	if (rump_sys_unlink("/mp/file") == -1)
		atf_tc_fail_errno("unlink 1");

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 0);
	ianow = vn_toserv_ops[PUFFS_VN_INACTIVE];

	/* unlink second hardlink */
	if (rump_sys_unlink("/mp/anotherfile") == -1)
		atf_tc_fail_errno("unlink 2");

	syncbar("/mp");

	ATF_REQUIRE(ianow < vn_toserv_ops[PUFFS_VN_INACTIVE]);
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 1);

	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TC(unlink_accessible);
ATF_TC_HEAD(unlink_accessible, tc)
{

	atf_tc_set_md_var(tc, "descr", "open file is accessible after "
	    "having been unlinked");
}

ATF_TC_BODY(unlink_accessible, tc)
{
	char *myopts[] = {
		"-i",
		"-o","nopagecache",
		"dtfs",
	};
	char buf[512];
	int fd, ianow;

	assert(sizeof(buf) > sizeof(MAGICSTR));

	dtfsmountv("/mp", myopts);
	fd = rump_sys_open("/mp/file", O_CREAT | O_RDWR, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create");

	if (rump_sys_write(fd, MAGICSTR, sizeof(MAGICSTR)) != sizeof(MAGICSTR))
		atf_tc_fail_errno("write");
	if (rump_sys_unlink("/mp/file") == -1)
		atf_tc_fail_errno("unlink");

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 0);
	ianow = vn_toserv_ops[PUFFS_VN_INACTIVE];

	if (rump_sys_pread(fd, buf, sizeof(buf), 0) == -1)
		atf_tc_fail_errno("read");
	rump_sys_close(fd);

	syncbar("/mp");

	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_RECLAIM], 1);
	ATF_REQUIRE_EQ(vn_toserv_ops[PUFFS_VN_INACTIVE], ianow+2);

	ATF_REQUIRE_STREQ(buf, MAGICSTR);

	if (rump_sys_unmount("/mp", 0) == -1)
		atf_tc_fail_errno("unmount");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mount);

	ATF_TP_ADD_TC(tp, root_fifo);
	ATF_TP_ADD_TC(tp, root_lnk);
	ATF_TP_ADD_TC(tp, root_reg);
	ATF_TP_ADD_TC(tp, root_chrdev);

	ATF_TP_ADD_TC(tp, inactive_basic);
	ATF_TP_ADD_TC(tp, inactive_reclaim);
	ATF_TP_ADD_TC(tp, reclaim_hardlink);
	ATF_TP_ADD_TC(tp, unlink_accessible);

	return atf_no_error();
}
