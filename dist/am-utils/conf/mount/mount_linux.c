/*	$NetBSD: mount_linux.c,v 1.1.1.6 2003/03/09 01:13:22 christos Exp $	*/

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
 * Id: mount_linux.c,v 1.36 2002/12/27 22:43:56 ezk Exp
 */

/*
 * Linux mount helper.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>


struct opt_map {
  const char *opt;		/* option name */
  int inv;			/* true if flag value should be inverted */
  int mask;			/* flag mask value */
};

const struct opt_map opt_map[] =
{
  {"defaults",		0,	0},
  {MNTTAB_OPT_RO,	0,	MNT2_GEN_OPT_RDONLY},
  {MNTTAB_OPT_RW,	1,	MNT2_GEN_OPT_RDONLY},
  {MNTTAB_OPT_EXEC,	1,	MNT2_GEN_OPT_NOEXEC},
  {MNTTAB_OPT_NOEXEC,	0,	MNT2_GEN_OPT_NOEXEC},
  {MNTTAB_OPT_SUID,	1,	MNT2_GEN_OPT_NOSUID},
  {MNTTAB_OPT_NOSUID,	0,	MNT2_GEN_OPT_NOSUID},
#ifdef MNT2_GEN_OPT_NODEV
  {MNTTAB_OPT_NODEV,	0,	MNT2_GEN_OPT_NODEV},
#endif /* MNT2_GEN_OPT_NODEV */
#ifdef MNT2_GEN_OPT_SYNC
  {MNTTAB_OPT_SYNC,	0,	MNT2_GEN_OPT_SYNC},
  {MNTTAB_OPT_ASYNC,	1,	MNT2_GEN_OPT_SYNC},
#endif /* MNT2_GEN_OPT_SYNC */
#ifdef MNT2_GEN_OPT_NOSUB
  {MNTTAB_OPT_SUB,	1,	MNT2_GEN_OPT_NOSUB},
  {MNTTAB_OPT_NOSUB,	0,	MNT2_GEN_OPT_NOSUB},
#endif /* MNT2_GEN_OPT_NOSUB */
  {NULL,		0,	0}
};

struct fs_opts {
  const char *opt;
  int type;			/* XXX: Ion, what is this for? */
};

const struct fs_opts iso_opts[] = {
  { "map",	0 },
  { "norock",	0 },
  { "cruft",	0 },
  { "unhide",	0 },
  { "conv",	1 },
  { "block",	1 },
  { "mode",	1 },
  { "gid",	1 },
  { "uid",	1 },
  { NULL,	0 }
};

const struct fs_opts dos_opts[] = {
  { "check",	1 },
  { "conv",	1 },
  { "uid",	1 },
  { "gid",	1 },
  { "umask",	1 },
  { "debug",	0 },
  { "fat",	1 },
  { "quiet",	0 },
  { "blocksize",1 },
  { NULL,	0 }
};

const struct fs_opts autofs_opts[] = {
  { "fd",	1 },
  { "pgrp",	1 },
  { "minproto",	1 },
  { "maxproto",	1 },
  { NULL,	0 }
};

const struct fs_opts null_opts[] = {
  { NULL,	0 }
};


/*
 * New parser for linux-specific mounts.
 * Should now handle fs-type specific mount-options correctly.
 * Currently implemented: msdos, iso9660.
 */
