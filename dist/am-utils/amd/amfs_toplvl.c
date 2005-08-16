/*	$NetBSD: amfs_toplvl.c,v 1.3.2.1 2005/08/16 13:02:13 tron Exp $	*/

/*
 * Copyright (c) 1997-2005 Erez Zadok
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
 * Id: amfs_toplvl.c,v 1.38 2005/04/17 03:05:54 ezk Exp
 *
 */

/*
 * Top-level file system
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

/****************************************************************************
 *** FORWARD DEFINITIONS                                                  ***
 ****************************************************************************/


/****************************************************************************
 *** OPS STRUCTURES                                                       ***
 ****************************************************************************/
am_ops amfs_toplvl_ops =
{
  "toplvl",
  amfs_generic_match,
  0,				/* amfs_toplvl_init */
  amfs_toplvl_mount,
  amfs_toplvl_umount,
  amfs_generic_lookup_child,
  amfs_generic_mount_child,
  amfs_generic_readdir,
  0,				/* amfs_toplvl_readlink */
  amfs_generic_mounted,
  0,				/* amfs_toplvl_umounted */
  amfs_generic_find_srvr,
  0,				/* amfs_toplvl_get_wchan */
  FS_MKMNT | FS_NOTIMEOUT | FS_BACKGROUND |
	  FS_AMQINFO | FS_DIRECTORY, /* nfs_fs_flags */
#ifdef HAVE_FS_AUTOFS
  AUTOFS_TOPLVL_FS_FLAGS,
#endif /* HAVE_FS_AUTOFS */
};


/****************************************************************************
 *** FUNCTIONS                                                             ***
 ****************************************************************************/

static void
set_auto_attrcache_timeout(char *preopts, char *opts)
{

#ifdef MNTTAB_OPT_NOAC
  /*
   * Don't cache attributes - they are changing under the kernel's feet.
   * For example, IRIX5.2 will dispense with nfs lookup calls and hand stale
   * filehandles to getattr unless we disable attribute caching on the
   * automount points.
   */
  if (gopt.auto_attrcache == 0) {
    sprintf(preopts, ",%s", MNTTAB_OPT_NOAC);
    strcat(opts, preopts);
  }
#endif /* MNTTAB_OPT_NOAC */

  /*
   * XXX: note that setting these to 0 in the past resulted in an error on
   * some systems, which is why it's better to use "noac" if possible.  For
   * now, we're setting everything possible, but if this will cause trouble,
   * then we'll have to condition the remainder of this on OPT_NOAC.
   */
#ifdef MNTTAB_OPT_ACTIMEO
  sprintf(preopts, ",%s=%d", MNTTAB_OPT_ACTIMEO, gopt.auto_attrcache);
  strcat(opts, preopts);
#else /* MNTTAB_OPT_ACTIMEO */
# ifdef MNTTAB_OPT_ACDIRMIN
  sprintf(preopts, ",%s=%d", MNTTAB_OPT_ACTDIRMIN, gopt.auto_attrcache);
  strcat(opts, preopts);
# endif /* MNTTAB_OPT_ACDIRMIN */
# ifdef MNTTAB_OPT_ACDIRMAX
  sprintf(preopts, ",%s=%d", MNTTAB_OPT_ACTDIRMAX, gopt.auto_attrcache);
  strcat(opts, preopts);
# endif /* MNTTAB_OPT_ACDIRMAX */
# ifdef MNTTAB_OPT_ACREGMIN
  sprintf(preopts, ",%s=%d", MNTTAB_OPT_ACTREGMIN, gopt.auto_attrcache);
  strcat(opts, preopts);
# endif /* MNTTAB_OPT_ACREGMIN */
# ifdef MNTTAB_OPT_ACREGMAX
  sprintf(preopts, ",%s=%d", MNTTAB_OPT_ACTREGMAX, gopt.auto_attrcache);
  strcat(opts, preopts);
# endif /* MNTTAB_OPT_ACREGMAX */
#endif /* MNTTAB_OPT_ACTIMEO */
}


/*
 * Mount the top-level
 */
