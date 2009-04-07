/*	$NetBSD: t_renamerace.c,v 1.1 2009/04/07 20:51:46 pooka Exp $	*/

/*
 * Modified for rump and atf from a program supplied
 * by Nicolas Joly in kern/40948
 */

#include <sys/types.h>
#include <sys/mount.h>

#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <rump/ukfs.h>

#include <fs/tmpfs/tmpfs_args.h>

#define NROUND (1<<16) /* usually triggered with this amount of rounds */

ATF_TC(renamerace);
ATF_TC_HEAD(renamerace, tc)
{
	atf_tc_set_md_var(tc, "descr", "rename(2) race against files "
	    "unlinked mid-operation, kern/41128");
}

void *
w1(void *arg)
{
  int fd, i;

  for (i = 0; i < NROUND; i++) {
    fd = rump_sys_open("/rename.test1", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    rump_sys_unlink("/rename.test1");
    rump_sys_close(fd);
  }
  return NULL;
}

ATF_TC_BODY(renamerace, tc)
{
  struct tmpfs_args args;
  struct ukfs *fs;
  pthread_t pt;
  int fail = 0, succ = 0, i;

  memset(&args, 0, sizeof(args));
  args.ta_version = TMPFS_ARGS_VERSION;
  args.ta_root_mode = 0777;

  ukfs_init();
  fs = ukfs_mount(MOUNT_TMPFS, "tmpfs", UKFS_DEFAULTMP, 0, &args, sizeof(args));
  if (fs == NULL)
    err(1, "ukfs_mount");

  pthread_create(&pt, NULL, w1, fs);

  for (i = 0; i < NROUND; i++) {
    int rv;
    rv = rump_sys_rename("/rename.test1", "/rename.test2");
#if 0
    if (rv == 0) {
      if (succ++ % 10000 == 0)
        printf("success\n");
    } else {
      if (fail++ % 10000 == 0)
        printf("fail\n");
    }
#endif
  }

  pthread_join(pt, NULL);
  ukfs_release(fs, 0);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, renamerace);
}