static char *
parse_opts(char *type, char *optstr, int *flags, char **xopts, int *noauto)
{
  const struct opt_map *std_opts;
  const struct fs_opts *dev_opts;
  char *opt, *topts;

  *noauto = 0;
  if (optstr != NULL) {
    *xopts = (char *) xmalloc (strlen(optstr) + 2);
    topts = (char *) xmalloc (strlen(optstr) + 2);
    *topts = '\0';
    **xopts = '\0';

    for (opt = strtok(optstr, ","); opt; opt = strtok(NULL, ",")) {
      /*
       * First, parse standard options
       */
      std_opts = opt_map;
      while (std_opts->opt &&
	     !NSTREQ(std_opts->opt, opt, strlen(std_opts->opt)))
	++std_opts;
      if (!(*noauto = STREQ(opt, MNTTAB_OPT_NOAUTO)) || std_opts->opt) {
	strcat(topts, opt);
	strcat(topts, ",");
	if (std_opts->inv)
	  *flags &= ~std_opts->mask;
	else
	  *flags |= std_opts->mask;
      }
      /*
       * Next, select which fs-type is to be used
       * and parse the fs-specific options
       */
#ifdef MOUNT_TYPE_AUTOFS
      if (STREQ(type, MOUNT_TYPE_AUTOFS)) {
	dev_opts = autofs_opts;
	goto do_opts;
      }
#endif /* MOUNT_TYPE_AUTOFS */
#ifdef MOUNT_TYPE_PCFS
      if (STREQ(type, MOUNT_TYPE_PCFS)) {
	dev_opts = dos_opts;
	goto do_opts;
      }
#endif /* MOUNT_TYPE_PCFS */
#ifdef MOUNT_TYPE_CDFS
      if (STREQ(type, MOUNT_TYPE_CDFS)) {
	dev_opts = iso_opts;
	goto do_opts;
      }
#endif /* MOUNT_TYPE_CDFS */
#ifdef MOUNT_TYPE_LOFS
      if (STREQ(type, MOUNT_TYPE_LOFS)) {
	dev_opts = null_opts;
	goto do_opts;
      }
#endif /* MOUNT_TYPE_LOFS */
      plog(XLOG_FATAL, "linux mount: unknown fs-type: %s\n", type);
      return NULL;

do_opts:
      while (dev_opts->opt &&
	     (!NSTREQ(dev_opts->opt, opt, strlen(dev_opts->opt)))) {
	++dev_opts;
      }
      if (dev_opts->opt && *xopts) {
	strcat(*xopts, opt);
	strcat(*xopts, ",");
      }
    }
    /*
     * All other options are discarded
     */
    if (strlen(*xopts))
      *(*xopts + strlen(*xopts)-1) = '\0';
    if (strlen(topts))
      topts[strlen(topts)-1] = 0;
    return topts;
  } /* end of "if (optstr != NULL)" statement */

  return NULL;
}


/*
 * Returns combined linux kernel version number.  For a kernel numbered
 * x.y.z, returns x*65535+y*256+z.
 */
int
linux_version_code(void)
{
  struct utsname my_utsname;
  static int release = 0;

  if ( 0 == release && 0 == uname(&my_utsname)) {
    release = 65536 * atoi(strtok(my_utsname.release, "."))
      + 256 * atoi(strtok(NULL, "."))
      + atoi(strtok(NULL, "."));
  }
  return release;
}


int
do_mount_linux(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data)
{
  if (amuDebug(D_FULL)) {
    plog(XLOG_DEBUG, "do_mount_linux: fsname %s\n", mnt->mnt_fsname);
    plog(XLOG_DEBUG, "do_mount_linux: type (mntent) %s\n", mnt->mnt_type);
    plog(XLOG_DEBUG, "do_mount_linux: opts %s\n", mnt->mnt_opts);
    plog(XLOG_DEBUG, "do_mount_linux: dir %s\n", mnt->mnt_dir);
  }

  /*
   * If we have an nfs mount, the 5th argument to system mount() must be the
   * nfs_mount_data structure, otherwise it is the return from parse_opts()
   */
  return mount(mnt->mnt_fsname,
	       mnt->mnt_dir,
	       type,
	       MS_MGC_VAL | flags,
	       data);
}


