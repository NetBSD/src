/*	$NetBSD: umount_default.c,v 1.1.1.6 2003/03/09 01:13:33 christos Exp $	*/

/*
 * Copyright (c) 1997-2003 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgment:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Id: umount_default.c,v 1.9 2002/12/27 22:44:03 ezk Exp
 *
 */

/*
 * Default method of unmounting filesystems.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>


int
umount_fs(char *mntdir, const char *mnttabname)
{
  return umount_fs2(mntdir, mntdir, mnttabname);
}

int
umount_fs2(char *mntdir, char *real_mntdir, const char *mnttabname)
{
  mntlist *mlist, *mp, *mp_save = 0;
  int error = 0;
  char *mnt_dir_save;

  mp = mlist = read_mtab(mntdir, mnttabname);

  /*
   * Search the mount table looking for
   * the correct (ie last) matching entry
   */
  while (mp) {
    if (STREQ(mp->mnt->mnt_dir, mntdir))
      mp_save = mp;
    mp = mp->mnext;
  }

  if (mp_save) {
    dlog("Trying unmount(%s)", mp_save->mnt->mnt_dir);

#ifdef MOUNT_TABLE_ON_FILE
    /*
     * This unmount may hang leaving this process with an exclusive lock on
     * /etc/mtab. Therefore it is necessary to unlock mtab, do the unmount,
     * then lock mtab (again) and reread it and finally update it.
     */
    unlock_mntlist();
#endif /* MOUNT_TABLE_ON_FILE */

    mnt_dir_save = mp_save->mnt->mnt_dir;
    mp_save->mnt->mnt_dir = real_mntdir;
    error = UNMOUNT_TRAP(mp_save->mnt);
    mp_save->mnt->mnt_dir = mnt_dir_save;
    if (error < 0) {
      switch (error = errno) {
      case EINVAL:
      case ENOTBLK:
	plog(XLOG_WARNING, "unmount: %s is not mounted", mp_save->mnt->mnt_dir);
	error = 0;		/* Not really an error */
	break;

      case ENOENT:
	plog(XLOG_ERROR, "mount point %s: %m", mp_save->mnt->mnt_dir);
	error = 0;		/* Not really an error (?) */
	break;

      default:
	dlog("%s: unmount: %m", mp_save->mnt->mnt_dir);
	break;
      }
    }
    dlog("Finished unmount(%s)", mp_save->mnt->mnt_dir);

    if (!error) {
#ifdef HAVE_LOOP_DEVICE
      /* look for loop=/dev/loopX in mnt_opts */
      char *opt;
      char loopstr[] = "loop=";
      char *loopdev;
      for (opt = strtok(mp_save->mnt->mnt_opts, ","); opt; opt = strtok(NULL, ","))
	if (NSTREQ(opt, loopstr, sizeof(loopstr) - 1)) {
	  loopdev = opt + sizeof(loopstr) - 1;
	  if (delete_loop_device(loopdev) < 0)
	    plog(XLOG_WARNING, "unmount() failed to release loop device %s: %m", loopdev);
	  else
	    plog(XLOG_INFO, "unmount() released loop device %s OK", loopdev);
	  break;
	}
#endif /* HAVE_LOOP_DEVICE */

#ifdef MOUNT_TABLE_ON_FILE
      free_mntlist(mlist);
      mp = mlist = read_mtab(mntdir, mnttabname);

      /*
       * Search the mount table looking for
       * the correct (ie last) matching entry
       */
      mp_save = 0;
      while (mp) {
	if (STREQ(mp->mnt->mnt_dir, mntdir))
	  mp_save = mp;
	mp = mp->mnext;
      }

      if (mp_save) {
	mnt_free(mp_save->mnt);
	mp_save->mnt = 0;
	rewrite_mtab(mlist, mnttabname);
      }
#endif /* MOUNT_TABLE_ON_FILE */
    }

  } else {

    plog(XLOG_ERROR, "Couldn't find how to unmount %s", mntdir);
    /*
     * Assume it is already unmounted
     */
    error = 0;
  } /* end of "if (mp_save)" statement */

  free_mntlist(mlist);

  return error;
}
