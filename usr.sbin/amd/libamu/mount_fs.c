/*
 * Copyright (c) 1997 Erez Zadok
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
 *    must display the following acknowledgement:
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
 * $Id: mount_fs.c,v 1.1.1.1 1997/07/24 21:20:08 christos Exp $
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>


/* ensure that mount table options are delimited by a comma */
#define append_opts(old, new) { \
	if (*(old) != '\0') strcat(old, ","); \
	strcat(old, new); }

/*
 * Standard mount flags
 */
struct opt_tab mnt_flags[] =
{
#if defined(MNT2_GEN_OPT_RDONLY) && defined(MNTTAB_OPT_RO)
  {MNTTAB_OPT_RO, MNT2_GEN_OPT_RDONLY},
#endif /* defined(MNT2_GEN_OPT_RDONLY) && defined(MNTTAB_OPT_RO) */

#if defined(MNT2_GEN_OPT_NOCACHE) && defined(MNTTAB_OPT_NOCACHE)
  {MNTTAB_OPT_NOCACHE, MNT2_GEN_OPT_NOCACHE},
#endif /* defined(MNT2_GEN_OPT_NOCACHE) && defined(MNTTAB_OPT_NOCACHE) */

#if defined(MNT2_GEN_OPT_GRPID) && defined(MNTTAB_OPT_GRPID)
  {MNTTAB_OPT_GRPID, MNT2_GEN_OPT_GRPID},
#endif /* defined(MNT2_GEN_OPT_GRPID) && defined(MNTTAB_OPT_GRPID) */

#if defined(MNT2_GEN_OPT_MULTI) && defined(MNTTAB_OPT_MULTI)
  {MNTTAB_OPT_MULTI, MNT2_GEN_OPT_MULTI},
#endif /* defined(MNT2_GEN_OPT_MULTI) && defined(MNTTAB_OPT_MULTI) */

#if defined(MNT2_GEN_OPT_NODEV) && defined(MNTTAB_OPT_NODEV)
  {MNTTAB_OPT_NODEV, MNT2_GEN_OPT_NODEV},
#endif /* defined(MNT2_GEN_OPT_NODEV) && defined(MNTTAB_OPT_NODEV) */

#if defined(MNT2_GEN_OPT_NOEXEC) && defined(MNTTAB_OPT_NOEXEC)
  {MNTTAB_OPT_NOEXEC, MNT2_GEN_OPT_NOEXEC},
#endif /* defined(MNT2_GEN_OPT_NOEXEC) && defined(MNTTAB_OPT_NOEXEC) */

#if defined(MNT2_GEN_OPT_NOSUB) && defined(MNTTAB_OPT_NOSUB)
  {MNTTAB_OPT_NOSUB, MNT2_GEN_OPT_NOSUB},
#endif /* defined(MNT2_GEN_OPT_NOSUB) && defined(MNTTAB_OPT_NOSUB) */

#if defined(MNT2_GEN_OPT_NOSUID) && defined(MNTTAB_OPT_NOSUID)
  {MNTTAB_OPT_NOSUID, MNT2_GEN_OPT_NOSUID},
#endif /* defined(MNT2_GEN_OPT_NOSUID) && defined(MNTTAB_OPT_NOSUID) */

#if defined(MNT2_GEN_OPT_SYNC) && defined(MNTTAB_OPT_SYNC)
  {MNTTAB_OPT_SYNC, MNT2_GEN_OPT_SYNC},
#endif /* defined(MNT2_GEN_OPT_SYNC) && defined(MNTTAB_OPT_SYNC) */

#if defined(MNT2_GEN_OPT_OVERLAY) && defined(MNTTAB_OPT_OVERLAY)
  {MNTTAB_OPT_OVERLAY, MNT2_GEN_OPT_OVERLAY},
#endif /* defined(MNT2_GEN_OPT_OVERLAY) && defined(MNTTAB_OPT_OVERLAY) */