int
mount_linux_nfs(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data)
{
  nfs_args_t *mnt_data = (nfs_args_t *) data;
  int errorcode;

  /* Fake some values for linux */
  mnt_data->version = NFS_MOUNT_VERSION;
  if (!mnt_data->timeo) {
#ifdef MNT2_NFS_OPT_TCP
    if (mnt_data->flags & MNT2_NFS_OPT_TCP)
      mnt_data->timeo = 600;
    else
#endif /* MNT2_NFS_OPT_TCP */
      mnt_data->timeo = 7;
  }
  if (!mnt_data->retrans)
    mnt_data->retrans = 3;

#ifdef MNT2_NFS_OPT_NOAC
  if (!(mnt_data->flags & MNT2_NFS_OPT_NOAC)) {
    if (!mnt_data->acregmin)
      mnt_data->acregmin = 3;
    if (!mnt_data->acregmax)
      mnt_data->acregmax = 60;
    if (!mnt_data->acdirmin)
      mnt_data->acdirmin = 30;
    if (!mnt_data->acdirmax)
      mnt_data->acdirmax = 60;
  }
#endif /* MNT2_NFS_OPT_NOAC */

  /*
   * in nfs structure implementation version 4, the old
   * filehandle field was renamed "old_root" and left as 3rd field,
   * while a new field called "root" was added to the end of the
   * structure. Both of them however need a copy of the file handle
   * for NFSv2 mounts.
   */
#ifdef MNT2_NFS_OPT_VER3
  if (mnt_data->flags & MNT2_NFS_OPT_VER3)
    memset(mnt_data->old_root.data, 0, FHSIZE);
  else
#endif /* MNT2_NFS_OPT_VER3 */
    memcpy(mnt_data->old_root.data, mnt_data->root.data, FHSIZE);

#ifdef HAVE_NFS_ARGS_T_BSIZE
  /* linux mount version 3 */
  mnt_data->bsize = 0;			/* let the kernel decide */
#endif /* HAVE_NFS_ARGS_T_BSIZE */

#ifdef HAVE_NFS_ARGS_T_NAMLEN
  /* linux mount version 2 */
  mnt_data->namlen = NAME_MAX;		/* 256 bytes */
#endif /* HAVE_NFS_ARGS_T_NAMELEN */

  mnt_data->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (mnt_data->fd < 0) {
    plog(XLOG_ERROR, "Can't create socket for kernel");
    return 1;
  }
  if (bindresvport(mnt_data->fd, 0) < 0) {
    plog(XLOG_ERROR, "Can't bind to reserved port");
    errorcode = 1;
    goto out;
  }
  /*
   * connect() the socket for kernels 1.3.10 and below
   * only to avoid problems with multihomed hosts.
   */
  if (linux_version_code() <= 0x01030a) {
    int ret = connect(mnt_data->fd,
		      (struct sockaddr *) &mnt_data->addr,
		      sizeof(mnt_data->addr));
    if (ret < 0) {
      plog(XLOG_ERROR, "Can't connect socket for kernel");
      errorcode = 1;
      goto out;
    }
  }
  if (amuDebug(D_FULL)) {
    plog(XLOG_DEBUG, "mount_linux_nfs: type %s\n", type);
    plog(XLOG_DEBUG, "mount_linux_nfs: version %d\n", mnt_data->version);
    plog(XLOG_DEBUG, "mount_linux_nfs: fd %d\n", mnt_data->fd);
    plog(XLOG_DEBUG, "mount_linux_nfs: hostname %s\n",
	 inet_ntoa(mnt_data->addr.sin_addr));
    plog(XLOG_DEBUG, "mount_linux_nfs: port %d\n",
	 htons(mnt_data->addr.sin_port));
  }
  if (amuDebug(D_TRACE)) {
    plog(XLOG_DEBUG, "mount_linux_nfs: Generic mount flags 0x%x", MS_MGC_VAL | flags);
    plog(XLOG_DEBUG, "mount_linux_nfs: updated nfs_args...");
    print_nfs_args(mnt_data, 0);
  }

  errorcode = do_mount_linux(type, mnt, flags, data);

 out:
  /*
   * If we failed, (i.e. errorcode != 0), then close the socket
   * if it is open.
   */
  if (errorcode && mnt_data->fd != -1) {
    /* save errno, may be clobbered by close() call! */
    int save_errno = errno;
    close(mnt_data->fd);
    errno = save_errno;
  }
  return errorcode;
}


