/*
 * Copyright (c) 1997 Erez Zadok
 * Copyright (c) 1989 Jan-Simon Pendry
 * Copyright (c) 1989 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1989 The Regents of the University of California.
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
 * $Id: am_ops.c,v 1.4 1997/07/24 23:16:15 christos Exp $
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>


/*
 * The order of these entries matters, since lookups in this table are done
 * on a first-match basis.  The entries below are a mixture of native
 * filesystems supported by the OS (HAVE_FS_FOO), and some meta-filesystems
 * supported by amd (HAVE_AM_FS_FOO).  The order is set here in expected
 * match-hit such that more popular filesystems are listed first (nfs is the
 * most popular, followed by a symlink F/S)
 */
static am_ops *vops[] =
{
#ifdef HAVE_FS_NFS
  &nfs_ops,			/* network F/S (version 2) */
#endif /* HAVE_FS_NFS */
#ifdef HAVE_AM_FS_SFS
  &sfs_ops,			/* symlink F/S */
#endif /* HAVE_AM_FS_SFS */

  /*
   * Other amd-supported meta-filesystems.
   */
#ifdef HAVE_AM_FS_NFSX
  &nfsx_ops,			/* multiple-nfs F/S */
#endif /* HAVE_AM_FS_NFSX */
#ifdef HAVE_AM_FS_HOST
  &host_ops,			/* multiple exported nfs F/S */
#endif /* HAVE_AM_FS_HOST */
#ifdef HAVE_AM_FS_SFSX
  &sfsx_ops,			/* symlink F/S with link target verify */
#endif /* HAVE_AM_FS_SFSX */
#ifdef HAVE_AM_FS_PFS
  &pfs_ops,			/* program F/S */
#endif /* HAVE_AM_FS_PFS */
#ifdef HAVE_AM_FS_UNION
  &union_ops,			/* union F/S */
#endif /* HAVE_AM_FS_UNION */
#ifdef HAVE_AM_FS_IFS
  &ifs_ops,			/* inheritance F/S */
#endif /* HAVE_AM_FS_IFS */

  /*
   * A few more native filesystems.
   */
#ifdef HAVE_FS_UFS
  &ufs_ops,			/* Unix F/S */
#endif /* HAVE_FS_UFS */
#ifdef HAVE_FS_LOFS
  &lofs_ops,			/* loopback F/S */
#endif /* HAVE_FS_LOFS */
#ifdef HAVE_FS_CDFS
  &cdfs_ops,			/* CDROM/HSFS/ISO9960 F/S */
#endif /* HAVE_FS_CDFS */
#ifdef HAVE_FS_PCFS
  &pcfs_ops,			/* Floppy/MSDOS F/S */
#endif /* HAVE_FS_PCFS */
#ifdef HAVE_FS_NULLFS
				/* null (loopback) F/S */
#endif /* HAVE_FS_NULLFS */
#ifdef HAVE_FS_UNIONFS
				/* union (bsd44) F/S */
#endif /* HAVE_FS_UNIONFS */
#ifdef HAVE_FS_UMAPFS
				/* uid/gid mapping F/S */
#endif /* HAVE_FS_UMAPFS */


  /*
   * These four should be last, in the order:
   *	(1) afs
   *	(2) dfs
   *	(3) toplvl
   *	(4) efs
   */
#ifdef HAVE_AM_FS_AFS
  &afs_ops,			/* Automounter F/S */
#endif /* HAVE_AM_FS_AFS */
#ifdef HAVE_AM_FS_DFS
  &dfs_ops,			/* direct-mount F/S */
#endif /* HAVE_AM_FS_DFS */
#ifdef HAVE_AM_FS_TOPLVL
  &toplvl_ops,			/* top-level mount F/S */
#endif /* HAVE_AM_FS_TOPLVL */
#ifdef HAVE_AM_FS_EFS
  &efs_ops,			/* error F/S */
#endif /* HAVE_AM_FS_EFS */
  0
};


void
ops_showamfstypes(char *buf)
{
  struct am_ops **ap;
  int l = 0;

  buf[0] = '\0';
  for (ap = vops; *ap; ap++) {
    strcat(buf, (*ap)->fs_type);
    if (ap[1])
      strcat(buf, ", ");
    l += strlen((*ap)->fs_type) + 2;
    if (l > 60) {
      l = 0;
      strcat(buf, "\n      ");
    }
  }
}


static void
ops_show1(char *buf, int *lp, const char *name)
{
  strcat(buf, name);
  strcat(buf, ", ");
  *lp += strlen(name) + 2;
  if (*lp > 60) {
    strcat(buf, "\t\n");
    *lp = 0;
  }
}


