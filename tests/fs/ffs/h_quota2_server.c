/*	$NetBSD: h_quota2_server.c,v 1.1.2.1 2011/01/20 14:25:04 bouyer Exp $	*/

/*
 * rump server for advanced quota tests
 */

#include "../common/h_fsmacros.h"

#include <err.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mount.h>

#include <stdlib.h>

#include <ufs/ufs/ufsmount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "../../h_macros.h"

static void
usage(void)
{
	fprintf(stderr, "usage: %s diskimage bindurl\n", getprogname());
	exit(1);
}

static void
die(const char *reason, int error)
{

	warnx("%s: %s", reason, strerror(error));
	//rump_daemonize_done(error);
	exit(1);
}

static sem_t sigsem;
static void
sigreboot(int sig)
{

	sem_post(&sigsem);
}

int 
main(int argc, const char *argv[])
{
	int error;
	struct ufs_args uargs;
	const char *filename;
	const char *serverurl;
	int log = 0;

	if (argc != 3)
		usage();

	filename = argv[1];
	serverurl = argv[2];

#if 0
	error = rump_daemonize_begin();
	if (error)
		errx(1, "rump daemonize: %s", strerror(error));
#endif

	error = rump_init();
	if (error)
		die("rump init failed", error);

	if (rump_sys_mkdir(FSTEST_MNTNAME, 0777) == -1)
		atf_tc_fail_errno("mount point create");
	rump_pub_etfs_register("/diskdev", filename, RUMP_ETFS_BLK);
	uargs.fspec = __UNCONST("/diskdev");
	if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, (log) ? MNT_LOG : 0,
	    &uargs, sizeof(uargs)) == -1)
		die("mount ffs", errno);

	error = rump_init_server(serverurl);
	if (error)
		die("rump server init failed", error);
	//rump_daemonize_done(RUMP_DAEMONIZE_SUCCESS);

	sem_init(&sigsem, 0, 0);
	signal(SIGTERM, sigreboot);
	signal(SIGINT, sigreboot);
	sem_wait(&sigsem);

	rump_sys_reboot(0, NULL);
	/*NOTREACHED*/
	return 0;
}