int
mount_linux_nonfs(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data)
{
  char *extra_opts = NULL;
  char *tmp_opts = NULL;
  char *sub_type = NULL;
  char *loopdev = NULL;
  int noauto = 0;
  int errorcode;

  sub_type = hasmnteq(mnt, "type");
  if (sub_type) {
    sub_type = strdup(sub_type);
    if (sub_type) {		/* the strdup malloc might have failed */
      type = strpbrk(sub_type, ",:;\n\t");
      if (type == NULL)
	type = MOUNT_TYPE_UFS;
      else {
	*type = '\0';
	type = sub_type;
      }
    } else {
      plog(XLOG_ERROR, "strdup returned null in mount_linux_nonfs");
    }
  }

  if (!hasmntopt(mnt, "type"))
    mnt->mnt_type = type;

  tmp_opts = parse_opts(type, mnt->mnt_opts, &flags, &extra_opts, &noauto);

#if defined(MOUNT_TYPE_LOFS)
  if (STREQ(type, MOUNT_TYPE_LOFS)) {
# if defined(MNT2_GEN_OPT_BIND)
    /* use bind mounts for lofs */
    flags |= MNT2_GEN_OPT_BIND;
# else /* not MNT2_GEN_OPT_BIND */
    /* this is basically a hack to support fist lofs */
    XFREE(extra_opts);
    extra_opts = (char *) xmalloc(strlen(mnt->mnt_fsname) + sizeof("dir=") + 1);
    sprintf(extra_opts, "dir=%s", mnt->mnt_fsname);
# endif /* not MNT2_GEN_OPT_BIND */
    errorcode = do_mount_linux(type, mnt, flags, extra_opts);
  } else /* end of "if type is LOFS" */
#endif /* MOUNT_TYPE_LOFS */

  {
#ifdef HAVE_LOOP_DEVICE
    /*
     * If the mounted "device" is actually a regular file,
     # try to attach a loop device to it.
     */
    struct stat buf;
    char *old_fsname = NULL;
    if (stat(mnt->mnt_fsname, &buf) == 0 &&
	S_ISREG(buf.st_mode)) {
      if ((loopdev = setup_loop_device(mnt->mnt_fsname)) != NULL) {
	char *str;
	int len;

	plog(XLOG_INFO, "setup loop device %s over %s OK", loopdev, mnt->mnt_fsname);
	old_fsname = mnt->mnt_fsname;
	mnt->mnt_fsname = loopdev;
	/* XXX: hack, append loop=/dev/loopX to mnttab opts */
	len = strlen(mnt->mnt_opts) + 7 + strlen(loopdev);
	str = (char *) xmalloc(len);
	if (str) {
	  sprintf(str, "%s,loop=%s", mnt->mnt_opts, loopdev);
	  XFREE(mnt->mnt_opts);
	  mnt->mnt_opts = str;
	}
      } else {
	plog(XLOG_ERROR, "failed to set up a loop device: %m");
	errorcode = 1;
	goto out;
      }
    }
#endif /* HAVE_LOOP_DEVICE */

    errorcode = do_mount_linux(type, mnt, flags, extra_opts);

#ifdef HAVE_LOOP_DEVICE
    /* if mount failed and we used a loop device, then undo it */
    if (errorcode != 0 && loopdev != NULL) {
      if (delete_loop_device(loopdev) < 0)
	plog(XLOG_WARNING, "mount() failed to release loop device %s: %m", loopdev);
      else
	plog(XLOG_INFO, "mount() released loop device %s OK", loopdev);
    }
    if (old_fsname)
      mnt->mnt_fsname = old_fsname;
#endif /* HAVE_LOOP_DEVICE */
  }

  /*
   * Free all allocated space and return errorcode.
   */
out:
if (loopdev)
     XFREE(loopdev);
  if (extra_opts != NULL)
    XFREE(extra_opts);
  if (tmp_opts != NULL)
    XFREE(tmp_opts);
  if (sub_type != NULL)
    XFREE(sub_type);
  return errorcode;
}


