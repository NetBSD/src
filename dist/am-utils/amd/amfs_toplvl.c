/*	$NetBSD: amfs_toplvl.c,v 1.2 2003/07/15 09:01:15 itojun Exp $	*/

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
 * Id: amfs_toplvl.c,v 1.28 2002/12/27 22:43:48 ezk Exp
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
  amfs_auto_match,
  0,				/* amfs_auto_init */
  amfs_toplvl_mount,
  amfs_toplvl_umount,
  amfs_auto_lookup_child,
  amfs_auto_mount_child,
  amfs_auto_readdir,		/* browsable version of readdir() */
  0,				/* amfs_toplvl_readlink */
  amfs_auto_mounted,
  0,				/* amfs_toplvl_umounted */
  find_amfs_auto_srvr,
  FS_MKMNT | FS_NOTIMEOUT | FS_BACKGROUND |
	  FS_AMQINFO | FS_DIRECTORY | FS_AUTOFS, /* nfs_fs_flags */
#ifdef HAVE_FS_AUTOFS
  AUTOFS_TOPLVL_FS_FLAGS,
#endif /* HAVE_FS_AUTOFS */
};


/****************************************************************************
 *** FUNCTIONS                                                             ***
 ****************************************************************************/

/*
 * Mount an automounter directory.
 * The automounter is connected into the system
 * as a user-level NFS server.  mount_amfs_toplvl constructs
 * the necessary NFS parameters to be given to the
 * kernel so that it will talk back to us.
 *
 * NOTE: automounter mounts in themselves are using NFS Version 2.
 *
 * NEW: on certain systems, mounting can be done using the
 * kernel-level automount (autofs) support. In that case,
 * we don't need NFS at all here.
 */