int
amfs_toplvl_mount(am_node *mp, mntfs *mf)
{
  struct stat stb;
  char opts[256], preopts[256];
  int error;

  /*
   * Mounting the automounter.
   * Make sure the mount directory exists, construct
   * the mount options and call the mount_amfs_toplvl routine.
   */

  if (stat(mp->am_path, &stb) < 0) {
    return errno;
  } else if ((stb.st_mode & S_IFMT) != S_IFDIR) {
    plog(XLOG_WARNING, "%s is not a directory", mp->am_path);
    return ENOTDIR;
  }

  /*
   * Construct some mount options:
   *
   * Tack on magic map=<mapname> option in mtab to emulate
   * SunOS automounter behavior.
   */

#ifdef HAVE_FS_AUTOFS
  if (mf->mf_flags & MFF_IS_AUTOFS) {
    autofs_get_opts(opts, mp->am_autofs_fh);
  } else
#endif /* HAVE_FS_AUTOFS */
  {
    preopts[0] = '\0';
#ifdef MNTTAB_OPT_INTR
    strlcat(preopts, MNTTAB_OPT_INTR, sizeof(preopts));
    strlcat(preopts, ",", sizeof(preopts));
#endif /* MNTTAB_OPT_INTR */
#ifdef MNTTAB_OPT_IGNORE
    strlcat(preopts, MNTTAB_OPT_IGNORE, sizeof(preopts));
    strlcat(preopts, ",", sizeof(preopts));
#endif /* MNTTAB_OPT_IGNORE */
#ifdef WANT_TIMEO_AND_RETRANS_ON_TOPLVL
    sprintf(opts, "%s%s,%s=%d,%s=%d,%s=%d,%s,map=%s",
#else /* WANT_TIMEO_AND_RETRANS_ON_TOPLVL */
    sprintf(opts, "%s%s,%s=%d,%s,map=%s",
#endif /* WANT_TIMEO_AND_RETRANS_ON_TOPLVL */
	    preopts,
	    MNTTAB_OPT_RW,
	    MNTTAB_OPT_PORT, nfs_port,
#ifdef WANT_TIMEO_AND_RETRANS_ON_TOPLVL
	    /* note: TIMEO+RETRANS for toplvl are only "udp" currently */
	    MNTTAB_OPT_TIMEO, gopt.amfs_auto_timeo[AMU_TYPE_UDP],
	    MNTTAB_OPT_RETRANS, gopt.amfs_auto_retrans[AMU_TYPE_UDP],
#endif /* WANT_TIMEO_AND_RETRANS_ON_TOPLVL */
	    mf->mf_ops->fs_type, mf->mf_info);
#ifdef MNTTAB_OPT_NOAC
    if (gopt.auto_attrcache == 0) {
      strcat(opts, ",");
      strcat(opts, MNTTAB_OPT_NOAC);
    } else
#endif /* MNTTAB_OPT_NOAC */
      set_auto_attrcache_timeout(preopts, opts);
  }

  /* now do the mount */
  error = amfs_mount(mp, mf, opts);
  if (error) {
    errno = error;
    plog(XLOG_FATAL, "amfs_toplvl_mount: amfs_mount failed: %m");
    return error;
  }
  return 0;
}


/*
 * Unmount a top-level automount node
 */
int
amfs_toplvl_umount(am_node *mp, mntfs *mf)
{
  struct stat stb;
  int on_autofs = mf->mf_flags & MFF_ON_AUTOFS;
  int error;

again:
  /*
   * The lstat is needed if this mount is type=direct.
   * When that happens, the kernel cache gets confused
   * between the underlying type (dir) and the mounted
   * type (link) and so needs to be re-synced before
   * the unmount.  This is all because the unmount system
   * call follows links and so can't actually unmount
   * a link (stupid!).  It was noted that doing an ls -ld
   * of the mount point to see why things were not working
   * actually fixed the problem - so simulate an ls -ld here.
   */
  if (lstat(mp->am_path, &stb) < 0) {
    error = errno;
    dlog("lstat(%s): %m", mp->am_path);
    goto out;
  }
  if ((stb.st_mode & S_IFMT) != S_IFDIR) {
    plog(XLOG_ERROR, "amfs_toplvl_umount: %s is not a directory, aborting.", mp->am_path);
    error = ENOTDIR;
    goto out;
  }

  error = UMOUNT_FS(mp->am_path, mnttab_file_name, on_autofs);
  if (error == EBUSY) {
#ifdef HAVE_FS_AUTOFS
    /*
     * autofs mounts are "in place", so it is possible
     * that we can't just unmount our mount points and go away.
     * If that's the case, just give up.
     */
    if (mf->mf_flags & MFF_IS_AUTOFS)
      return error;
#endif /* HAVE_FS_AUTOFS */
    plog(XLOG_WARNING, "amfs_toplvl_unmount retrying %s in 1s", mp->am_path);
    sleep(1);			/* XXX */
    goto again;
  }
out:
  return error;
}