int
mount_linux(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data)
{
  int errorcode;

  if (mnt->mnt_opts && STREQ(mnt->mnt_opts, "defaults"))
    mnt->mnt_opts = NULL;

  if (type == NULL)
    type = index(mnt->mnt_fsname, ':') ? MOUNT_TYPE_NFS : MOUNT_TYPE_UFS;

  if (STREQ(type, MOUNT_TYPE_NFS))
    errorcode = mount_linux_nfs(type, mnt, flags, data);
  else				/* non-NFS mounts */
    errorcode = mount_linux_nonfs(type, mnt, flags, data);

  return errorcode;
}


/****************************************************************************/
/*
 * NFS error numbers and Linux errno's are two different things!  Linux is
 * `worse' than other OSes in the respect that it loudly complains about
 * undefined NFS return value ("bad NFS return value..").  So we should
 * translate ANY possible Linux errno to their NFS equivalent.  Just, there
 * aren't much NFS numbers, so most go to EINVAL or EIO.  The mapping below
 * should fit at least for Linux/i386 and Linux/68k.  I haven't checked
 * other architectures yet.
 */

#define NE_PERM		1
#define NE_NOENT	2
#define NE_IO		5
#define NE_NXIO		6
#define NE_AGAIN	11
#define NE_ACCES	13
#define NE_EXIST	17
#define NE_NODEV	19
#define NE_NOTDIR	20
#define NE_ISDIR	21
#define NE_INVAL	22
#define NE_FBIG		27
#define NE_NOSPC	28
#define NE_ROFS		30
#define NE_NAMETOOLONG	63
#define NE_NOTEMPTY	66
#define NE_DQUOT	69
#define NE_STALE	70

#define NFS_LOMAP	0
#define NFS_HIMAP	122

/*
 * The errno's below are correct for Linux/i386. One day, somebody
 * with lots of energy ought to verify them against the other ports...
 */
