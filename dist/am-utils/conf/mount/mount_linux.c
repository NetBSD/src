/*	$NetBSD: mount_linux.c,v 1.1.1.4 2001/05/13 17:50:17 veego Exp $	*/

/*
 * Copyright (c) 1997-2001 Erez Zadok
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
 * Id: mount_linux.c,v 1.11.2.8 2001/05/01 23:05:55 ib42 Exp
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
  {MNTTAB_OPT_DEV,	1,	MNT2_GEN_OPT_NODEV},
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
  int type;
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

const struct fs_opts null_opts[] = {
  { NULL,	0 }
};


/*
 * New parser for linux-specific mounts Should now handle fs-type specific
 * mount-options correctly Currently implemented: msdos, iso9660
 *
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
	     !NSTREQ(dev_opts->opt, opt, strlen(dev_opts->opt)))
	++dev_opts;
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
static int
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
mount_linux(MTYPE_TYPE type, mntent_t *mnt, int flags, caddr_t data)
{
  char *extra_opts = NULL;
  char *tmp_opts = NULL;
  char *sub_type = NULL;
  int noauto = 0;
  int errorcode;
  nfs_args_t *mnt_data = (nfs_args_t *) data;

  if (mnt->mnt_opts && STREQ (mnt->mnt_opts, "defaults"))
    mnt->mnt_opts = NULL;

  if (type == NULL)
    type = index(mnt->mnt_fsname, ':') ? MOUNT_TYPE_NFS : MOUNT_TYPE_UFS;

  if (STREQ(type, MOUNT_TYPE_NFS)) {
    /* Fake some values for linux */
    mnt_data->version = NFS_MOUNT_VERSION;
    if (!mnt_data->timeo)
      mnt_data->timeo = 7;
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
     * structure.
     */
#ifdef MNT2_NFS_OPT_VER3
    if (mnt_data->flags & MNT2_NFS_OPT_VER3)
      memset(mnt_data->old_root.data, 0, FHSIZE);
    else
#endif /* MNT2_NFS_OPT_VER3 */
      memcpy(mnt_data->old_root.data, mnt_data->root.data, FHSIZE);

#ifdef HAVE_FIELD_NFS_ARGS_T_BSIZE
    /* linux mount version 3 */
    mnt_data->bsize = 0;	/* let the kernel decide */
#endif /* HAVE_FIELD_NFS_ARGS_T_BSIZE */

#ifdef HAVE_FIELD_NFS_ARGS_T_NAMLEN
    /* linux mount version 2 */
    mnt_data->namlen = NAME_MAX;	/* 256 bytes */
#endif /* HAVE_FIELD_NFS_ARGS_T_NAMELEN */

    mnt_data->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mnt_data->fd < 0) {
      plog(XLOG_ERROR, "Can't create socket for kernel");
      errorcode = 1;
      goto fail;
    }
    if (bindresvport(mnt_data->fd, 0) < 0) {
      plog(XLOG_ERROR, "Can't bind to reserved port");
      errorcode = 1;
      goto fail;
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
	goto fail;
      }
    }
#ifdef DEBUG
    amuDebug(D_FULL) {
      plog(XLOG_DEBUG, "linux mount: type %s\n",type);
      plog(XLOG_DEBUG, "linux mount: version %d\n",mnt_data->version);
      plog(XLOG_DEBUG, "linux mount: fd %d\n",mnt_data->fd);
      plog(XLOG_DEBUG, "linux mount: hostname %s\n",
	   inet_ntoa(mnt_data->addr.sin_addr));
      plog(XLOG_DEBUG, "linux mount: port %d\n",
	   htons(mnt_data->addr.sin_port));
    }
#endif /* DEBUG */

  } else {

    /* Non nfs mounts */
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
	plog(XLOG_ERROR, "strdup returned null in mount_linux");
      }
    }

    if (!hasmntopt(mnt, "type"))
      mnt->mnt_type = type;

    /* We only parse opts if non-NFS drive */
    tmp_opts = parse_opts(type, mnt->mnt_opts, &flags, &extra_opts, &noauto);
#ifdef MOUNT_TYPE_LOFS
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
    }
#endif /* MOUNT_TYPE_LOFS */

#ifdef DEBUG
    amuDebug(D_FULL) {
      plog(XLOG_DEBUG, "linux mount: type %s\n", type);
      plog(XLOG_DEBUG, "linux mount: xopts %s\n", extra_opts);
    }
#endif /* DEBUG */
  }

#ifdef DEBUG
  amuDebug(D_FULL) {
    plog(XLOG_DEBUG, "linux mount: fsname %s\n", mnt->mnt_fsname);
    plog(XLOG_DEBUG, "linux mount: type (mntent) %s\n", mnt->mnt_type);
    plog(XLOG_DEBUG, "linux mount: opts %s\n", tmp_opts);
    plog(XLOG_DEBUG, "linux mount: dir %s\n", mnt->mnt_dir);
  }
  amuDebug(D_TRACE) {
    plog(XLOG_DEBUG, "linux mount: Generic mount flags 0x%x", MS_MGC_VAL | flags);
    if (STREQ(type, MOUNT_TYPE_NFS)) {
      plog(XLOG_DEBUG, "linux mount: updated nfs_args...");
      print_nfs_args(mnt_data, 0);
    }
  }
#endif /* DEBUG */

  /*
   * If we have an nfs mount, the 5th argument to system mount() must be the
   * nfs_mount_data structure, otherwise it is the return from parse_opts()
   */
  errorcode = mount(mnt->mnt_fsname,
		    mnt->mnt_dir,
		    type,
		    MS_MGC_VAL | flags,
		    STREQ(type, MOUNT_TYPE_NFS) ? (char *) mnt_data : extra_opts);

  /*
   * If we failed, (i.e. errorcode != 0), then close the socket if its is
   * open.  mnt_data->fd is valid only for NFS.
   */
  if (errorcode && STREQ(type, "nfs") && mnt_data->fd != -1) {
    /* save errno, may be clobbered by close() call! */
    int save_errno = errno;
    close(mnt_data->fd);
    errno = save_errno;
  }

  /*
   * Free all allocated space and return errorcode.
   */
fail:
  if (extra_opts != NULL)
    XFREE(extra_opts);
  if (tmp_opts != NULL)
    XFREE(tmp_opts);
  if (sub_type != NULL)
    XFREE(sub_type);

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

#define NFS_LOMAP	1
#define NFS_HIMAP	122

/*
 * The errno's below are correct for Linux/i386. One day, somebody
 * with lots of energy ought to verify them against the other ports...
 */
static int nfs_errormap[] = {
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

/****************************************************************************/