int
mount_amfs_toplvl(mntfs *mf, char *opts)
{
  char fs_hostname[MAXHOSTNAMELEN + MAXPATHLEN + 1];
  int retry, error = 0, genflags;
  char *dir = mf->mf_mount;
  mntent_t mnt;
  MTYPE_TYPE type;

  memset((voidp) &mnt, 0, sizeof(mnt));
  mnt.mnt_dir = dir;
  mnt.mnt_fsname = pid_fsname;
  mnt.mnt_opts = opts;

#ifdef HAVE_FS_AUTOFS
  if (mf->mf_flags & MFF_AUTOFS) {
    type = MOUNT_TYPE_AUTOFS;
    /*
     * Make sure that amd's top-level autofs mounts are hidden by default
     * from df.
     * XXX: It works ok on Linux, might not work on other systems.
     */
    mnt.mnt_type = "autofs";
  } else
#endif /* HAVE_FS_AUTOFS */
  {
    type = MOUNT_TYPE_NFS;
    /*
     * Make sure that amd's top-level NFS mounts are hidden by default
     * from df.
     * If they don't appear to support the either the "ignore" mnttab
     * option entry, or the "auto" one, set the mount type to "nfs".
     */
    mnt.mnt_type = HIDE_MOUNT_TYPE;
  }

  retry = hasmntval(&mnt, MNTTAB_OPT_RETRY);
  if (retry <= 0)
    retry = 2;			/* XXX: default to 2 retries */

  /*
   * SET MOUNT ARGS
   */

  /*
   * Make a ``hostname'' string for the kernel
   */
  snprintf(fs_hostname, sizeof(fs_hostname), "pid%ld@%s:%s",
	  get_server_pid(), am_get_hostname(), dir);
  /*
   * Most kernels have a name length restriction (64 bytes)...
   */
  if (strlen(fs_hostname) >= MAXHOSTNAMELEN)
    strlcpy(fs_hostname + MAXHOSTNAMELEN - 3, "..", sizeof(fs_hostname) - (MAXHOSTNAMELEN - 3));
#ifdef HOSTNAMESZ
  /*
   * ... and some of these restrictions are 32 bytes (HOSTNAMESZ)
   * If you need to get the definition for HOSTNAMESZ found, you may
   * add the proper header file to the conf/nfs_prot/nfs_prot_*.h file.
   */
  if (strlen(fs_hostname) >= HOSTNAMESZ)
    strlcpy(fs_hostname + HOSTNAMESZ - 3, "..", sizeof(fs_hostname) - (HOSTNAMESZ - 3));
#endif /* HOSTNAMESZ */

  /*
   * Finally we can compute the mount genflags set above,
   * and add any automounter specific flags.
   */
  genflags = compute_mount_flags(&mnt);
  genflags |= compute_automounter_mount_flags(&mnt);

  if (!(mf->mf_flags & MFF_AUTOFS)) {
    nfs_args_t nfs_args;
    am_nfs_fh *fhp;
    am_nfs_handle_t anh;
#ifndef HAVE_TRANSPORT_TYPE_TLI
    u_short port;
    struct sockaddr_in sin;
#endif /* not HAVE_TRANSPORT_TYPE_TLI */

    /*
     * get fhandle of remote path for automount point
     */
    fhp = get_root_nfs_fh(dir);
    if (!fhp) {
      plog(XLOG_FATAL, "Can't find root file handle for %s", dir);
      return EINVAL;
    }

#ifndef HAVE_TRANSPORT_TYPE_TLI
    /*
     * Create sockaddr to point to the local machine.
     */
    memset((voidp) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr = myipaddr;
    port = hasmntval(&mnt, MNTTAB_OPT_PORT);
    if (port) {
      sin.sin_port = htons(port);
    } else {
      plog(XLOG_ERROR, "no port number specified for %s", dir);
      return EINVAL;
    }
#endif /* not HAVE_TRANSPORT_TYPE_TLI */

    /* setup the many fields and flags within nfs_args */
    memmove(&anh.v2.fhs_fh, fhp, sizeof(*fhp));
#ifdef HAVE_TRANSPORT_TYPE_TLI
    compute_nfs_args(&nfs_args,
		     &mnt,
		     genflags,
		     nfsncp,
		     NULL,	/* remote host IP addr is set below */
		     NFS_VERSION,	/* version 2 */
		     "udp",
		     &anh,
		     fs_hostname,
		     pid_fsname);
    /*
     * IMPORTANT: set the correct IP address AFTERWARDS.  It cannot
     * be done using the normal mechanism of compute_nfs_args(), because
     * that one will allocate a new address and use NFS_SA_DREF() to copy
     * parts to it, while assuming that the ip_addr passed is always
     * a "struct sockaddr_in".  That assumption is incorrect on TLI systems,
     * because they define a special macro HOST_SELF which is DIFFERENT
     * than localhost (127.0.0.1)!
     */
    nfs_args.addr = &nfsxprt->xp_ltaddr;
#else /* not HAVE_TRANSPORT_TYPE_TLI */
    compute_nfs_args(&nfs_args,
		     &mnt,
		     genflags,
		     NULL,
		     &sin,
		     NFS_VERSION,	/* version 2 */
		     "udp",
		     &anh,
		     fs_hostname,
		     pid_fsname);
#endif /* not HAVE_TRANSPORT_TYPE_TLI */

    /*************************************************************************
     * NOTE: while compute_nfs_args() works ok for regular NFS mounts	     *
     * the toplvl one is not quite regular, and so some options must be      *
     * corrected by hand more carefully, *after* compute_nfs_args() runs.    *
     *************************************************************************/
    compute_automounter_nfs_args(&nfs_args, &mnt);

    if (amuDebug(D_TRACE)) {
      print_nfs_args(&nfs_args, 0);
      plog(XLOG_DEBUG, "Generic mount flags 0x%x", genflags);
    }

    /* This is it!  Here we try to mount amd on its mount points */
    error = mount_fs2(&mnt, mf->mf_real_mount, genflags, (caddr_t) &nfs_args,
		      retry, type, 0, NULL, mnttab_file_name);

#ifdef HAVE_TRANSPORT_TYPE_TLI
    free_knetconfig(nfs_args.knconf);
    /*
     * local automounter mounts do not allocate a special address, so
     * no need to XFREE(nfs_args.addr) under TLI.
     */
#endif /* HAVE_TRANSPORT_TYPE_TLI */

#ifdef HAVE_FS_AUTOFS
  } else {
    /* This is it!  Here we try to mount amd on its mount points */
    error = mount_fs2(&mnt, mf->mf_real_mount, genflags, (caddr_t) mf->mf_autofs_fh,
		      retry, type, 0, NULL, mnttab_file_name);
#endif /* HAVE_FS_AUTOFS */
  }

  return error;
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
  if (mf->mf_flags & MFF_AUTOFS) {
    autofs_get_opts(opts, mf->mf_autofs_fh);
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
    snprintf(opts, sizeof(opts), "%s%s,%s=%d,%s=%d,%s=%d,%s,map=%s",
	    preopts,
	    MNTTAB_OPT_RW,
	    MNTTAB_OPT_PORT, nfs_port,
	    MNTTAB_OPT_TIMEO, gopt.amfs_auto_timeo,
	    MNTTAB_OPT_RETRANS, gopt.amfs_auto_retrans,
	    mf->mf_ops->fs_type, mf->mf_info);
  }

  /* now do the mount */
  error = mount_amfs_toplvl(mf, opts);
  if (error) {
    errno = error;
    plog(XLOG_FATAL, "amfs_toplvl_mount: mount_amfs_toplvl failed: %m");
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
  int error;
  struct stat stb;

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
  if (lstat(mp->am_path, &stb) < 0)
    dlog("lstat(%s): %m", mp->am_path);

  error = UMOUNT_FS(mp->am_path, mf->mf_real_mount, mnttab_file_name);
  if (error == EBUSY) {
#ifdef HAVE_FS_AUTOFS
    /*
     * autofs mounts are in place, so it is possible
     * that we can't just unmount our mount points and go away.
     * If that's the case, just give up.
     */
    if (mf->mf_flags & MFF_AUTOFS)
      return 0;
#endif /* HAVE_FS_AUTOFS */
    plog(XLOG_WARNING, "amfs_toplvl_unmount retrying %s in 1s", mp->am_path);
    sleep(1);			/* XXX */
    goto again;
  }
  return error;
}