static int nfs_errormap[] = {
	0,		/* success(0)		*/
	NE_PERM,	/* EPERM (1)		*/
	NE_NOENT,	/* ENOENT (2)		*/
	NE_INVAL,	/* ESRCH (3)		*/
	NE_IO,		/* EINTR (4)		*/
	NE_IO,		/* EIO (5)		*/
	NE_NXIO,	/* ENXIO (6)		*/
	NE_INVAL,	/* E2BIG (7)		*/
	NE_INVAL,	/* ENOEXEC (8)		*/
	NE_INVAL,	/* EBADF (9)		*/
	NE_IO,		/* ECHILD (10)		*/
	NE_AGAIN,	/* EAGAIN (11)		*/
	NE_IO,		/* ENOMEM (12)		*/
	NE_ACCES,	/* EACCES (13)		*/
	NE_INVAL,	/* EFAULT (14)		*/
	NE_INVAL,	/* ENOTBLK (15)		*/
	NE_IO,		/* EBUSY (16)		*/
	NE_EXIST,	/* EEXIST (17)		*/
	NE_INVAL,	/* EXDEV (18)		*/
	NE_NODEV,	/* ENODEV (19)		*/
	NE_NOTDIR,	/* ENOTDIR (20)		*/
	NE_ISDIR,	/* EISDIR (21)		*/
	NE_INVAL,	/* EINVAL (22)		*/
	NE_IO,		/* ENFILE (23)		*/
	NE_IO,		/* EMFILE (24)		*/
	NE_INVAL,	/* ENOTTY (25)		*/
	NE_ACCES,	/* ETXTBSY (26)		*/
	NE_FBIG,	/* EFBIG (27)		*/
	NE_NOSPC,	/* ENOSPC (28)		*/
	NE_INVAL,	/* ESPIPE (29)		*/
	NE_ROFS,	/* EROFS (30)		*/
	NE_INVAL,	/* EMLINK (31)		*/
	NE_INVAL,	/* EPIPE (32)		*/
	NE_INVAL,	/* EDOM (33)		*/
	NE_INVAL,	/* ERANGE (34)		*/
	NE_INVAL,	/* EDEADLK (35)		*/
	NE_NAMETOOLONG,	/* ENAMETOOLONG (36)	*/
	NE_INVAL,	/* ENOLCK (37)		*/
	NE_INVAL,	/* ENOSYS (38)		*/
	NE_NOTEMPTY,	/* ENOTEMPTY (39)	*/
	NE_INVAL,	/* ELOOP (40)		*/
	NE_INVAL,	/* unused (41)		*/
	NE_INVAL,	/* ENOMSG (42)		*/
	NE_INVAL,	/* EIDRM (43)		*/
	NE_INVAL,	/* ECHRNG (44)		*/
	NE_INVAL,	/* EL2NSYNC (45)	*/
	NE_INVAL,	/* EL3HLT (46)		*/
	NE_INVAL,	/* EL3RST (47)		*/
	NE_INVAL,	/* ELNRNG (48)		*/
	NE_INVAL,	/* EUNATCH (49)		*/
	NE_INVAL,	/* ENOCSI (50)		*/
	NE_INVAL,	/* EL2HLT (51)		*/
	NE_INVAL,	/* EBADE (52)		*/
	NE_INVAL,	/* EBADR (53)		*/
	NE_INVAL,	/* EXFULL (54)		*/
	NE_INVAL,	/* ENOANO (55)		*/
	NE_INVAL,	/* EBADRQC (56)		*/
	NE_INVAL,	/* EBADSLT (57)		*/
	NE_INVAL,	/* unused (58)		*/
	NE_INVAL,	/* EBFONT (59)		*/
	NE_INVAL,	/* ENOSTR (60)		*/
	NE_INVAL,	/* ENODATA (61)		*/
	NE_INVAL,	/* ETIME (62)		*/
	NE_INVAL,	/* ENOSR (63)		*/
	NE_INVAL,	/* ENONET (64)		*/
	NE_INVAL,	/* ENOPKG (65)		*/
	NE_INVAL,	/* EREMOTE (66)		*/
	NE_INVAL,	/* ENOLINK (67)		*/
	NE_INVAL,	/* EADV (68)		*/
	NE_INVAL,	/* ESRMNT (69)		*/
	NE_IO,		/* ECOMM (70)		*/
	NE_IO,		/* EPROTO (71)		*/
	NE_IO,		/* EMULTIHOP (72)	*/
	NE_IO,		/* EDOTDOT (73)		*/
	NE_INVAL,	/* EBADMSG (74)		*/
	NE_INVAL,	/* EOVERFLOW (75)	*/
	NE_INVAL,	/* ENOTUNIQ (76)	*/
	NE_INVAL,	/* EBADFD (77)		*/
	NE_IO,		/* EREMCHG (78)		*/
	NE_IO,		/* ELIBACC (79)		*/
	NE_IO,		/* ELIBBAD (80)		*/
	NE_IO,		/* ELIBSCN (81)		*/
	NE_IO,		/* ELIBMAX (82)		*/
	NE_IO,		/* ELIBEXEC (83)	*/
	NE_INVAL,	/* EILSEQ (84)		*/
	NE_INVAL,	/* ERESTART (85)	*/
	NE_INVAL,	/* ESTRPIPE (86)	*/
	NE_INVAL,	/* EUSERS (87)		*/
	NE_INVAL,	/* ENOTSOCK (88)	*/
	NE_INVAL,	/* EDESTADDRREQ (89)	*/
	NE_INVAL,	/* EMSGSIZE (90)	*/
	NE_INVAL,	/* EPROTOTYPE (91)	*/
	NE_INVAL,	/* ENOPROTOOPT (92)	*/
	NE_INVAL,	/* EPROTONOSUPPORT (93) */
	NE_INVAL,	/* ESOCKTNOSUPPORT (94) */
	NE_INVAL,	/* EOPNOTSUPP (95)	*/
	NE_INVAL,	/* EPFNOSUPPORT (96)	*/
	NE_INVAL,	/* EAFNOSUPPORT (97)	*/
	NE_INVAL,	/* EADDRINUSE (98)	*/
	NE_INVAL,	/* EADDRNOTAVAIL (99)	*/
	NE_IO,		/* ENETDOWN (100)	*/
	NE_IO,		/* ENETUNREACH (101)	*/
	NE_IO,		/* ENETRESET (102)	*/
	NE_IO,		/* ECONNABORTED (103)	*/
	NE_IO,		/* ECONNRESET (104)	*/
	NE_IO,		/* ENOBUFS (105)	*/
	NE_IO,		/* EISCONN (106)	*/
	NE_IO,		/* ENOTCONN (107)	*/
	NE_IO,		/* ESHUTDOWN (108)	*/
	NE_IO,		/* ETOOMANYREFS (109)	*/
	NE_IO,		/* ETIMEDOUT (110)	*/
	NE_IO,		/* ECONNREFUSED (111)	*/
	NE_IO,		/* EHOSTDOWN (112)	*/
	NE_IO,		/* EHOSTUNREACH (113)	*/
	NE_IO,		/* EALREADY (114)	*/
	NE_IO,		/* EINPROGRESS (115)	*/
	NE_STALE,	/* ESTALE (116)		*/
	NE_IO,		/* EUCLEAN (117)	*/
	NE_INVAL,	/* ENOTNAM (118)	*/
	NE_INVAL,	/* ENAVAIL (119)	*/
	NE_INVAL,	/* EISNAM (120)		*/
	NE_IO,		/* EREMOTEIO (121)	*/
	NE_DQUOT,	/* EDQUOT (122)		*/
};


