/*	$NetBSD: h_quota2_tests.c,v 1.1.2.1 2011/02/02 19:17:08 bouyer Exp $	*/

/*
 * rump server for advanced quota tests
 * this one includes functions to run against the filesystem before
 * starting to handle rump requests from clients.
 */

#include "../common/h_fsmacros.h"

#include <err.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mount.h>

#include <stdlib.h>
#include <unistd.h>

#include <ufs/ufs/ufsmount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "../../h_macros.h"

int background = 0;

#define TEST_NONROOT_ID 1

static int
quota_test0(void)
{
	static char buf[512];
	int fd;
	int error;
	rump_sys_chown(".", TEST_NONROOT_ID, TEST_NONROOT_ID);
	if (rump_sys_setegid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_setegid");
		return error;
	}
	if (rump_sys_seteuid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_seteuid");
		return error;
	}
	fd = rump_sys_open("test_fillup", O_CREAT | O_RDWR, 0644);
	if (fd < 0) {
		error = errno;
		perror("rump_sys_open");
	} else {
		while (rump_sys_write(fd, buf, sizeof(buf)) == sizeof(buf))
			error = 0;
		error = errno;
	}
	rump_sys_close(fd);
	rump_sys_seteuid(0);
	rump_sys_setegid(0);
	return error;
}

static int
quota_test1(void)
{
	static char buf[512];
	int fd;
	int error;
	rump_sys_chown(".", TEST_NONROOT_ID, TEST_NONROOT_ID);
	if (rump_sys_setegid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_setegid");
		return error;
	}
	if (rump_sys_seteuid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_seteuid");
		return error;
	}
	fd = rump_sys_open("test_fillup", O_CREAT | O_RDWR, 0644);
	if (fd < 0) {
		error = errno;
		perror("rump_sys_open");
	} else {
		/*
		 * write up to the soft limit, wait a bit, an try to
		 * keep on writing
		 */
		int i;

		/* write 1.5k: with the directory this makes 2K */
		for (i = 0; i < 3; i++) {
			error = rump_sys_write(fd, buf, sizeof(buf));
			if (error != sizeof(buf))
				err(1, "write failed early");
		}
		sleep(2);
		/* now try to write an extra .5k */
		if (rump_sys_write(fd, buf, sizeof(buf)) != sizeof(buf))
			error = errno;
		else
			error = 0;
	}
	rump_sys_close(fd);
	rump_sys_seteuid(0);
	rump_sys_setegid(0);
	return error;
}

static int
quota_test2(void)
{
	static char buf[512];
	int fd;
	int error;
	int i;
	rump_sys_chown(".", TEST_NONROOT_ID, TEST_NONROOT_ID);
	if (rump_sys_setegid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_setegid");
		return error;
	}
	if (rump_sys_seteuid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_seteuid");
		return error;
	}

	for (i = 0; ; i++) {
		sprintf(buf, "file%d", i);
		fd = rump_sys_open(buf, O_CREAT | O_RDWR, 0644);
		if (fd < 0)
			break;
		sprintf(buf, "test file no %d", i);
		rump_sys_write(fd, buf, strlen(buf));
		rump_sys_close(fd);
	}
	error = errno;
	
	rump_sys_close(fd);
	rump_sys_seteuid(0);
	rump_sys_setegid(0);
	return error;
}

static int
quota_test3(void)
{
	static char buf[512];
	int fd;
	int error;
	int i;
	rump_sys_chown(".", TEST_NONROOT_ID, TEST_NONROOT_ID);
	if (rump_sys_setegid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_setegid");
		return error;
	}
	if (rump_sys_seteuid(TEST_NONROOT_ID) != 0) {
		error = errno;
		perror("rump_sys_seteuid");
		return error;
	}

	/*
	 * create files up to the soft limit: one less as we already own the
	 * root directory
	 */
	for (i = 0; i < 3; i++) {
		sprintf(buf, "file%d", i);
		fd = rump_sys_open(buf, O_EXCL| O_CREAT | O_RDWR, 0644);
		if (fd < 0)
			err(1, "file create failed early");
		sprintf(buf, "test file no %d", i);
		rump_sys_write(fd, buf, strlen(buf));
		rump_sys_close(fd);
	}
	/* now create an extra file after grace time: this should fail */
	sleep(2);
	sprintf(buf, "file%d", i);
	fd = rump_sys_open(buf, O_EXCL| O_CREAT | O_RDWR, 0644);
	if (fd < 0)
		error = errno;
	else
		error = 0;
	
	rump_sys_close(fd);
	rump_sys_seteuid(0);
	rump_sys_setegid(0);
	return error;
}

struct quota_test {
	int (*func)(void);
	const char *desc;
};

struct quota_test quota_tests[] = {
	{ quota_test0, "write up to hard limit"},
	{ quota_test1, "write beyond the soft limit after grace time"},
	{ quota_test2, "create file up to hard limit"},
	{ quota_test3, "create file beyond the soft limit after grace time"},
};

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-b] test# diskimage bindurl\n",
	    getprogname());
	exit(1);
}

static void
die(const char *reason, int error)
{

	warnx("%s: %s", reason, strerror(error));
	if (background)
		rump_daemonize_done(error);
	exit(1);
}

static sem_t sigsem;
static void
sigreboot(int sig)
{

	sem_post(&sigsem);
}

int 
main(int argc, char **argv)
{
	int error;
	u_long test;
	char *end;
	struct ufs_args uargs;
	const char *filename;
	const char *serverurl;
	int log = 0;
	int ch;

	while ((ch = getopt(argc, argv, "b")) != -1) {
		switch(ch) {
		case 'b':
			background = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 3)
		usage();

	filename = argv[1];
	serverurl = argv[2];

	test = strtoul(argv[0], &end, 10);
	if (*end != '\0') {
		usage();
	}
	if (test > sizeof(quota_tests) / sizeof(quota_tests[0])) {
		fprintf(stderr, "test number %lu too big\n", test);
		exit(1);
	}

	if (background) {
		error = rump_daemonize_begin();
		if (error)
			errx(1, "rump daemonize: %s", strerror(error));
	}

	error = rump_init();
	if (error)
		die("rump init failed", error);

	if (rump_sys_mkdir(FSTEST_MNTNAME, 0777) == -1)
		err(1, "mount point create");
	rump_pub_etfs_register("/diskdev", filename, RUMP_ETFS_BLK);
	uargs.fspec = __UNCONST("/diskdev");
	if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, (log) ? MNT_LOG : 0,
	    &uargs, sizeof(uargs)) == -1)
		die("mount ffs", errno);

	if (rump_sys_chdir(FSTEST_MNTNAME) == -1)
		err(1, "cd %s", FSTEST_MNTNAME);
	rump_sys_chown(".", 0, 0);
	rump_sys_chmod(".", 0777);
	error = quota_tests[test].func();
	if (error) {
		fprintf(stderr, " test %lu: %s returned %d: %s\n",
		    test, quota_tests[test].desc, error, strerror(error));
	}
	if (rump_sys_chdir("/") == -1)
		err(1, "cd /");

	error = rump_init_server(serverurl);
	if (error)
		die("rump server init failed", error);
	if (background)
		rump_daemonize_done(RUMP_DAEMONIZE_SUCCESS);

	sem_init(&sigsem, 0, 0);
	signal(SIGTERM, sigreboot);
	signal(SIGINT, sigreboot);
	sem_wait(&sigsem);

	rump_sys_reboot(0, NULL);
	/*NOTREACHED*/
	return 0;
}