  {0, 0}
};


int
compute_mount_flags(mntent_t *mnt)
{
  struct opt_tab *opt;
  int flags;

#ifdef MNT2_GEN_OPT_NEWTYPE
  flags = MNT2_GEN_OPT_NEWTYPE;
#else /* not MNT2_GEN_OPT_NEWTYPE */
  /* Not all machines have MNT2_GEN_OPT_NEWTYPE (HP-UX 9.01) */
  flags = 0;
#endif /* not MNT2_GEN_OPT_NEWTYPE */

  /*
   * Crack basic mount options
   */
  for (opt = mnt_flags; opt->opt; opt++)
    flags |= hasmntopt(mnt, opt->opt) ? opt->flag : 0;

  return flags;
}


int
mount_fs(mntent_t *mnt, int flags, caddr_t mnt_data, int retry, MTYPE_TYPE type, u_long nfs_version, const char *nfs_proto)
{
  int error = 0;
#if defined(MNTTAB_OPT_DEV) || defined(MNTTAB_OPT_FSID)
  struct stat stb;
#endif /* defined(MNTTAB_OPT_DEV) || defined(MNTTAB_OPT_FSID) */
#ifdef MOUNT_TABLE_ON_FILE
  char *zopts = NULL, *xopts = NULL;
# if defined(MNTTAB_OPT_DEV) || defined(HAVE_FS_NFS3) || defined(MNTTAB_OPT_VERS) || defined(MNTTAB_OPT_PROTO)
  char optsbuf[48];
# endif /* defined(MNTTAB_OPT_DEV) || defined(HAVE_FS_NFS3) || defined(MNTTAB_OPT_VERS) || defined(MNTTAB_OPT_PROTO) */
#endif /* MOUNT_TABLE_ON_FILE */
#ifdef DEBUG
  char buf[80];			/* buffer for sprintf */
#endif /* DEBUG */

#ifdef DEBUG
  sprintf(buf, "%s%s%s",
	  "%s fstype ", MTYPE_PRINTF_TYPE, " (%s) flags %#x (%s)");
  dlog(buf, mnt->mnt_dir, type, mnt->mnt_type, flags, mnt->mnt_opts);
#endif /* DEBUG */

again:
  clock_valid = 0;

  error = MOUNT_TRAP(type, mnt, flags, mnt_data);

  if (error < 0) {
    plog(XLOG_ERROR, "%s: mount: %m", mnt->mnt_dir);
    /*
     * The following code handles conditions which shouldn't
     * occur.  They are possible either because amd screws up
     * in preparing for the mount, or because some human
     * messed with the mount point.  Both have been known to
     * happen. -- stolcke 2/22/95
     */
    if (errno == ENOENT) {
      /*
       * Occasionally the mount point vanishes, probably
       * due to some race condition.  Just recreate it
       * as necessary.
       */
      errno = mkdirs(mnt->mnt_dir, 0555);
      if (errno != 0 && errno != EEXIST)
	plog(XLOG_ERROR, "%s: mkdirs: %m", mnt->mnt_dir);
      else {
	plog(XLOG_WARNING, "extra mkdirs required for %s",
	     mnt->mnt_dir);
	error = MOUNT_TRAP(type, mnt, flags, mnt_data);
      }
    } else if (errno == EBUSY) {
      /*
       * Also, sometimes unmount isn't called, e.g., because
       * our mountlist is garbled.  This leaves old mount
       * points around which need to be removed before we
       * can mount something new in their place.
       */
      errno = umount_fs(mnt->mnt_dir);
      if (errno != 0)
	plog(XLOG_ERROR, "%s: umount: %m", mnt->mnt_dir);
      else {
	plog(XLOG_WARNING, "extra umount required for %s",
	     mnt->mnt_dir);
	error = MOUNT_TRAP(type, mnt, flags, mnt_data);
      }
    }
  }

  if (error < 0 && --retry > 0) {
    sleep(1);
    goto again;
  }
  if (error < 0) {
    return errno;
  }

#ifdef MOUNT_TABLE_ON_FILE
  /*
   * Allocate memory for options:
   *        dev=..., vers={2,3}, proto={tcp,udp}
   */
  zopts = (char *) xmalloc(strlen(mnt->mnt_opts) + 48);

  /* copy standard options */
  xopts = mnt->mnt_opts;

  strcpy(zopts, xopts);

# ifdef MNTTAB_OPT_DEV
  /* add the extra dev= field to the mount table */
  if (lstat(mnt->mnt_dir, &stb) == 0) {
    if (sizeof(stb.st_dev) == 2) /* e.g. SunOS 4.1 */
      sprintf(optsbuf, "%s=%04lx",
	      MNTTAB_OPT_DEV, (u_long) stb.st_dev & 0xffff);
    else			/* e.g. System Vr4 */
      sprintf(optsbuf, "%s=%08lx",
	      MNTTAB_OPT_DEV, (u_long) stb.st_dev);
    append_opts(zopts, optsbuf);
  }
# endif /* MNTTAB_OPT_DEV */

# if defined(HAVE_FS_NFS3) && defined(MNTTAB_OPT_VERS)
  /*
   * add the extra vers={2,3} field to the mount table,
   * unless already specified by user
   */
   if (nfs_version == NFS_VERSION3 &&
       hasmntval(mnt, MNTTAB_OPT_VERS) != NFS_VERSION3) {
     sprintf(optsbuf, "%s=%d", MNTTAB_OPT_VERS, NFS_VERSION3);
     append_opts(zopts, optsbuf);
   }
# endif /* defined(HAVE_FS_NFS3) && defined(MNTTAB_OPT_VERS) */

# ifdef MNTTAB_OPT_PROTO
  /*
   * add the extra proto={tcp,udp} field to the mount table,
   * unless already specified by user.
   */
  if (nfs_proto && !hasmntopt(mnt, MNTTAB_OPT_PROTO)) {
    sprintf(optsbuf, "%s=%s", MNTTAB_OPT_PROTO, nfs_proto);
    append_opts(zopts, optsbuf);
  }
# endif /* MNTTAB_OPT_PROTO */

  /* finally, store the options into the mount table structure */
  mnt->mnt_opts = zopts;

  /*
   * Additional fields in mntent_t
   * are fixed up here
   */
# ifdef HAVE_FIELD_MNTENT_T_CNODE
  mnt->mnt_cnode = 0;
# endif /* HAVE_FIELD_MNTENT_T_CNODE */
# ifdef HAVE_FIELD_MNTENT_T_RO
  mnt->mnt_ro = (hasmntopt(mnt, MNTTAB_OPT_RO) != NULL);
# endif /* HAVE_FIELD_MNTENT_T_RO */
# ifdef HAVE_FIELD_MNTENT_T_TIME
#  ifdef HAVE_FIELD_MNTENT_T_TIME_STRING
  {				/* allocate enough space for a long */
    char *str = (char *) xmalloc(13 * sizeof(char));
    sprintf(str, "%ld", time((time_t *) NULL));
    mnt->mnt_time = str;
  }
#  else /* not HAVE_FIELD_MNTENT_T_TIME_STRING */
  mnt->mnt_time = time((time_t *) NULL);
#  endif /* not HAVE_FIELD_MNTENT_T_TIME_STRING */
# endif /* HAVE_FIELD_MNTENT_T_TIME */

  write_mntent(mnt);

# ifdef MNTTAB_OPT_DEV
  if (xopts) {
    free(mnt->mnt_opts);
    mnt->mnt_opts = xopts;
  }
# endif /* MNTTAB_OPT_DEV */
#endif /* MOUNT_TABLE_ON_FILE */

  return 0;
}