void
ops_showfstypes(char *buf)
{
  int l = 0;

  buf[0] = '\0';

#ifdef MNTTAB_TYPE_AUTOFS
  ops_show1(buf, &l, MNTTAB_TYPE_AUTOFS);
#endif /* MNTTAB_TYPE_AUTOFS */

#ifdef MNTTAB_TYPE_CACHEFS
  ops_show1(buf, &l, MNTTAB_TYPE_CACHEFS);
#endif /* MNTTAB_TYPE_CACHEFS */

#ifdef MNTTAB_TYPE_CDFS
  ops_show1(buf, &l, MNTTAB_TYPE_CDFS);
#endif /* MNTTAB_TYPE_CDFS */

#ifdef MNTTAB_TYPE_CFS
  ops_show1(buf, &l, MNTTAB_TYPE_CFS);
#endif /* MNTTAB_TYPE_CFS */

#ifdef MNTTAB_TYPE_LOFS
  ops_show1(buf, &l, MNTTAB_TYPE_LOFS);
#endif /* MNTTAB_TYPE_LOFS */

#ifdef MNTTAB_TYPE_MFS
  ops_show1(buf, &l, MNTTAB_TYPE_MFS);
#endif /* MNTTAB_TYPE_MFS */

#ifdef MNTTAB_TYPE_NFS
  ops_show1(buf, &l, MNTTAB_TYPE_NFS);
#endif /* MNTTAB_TYPE_NFS */

#ifdef MNTTAB_TYPE_NFS3
  ops_show1(buf, &l, MNTTAB_TYPE_NFS3);
#endif /* MNTTAB_TYPE_NFS3 */

#ifdef MNTTAB_TYPE_NULLFS
  ops_show1(buf, &l, MNTTAB_TYPE_NULLFS);
#endif /* MNTTAB_TYPE_NULLFS */

#ifdef MNTTAB_TYPE_PCFS
  ops_show1(buf, &l, MNTTAB_TYPE_PCFS);
#endif /* MNTTAB_TYPE_PCFS */

#ifdef MNTTAB_TYPE_TFS
  ops_show1(buf, &l, MNTTAB_TYPE_TFS);
#endif /* MNTTAB_TYPE_TFS */

#ifdef MNTTAB_TYPE_TMPFS
  ops_show1(buf, &l, MNTTAB_TYPE_TMPFS);
#endif /* MNTTAB_TYPE_TMPFS */

#ifdef MNTTAB_TYPE_UFS
  ops_show1(buf, &l, MNTTAB_TYPE_UFS);
#endif /* MNTTAB_TYPE_UFS */

#ifdef MNTTAB_TYPE_UMAPFS
  ops_show1(buf, &l, MNTTAB_TYPE_UMAPFS);
#endif /* MNTTAB_TYPE_UMAPFS */

#ifdef MNTTAB_TYPE_UNIONFS
  ops_show1(buf, &l, MNTTAB_TYPE_UNIONFS);
#endif /* MNTTAB_TYPE_UNIONFS */

  /* terminate with a period, newline, and NULL */
  if (buf[strlen(buf)-1] == '\n')
    buf[strlen(buf) - 4] = '\0';
  else
    buf[strlen(buf) - 2] = '\0';
  strcat(buf, ".\n");
}


am_ops *
ops_match(am_opts *fo, char *key, char *g_key, char *path, char *keym, char *map)
{
  am_ops **vp;
  am_ops *rop = 0;

  /*
   * First crack the global opts and the local opts
   */
  if (!eval_fs_opts(fo, key, g_key, path, keym, map)) {
    rop = &efs_ops;
  } else if (fo->opt_type == 0) {
    plog(XLOG_USER, "No fs type specified (key = \"%s\", map = \"%s\")", keym, map);
    rop = &efs_ops;
  } else {
    /*
     * Next find the correct filesystem type
     */
    for (vp = vops; (rop = *vp); vp++)
      if (STREQ(rop->fs_type, fo->opt_type))
	break;
    if (!rop) {
      plog(XLOG_USER, "fs type \"%s\" not recognised", fo->opt_type);
      rop = &efs_ops;
    }
  }

  /*
   * Make sure we have a default mount option.
   * Otherwise skip past any leading '-'.
   */
  if (fo->opt_opts == 0)
    fo->opt_opts = "rw,defaults";
  else if (*fo->opt_opts == '-') {
    /*
     * We cannot simply do fo->opt_opts++ here since the opts
     * module will try to free the pointer fo->opt_opts later.
     * So just reallocate the thing -- stolcke 11/11/94
     */
    char *old = fo->opt_opts;
    fo->opt_opts = strdup(old + 1);
    free(old);
  }

  /*
   * Check the filesystem is happy
   */
  if (fo->fs_mtab)
    free((voidp) fo->fs_mtab);

  if ((fo->fs_mtab = (*rop->fs_match) (fo)))
    return rop;

  /*
   * Return error file system
   */
  fo->fs_mtab = (*efs_ops.fs_match) (fo);
  return &efs_ops;
}