int
linux_nfs_error(int e)
{
  if (e < NFS_LOMAP || e > NFS_HIMAP)
    return (nfsstat)NE_IO;
  e = nfs_errormap[e - NFS_LOMAP];
  return (nfsstat)e;
}


#ifdef HAVE_LOOP_DEVICE
/****************************************************************************/
/*** LOOP DEVICE SUPPORT						  ***/
/*** Loop Device setup code taken from mount-2.11g-5.src.rpm, which was   ***/
/*** originally written bt Ted T'so and others.				  ***/
/****************************************************************************/

#define PROC_DEVICES	"/proc/devices"

#if not_used_yet
static int
show_loop(char *device)
{
  struct loop_info loopinfo;
  int fd;

  if ((fd = open(device, O_RDONLY)) < 0) {
    dlog("loop: can't open device %s: %m", device);
    return -2;
  }
  if (ioctl(fd, LOOP_GET_STATUS, &loopinfo) < 0) {
    dlog("loop: can't get info on device %s: %m", device);
    close(fd);
    return -1;
  }
  dlog("show_loop: %s: [%04x]:%ld (%s)",
       device, loopinfo.lo_device, loopinfo.lo_inode,
       loopinfo.lo_name);

  close(fd);

  return 0;
}


static int
is_loop_device(const char *device)
{
  struct stat statbuf;
  int loopmajor = 7;

  return (loopmajor && stat(device, &statbuf) == 0 &&
	  S_ISBLK(statbuf.st_mode) &&
	  (statbuf.st_rdev>>8) == loopmajor);
}
#endif /* not_used_yet */


/*
 * Just creating a device, say in /tmp, is probably a bad idea - people
 * might have problems with backup or so.  So, we just try /dev/loop[0-7].
 */
