/*	$NetBSD: checkmount_default.c,v 1.1.1.6 2003/03/09 01:13:21 christos Exp $	*/

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
 * Id: checkmount_default.c,v 1.7 2002/12/27 22:43:55 ezk Exp
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>

#ifndef _PATH_MTAB
#define _PATH_MTAB "/etc/mtab"
#endif /* not _PATH_MTAB */

extern int is_same_host(char *name1, char *name2, struct in_addr addr2);


int
fixmount_check_mount(char *host, struct in_addr hostaddr, char *path)
{
  FILE *mtab;
  mntent_t *ment;
  int found = 0;

  /* scan mtab for path */
  if (!(mtab = setmntent(_PATH_MTAB, "r"))) {
    perror(_PATH_MTAB);
    exit(1);
  }

  /*
   * setmntent() doesn't do locking in read-only mode. Too bad -- it seems to
   * rely on mount() and friends to do atomic updates by renaming the file.
   * Well, our patched amd rewrites mtab in place to avoid NFS lossage, so
   * better do the locking ourselves.
   */
#ifdef HAVE_FLOCK
  if (flock(fileno(mtab), LOCK_SH) < 0) {
#else /* not HAVE_FLOCK */
  if (lockf(fileno(mtab), F_LOCK, 0) < 0) {
#endif /* not HAVE_FLOCK */
    perror(_PATH_MTAB);
    exit(1);
  }

  while (!found && (ment = getmntent(mtab))) {
    char *colon;

    if ((colon = strchr(ment->mnt_fsname, ':'))) {
      *colon = '\0';
      if ((STREQ(colon + 1, path) ||
	   STREQ(ment->mnt_dir, path)) &&
	  is_same_host(ment->mnt_fsname, host, hostaddr))
	  found = 1;
    }
  }

  (void) endmntent(mtab);

  if (!found) {
    char *swap;

    /* swap files never show up in mtab, only root fs */
    if ((swap = strstr(path, "swap"))) {
      strncpy(swap, "root", 4);
      found = fixmount_check_mount(host, hostaddr, path);
      strncpy(swap, "swap", 4);
    }
  }
  return found;
}
