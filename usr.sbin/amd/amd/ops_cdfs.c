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
 * $Id: ops_cdfs.c,v 1.2 1997/07/24 23:16:53 christos Exp $
 *
 */

/*
 * High Sierra (CD-ROM) file system
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

static char *cdfs_match(am_opts *fo);
static int cdfs_fmount(mntfs *mf);
static int cdfs_fumount(mntfs *mf);

am_node *efs_lookuppn(am_node *mp, char *fname, int *error_return, int op);
fserver *find_afs_srvr(mntfs *mf);
int auto_fmount(am_node *mp);
int auto_fumount(am_node *mp);
int efs_readdir(am_node *mp, nfscookie cookie, nfsdirlist *dp, nfsentry *ep, int count);


/*
 * Ops structure
 */
am_ops cdfs_ops =
{
  MNTTAB_TYPE_CDFS,
  cdfs_match,
  0,				/* cdfs_init */
  auto_fmount,
  cdfs_fmount,
  auto_fumount,
  cdfs_fumount,
  efs_lookuppn,
  efs_readdir,
  0,				/* cdfs_readlink */
  0,				/* cdfs_mounted */
  0,				/* cdfs_umounted */
  find_afs_srvr,
  FS_MKMNT | FS_UBACKGROUND | FS_AMQINFO
};


/*
 * CDFS needs remote filesystem.
 */
static char *
cdfs_match(am_opts *fo)
{
  if (!fo->opt_dev) {
    plog(XLOG_USER, "cdfs: no source device specified");
    return 0;
  }
#ifdef DEBUG
  dlog("CDFS: mounting device \"%s\" on \"%s\"",
       fo->opt_dev, fo->opt_fs);
#endif /* DEBUG */

  /*
   * Determine magic cookie to put in mtab
   */
  return strdup(fo->opt_dev);
}


static int
mount_cdfs(char *dir, char *fs_name, char *opts)
{
  cdfs_args_t cdfs_args;
  mntent_t mnt;
  int flags;

  /*
   * Figure out the name of the file system type.
   */
  MTYPE_TYPE type = MOUNT_TYPE_CDFS;

  memset((voidp) &cdfs_args, 0, sizeof(cdfs_args)); /* Paranoid */

  /*
   * Fill in the mount structure
   */
  mnt.mnt_dir = dir;
  mnt.mnt_fsname = fs_name;
  mnt.mnt_type = MNTTAB_TYPE_CDFS;
  mnt.mnt_opts = opts;

  flags = compute_mount_flags(&mnt);

  cdfs_args.fspec = fs_name;
#ifdef HAVE_FIELD_CDFS_ARGS_T_NORRIP
  /* XXX: need to provide norrip mount opt */
  cdfs_args.norrip = 0;		/* use Rock-Ridge Protocol extensions */
#endif /* HAVE_FIELD_CDFS_ARGS_T_NORRIP */

  /*
   * Call generic mount routine
   */
  return mount_fs(&mnt, flags, (caddr_t) &cdfs_args, 0, type, 0, NULL);
}


static int
cdfs_fmount(mntfs *mf)
{
  int error;

  error = mount_cdfs(mf->mf_mount, mf->mf_info, mf->mf_mopts);
  if (error) {
    errno = error;
    plog(XLOG_ERROR, "mount_cdfs: %m");
    return error;
  }
  return 0;
}


static int
cdfs_fumount(mntfs *mf)
{
  return UMOUNT_FS(mf->mf_mount);
}