static char *
find_unused_loop_device(void)
{
  char dev[20];
  char *loop_formats[] = { "/dev/loop%d", "/dev/loop/%d" };
  int i, j, fd, somedev = 0, someloop = 0, loop_known = 0;
  struct stat statbuf;
  struct loop_info loopinfo;
  FILE *procdev;

#define LOOP_FMT_SIZE(a) (sizeof(a)/sizeof(a[0]))
  for (j = 0; j < LOOP_FMT_SIZE(loop_formats); j++) {
    for(i = 0; i < 256; i++) {
      sprintf(dev, loop_formats[j], i);
      if (stat(dev, &statbuf) == 0 && S_ISBLK(statbuf.st_mode)) {
	somedev++;
	fd = open(dev, O_RDONLY);
	if (fd >= 0) {
	  if(ioctl(fd, LOOP_GET_STATUS, &loopinfo) == 0)
	    someloop++;		/* in use */
	  else if (errno == ENXIO) {
	    close(fd);
	    return strdup(dev); /* probably free */
	  }
	  close(fd);
	}
	continue;		/* continue trying as long as devices exist */
      }
      break;
    }
  }

  /* Nothing found. Why not? */
  if ((procdev = fopen(PROC_DEVICES, "r")) != NULL) {
    char line[100];
    while (fgets(line, sizeof(line), procdev))
      if (strstr(line, " loop\n")) {
	loop_known = 1;
	break;
      }
    fclose(procdev);
    if (!loop_known)
      loop_known = -1;
  }

  if (!somedev) {
    dlog("Could not find any device /dev/loop#");
  } else if(!someloop) {
    if (loop_known == 1) {
      dlog("Could not find any loop device.");
      dlog("...Maybe /dev/loop# has a wrong major number?");
    }
    else if (loop_known == -1) {
      dlog("Could not find any loop device, and, according to %s,", PROC_DEVICES);
      dlog("...this kernel does not know about the loop device.");
      dlog("... (If so, then recompile or `insmod loop.o'.)");
    } else {
      dlog("Could not find any loop device. Maybe this kernel does not know,");
      dlog("...about the loop device (then recompile or `insmod loop.o'), or");
      dlog("...maybe /dev/loop# has the wrong major number?");
    }
  } else {
    dlog("Could not find any free loop device!");
  }
  return NULL;
}


/* returns 0 if OK, -1 otherwise */
char *
setup_loop_device(const char *file)
{
  struct loop_info loopinfo;
  int fd, ffd, mode, err = -1;
  char *device = find_unused_loop_device();

  if (!device) {
    dlog("no unused loop device");
    goto out;
  }

  mode = O_RDWR | O_LARGEFILE;
  if ((ffd = open(file, mode)) < 0) {
    if (errno == EROFS) {
      mode = O_RDONLY | O_LARGEFILE;
      ffd = open(file, mode);
    }
    if (ffd < 0) {
      dlog("%s: %m", file);
      goto out;
    }
  }
  if ((fd = open(device, mode)) < 0) {
    dlog("%s: %m", device);
    goto out_close;
  }

  memset(&loopinfo, 0, sizeof(loopinfo));
  strncpy(loopinfo.lo_name, file, LO_NAME_SIZE-1);
  loopinfo.lo_name[LO_NAME_SIZE-1] = '\0';
  loopinfo.lo_offset = 0;

  if (ioctl(fd, LOOP_SET_FD, ffd) < 0) {
    dlog("ioctl: LOOP_SET_FD: %m");
    goto out_close_all;
  }
  if (ioctl(fd, LOOP_SET_STATUS, &loopinfo) < 0) {
    (void) ioctl(fd, LOOP_CLR_FD, 0);
    dlog("ioctl: LOOP_SET_STATUS: %m");
    goto out_close_all;
  }

  /* if gets here, all is OK */
  err = 0;

out_close_all:
  close(fd);
out_close:
  close(ffd);
out:

  if (err) {
    XFREE(device);
    return NULL;
  } else {
    dlog("setup_loop_device(%s,%s): success", device, file);
    return device;
  }
}


int
delete_loop_device(const char *device)
{
  int fd;

  if ((fd = open(device, O_RDONLY)) < 0) {
    dlog("delete_loop_device: can't delete device %s: %m", device);
    return -1;
  }
  if (ioctl(fd, LOOP_CLR_FD, 0) < 0) {
    dlog("ioctl: LOOP_CLR_FD: %m");
    return -1;
  }
  close(fd);
  dlog("delete_loop_device(%s): success", device);
  return 0;
}
#endif /* HAVE_LOOP_DEVICE */


/****************************************************************************/
