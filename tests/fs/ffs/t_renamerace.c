/*	$NetBSD: t_renamerace.c,v 1.6.2.2 2009/05/13 19:19:22 jym Exp $	*/

/*
 * Modified for rump and atf from a program supplied
 * by Nicolas Joly in kern/40948
 */

#include <sys/types.h>
#include <sys/mount.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <rump/ukfs.h>

#include <ufs/ufs/ufsmount.h>

ATF_TC_WITH_CLEANUP(renamerace);
ATF_TC_HEAD(renamerace, tc)
{
	atf_tc_set_md_var(tc, "descr", "rename(2) race against files "
	    "unlinked mid-operation, kern/40948");
}

int quit;

static void *
w1(void *arg)
{
	int fd;

	while (!quit) {
		fd = rump_sys_open("/rename.test1",
		    O_WRONLY|O_CREAT|O_TRUNC, 0666);
		rump_sys_unlink("/rename.test1");
		rump_sys_close(fd);
	}
	return NULL;
}

static void *
w2(void *arg)
{

	while (!quit)
		rump_sys_rename("/rename.test1", "/rename.test2");

	return NULL;
}

/* XXX: how to do cleanup if we use mkdtemp? */
#define IMAGENAME "/tmp/ffsatf.img"
static char image[MAXPATHLEN];

ATF_TC_BODY(renamerace, tc)
{
	struct ufs_args args;
	char cmd[256];
	struct ukfs *fs;
	pthread_t pt1, pt2;

#if 0
	strcpy(image, TMPPATH);
	if (mkdtemp(image) == NULL)
		atf_tc_fail("can't create tmpdir %s: %d (%s)",
		    TMPPATH, errno, strerror(errno));
	strcat(image, "/ffsatf.img");
#else
	strcpy(image, IMAGENAME);
#endif

	strcpy(cmd, "newfs -F -s 10000 ");
	strcat(cmd, image);

	if (system(cmd) == -1)
		atf_tc_fail("newfs failed: %d (%s)", errno, strerror(errno));

	memset(&args, 0, sizeof(args));
	args.fspec = image;

	ukfs_init();
	fs = ukfs_mount(MOUNT_FFS, "ffs", UKFS_DEFAULTMP,
	    MNT_LOG, &args, sizeof(args));
	if (fs == NULL)
		atf_tc_fail("ukfs_mount failed: %d (%s)",
		    errno, strerror(errno));

	pthread_create(&pt1, NULL, w1, fs);
	pthread_create(&pt2, NULL, w2, fs);

	sleep(10);

	quit = 1;
	pthread_join(pt1, NULL);
	pthread_join(pt2, NULL);

	ukfs_release(fs, 0);
}

ATF_TC_CLEANUP(renamerace, tc)
{
#if 0
	char *img;

	img = strrchr(image, '/');
	if (!img)
		return;

	printf("removing %s\n", img+1);
	unlink(img+1);
	*img = '\0';
	rmdir(img);
#else
	unlink(IMAGENAME);
#endif
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, renamerace);
	return 0; /* ??? */
}
