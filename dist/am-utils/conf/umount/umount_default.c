/*	$NetBSD: umount_default.c,v 1.1.1.1.4.2 2000/06/07 00:52:22 dogcow Exp $ */
/*
 * Copyright (c) 1997-2000 Erez Zadok
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
 *      %W% (Berkeley) %G%
 *
 * Id: umount_default.c,v 1.3 2000/01/12 16:44:56 ezk Exp 
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
umount_fs(char *fs_name, const char *mnttabname)
{
  mntlist *mlist, *mp, *mp_save = 0;
  int error = 0;

  mp = mlist = read_mtab(fs_name, mnttabname);

  /*
   * Search the mount table looking for
   * the correct (ie last) matching entry
   */
  while (mp) {
    if (STREQ(mp->mnt->mnt_fsname, fs_name) ||
	STREQ(mp->mnt->mnt_dir, fs_name))
      mp_save = mp;
    mp = mp->mnext;
  }

  if (mp_save) {
#ifdef DEBUG
    dlog("Trying unmount(%s)", mp_save->mnt->mnt_dir);
#endif /* DEBUG */

#ifdef MOUNT_TABLE_ON_FILE
    /*
     * This unmount may hang leaving this process with an exclusive lock on
     * /etc/mtab. Therefore it is necessary to unlock mtab, do the unmount,
     * then lock mtab (again) and reread it and finally update it.
     */
    unlock_mntlist();
#endif /* MOUNT_TABLE_ON_FILE */

    if (UNMOUNT_TRAP(mp_save->mnt) < 0) {
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
#ifdef DEBUG
	dlog("%s: unmount: %m", mp_save->mnt->mnt_dir);
#endif /* DEBUG */
	break;
      }
    }
#ifdef DEBUG
    dlog("Finished unmount(%s)", mp_save->mnt->mnt_dir);
#endif /* DEBUG */

#ifdef MOUNT_TABLE_ON_FILE
    if (!error) {
      free_mntlist(mlist);
      mp = mlist = read_mtab(fs_name, mnttabname);

      /*
       * Search the mount table looking for
       * the correct (ie last) matching entry
       */
      mp_save = 0;
      while (mp) {
	if (STREQ(mp->mnt->mnt_fsname, fs_name) ||
	    STREQ(mp->mnt->mnt_dir, fs_name))
	  mp_save = mp;
	mp = mp->mnext;
      }

      if (mp_save) {
	mnt_free(mp_save->mnt);
	mp_save->mnt = 0;
	rewrite_mtab(mlist, mnttabname);
      }
    }
#endif /* MOUNT_TABLE_ON_FILE */

  } else {

    plog(XLOG_ERROR, "Couldn't find how to unmount %s", fs_name);
    /*
     * Assume it is already unmounted
     */
    error = 0;
  } /* end of "if (mp_save)" statement */

  free_mntlist(mlist);

  return error;
}
