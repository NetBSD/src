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
 * $Id: ops_nfs.c,v 1.1.1.1 1997/07/24 21:21:44 christos Exp $
 *
 */

/*
 * Network file system
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

/*
 * Convert from nfsstat to UN*X error code
 */
#define unx_error(e)	((int)(e))

/*
 * FH_TTL is the time a file handle will remain in the cache since
 * last being used.  If the file handle becomes invalid, then it
 * will be flushed anyway.
 */
#define	FH_TTL			(5 * 60) /* five minutes */
#define	FH_TTL_ERROR		(30) /* 30 seconds */
#define	FHID_ALLOC(struct)	(++fh_id)

/*
 * The NFS layer maintains a cache of file handles.
 * This is *fundamental* to the implementation and
 * also allows quick remounting when a filesystem
 * is accessed soon after timing out.
 *
 * The NFS server layer knows to flush this cache
 * when a server goes down so avoiding stale handles.
 *
 * Each cache entry keeps a hard reference to
 * the corresponding server.  This ensures that
 * the server keepalive information is maintained.
 *
 * The copy of the sockaddr_in here is taken so
 * that the port can be twiddled to talk to mountd
 * instead of portmap or the NFS server as used
 * elsewhere.
 * The port# is flushed if a server goes down.
 * The IP address is never flushed - we assume
 * that the address of a mounted machine never
 * changes.  If it does, then you have other
 * problems...
 */
typedef struct fh_cache fh_cache;
struct fh_cache {
  qelem			fh_q;		/* List header */
  voidp			fh_wchan;	/* Wait channel */
  int			fh_error;	/* Valid data? */
  int			fh_id;		/* Unique id */
  int			fh_cid;		/* Callout id */
  u_long		fh_nfs_version;	/* highest NFS version on host */
  am_nfs_handle_t	fh_nfs_handle;	/* Handle on filesystem */
  struct sockaddr_in	fh_sin;		/* Address of mountd */
  fserver		*fh_fs;		/* Server holding filesystem */
  char			*fh_path;	/* Filesystem on host */
};

/* forward definitions */
static char * nfs_match(am_opts *fo);
static int call_mountd(fh_cache *fp, u_long proc, fwd_fun f, voidp wchan);
static int fh_id = 0;
static int nfs_fmount(mntfs *mf);
static int nfs_fumount(mntfs *mf);
static int nfs_init(mntfs *mf);
static void nfs_umounted(am_node *mp);

/* globals */
AUTH *nfs_auth;
qelem fh_head = {&fh_head, &fh_head};

/*
 * Network file system operations
 */
am_ops nfs_ops =
{
  "nfs",
  nfs_match,
  nfs_init,
  auto_fmount,
  nfs_fmount,
  auto_fumount,
  nfs_fumount,
  efs_lookuppn,
  efs_readdir,
  0,				/* nfs_readlink */
  0,				/* nfs_mounted */
  nfs_umounted,
  find_nfs_srvr,
  FS_MKMNT | FS_BACKGROUND | FS_AMQINFO
};


static fh_cache *
find_nfs_fhandle_cache(voidp idv, int done)
{
  fh_cache *fp, *fp2 = 0;
  int id = (long) idv;		/* for 64-bit archs */

  ITER(fp, fh_cache, &fh_head) {
    if (fp->fh_id == id) {
      fp2 = fp;
      break;
    }
  }

#ifdef DEBUG
  if (fp2)
    dlog("fh cache gives fp %#x, fs %s", fp2, fp2->fh_path);
  else
    dlog("fh cache search failed");
#endif /* DEBUG */

  if (fp2 && !done) {
    fp2->fh_error = ETIMEDOUT;
    return 0;
  }

  return fp2;
}


/*
 * Called when a filehandle appears
 */
static void
got_nfs_fh(voidp pkt, int len, struct sockaddr_in * sa, struct sockaddr_in * ia, voidp idv, int done)
{
  fh_cache *fp;

  fp = find_nfs_fhandle_cache(idv, done);
  if (!fp)
    return;

  /*
   * retrieve the correct RPC reply for the file handle, based on the
   * NFS protocol version.
   */
#ifdef HAVE_FS_NFS3
  if (fp->fh_nfs_version == NFS_VERSION3)
    fp->fh_error = pickup_rpc_reply(pkt, len, (voidp) &fp->fh_nfs_handle.v3,
				    (XDRPROC_T_TYPE) xdr_mountres3);
  else
#endif /* HAVE_FS_NFS3 */
    fp->fh_error = pickup_rpc_reply(pkt, len, (voidp) &fp->fh_nfs_handle.v2,
				    (XDRPROC_T_TYPE) xdr_fhstatus);

  if (!fp->fh_error) {
#ifdef DEBUG
    dlog("got filehandle for %s:%s", fp->fh_fs->fs_host, fp->fh_path);
#endif /* DEBUG */

    /*
     * Wakeup anything sleeping on this filehandle
     */
    if (fp->fh_wchan) {
#ifdef DEBUG
      dlog("Calling wakeup on %#x", fp->fh_wchan);
#endif /* DEBUG */
      wakeup(fp->fh_wchan);
    }
  }
}


void
flush_nfs_fhandle_cache(fserver *fs)
{
  fh_cache *fp;

  ITER(fp, fh_cache, &fh_head) {
    if (fp->fh_fs == fs || fs == 0) {
      fp->fh_sin.sin_port = (u_short) 0;
      fp->fh_error = -1;
    }
  }
}


static void
discard_fh(fh_cache *fp)
{
  rem_que(&fp->fh_q);
#ifdef DEBUG
  dlog("Discarding filehandle for %s:%s", fp->fh_fs->fs_host, fp->fh_path);
#endif /* DEBUG */
  free_srvr(fp->fh_fs);
  free((voidp) fp->fh_path);
  free((voidp) fp);
}


/*
 * Determine the file handle for a node
 */
static int
prime_nfs_fhandle_cache(char *path, fserver *fs, am_nfs_handle_t *fhbuf, voidp wchan)
{
  fh_cache *fp, *fp_save = 0;
  int error;
  int reuse_id = FALSE;

#ifdef DEBUG
  dlog("Searching cache for %s:%s", fs->fs_host, path);
#endif /* DEBUG */

  /*
   * First search the cache
   */
  ITER(fp, fh_cache, &fh_head) {
    if (fs == fp->fh_fs && STREQ(path, fp->fh_path)) {
      switch (fp->fh_error) {
      case 0:
	plog(XLOG_INFO, "prime_nfs_fhandle_cache: NFS version %d", fp->fh_nfs_version);
#ifdef HAVE_FS_NFS3
	if (fp->fh_nfs_version == NFS_VERSION3)
	  error = fp->fh_error = unx_error(fp->fh_nfs_handle.v3.fhs_status);
	else
#endif /* HAVE_FS_NFS3 */
	  error = fp->fh_error = unx_error(fp->fh_nfs_handle.v2.fhs_status);
	if (error == 0) {
	  if (fhbuf) {
#ifdef HAVE_FS_NFS3
	    if (fp->fh_nfs_version == NFS_VERSION3)
	      memmove((voidp) &(fhbuf->v3), (voidp) &(fp->fh_nfs_handle.v3),
		      sizeof(fp->fh_nfs_handle.v3));
	    else
#endif /* HAVE_FS_NFS3 */
	      memmove((voidp) &(fhbuf->v2), (voidp) &(fp->fh_nfs_handle.v2),
		      sizeof(fp->fh_nfs_handle.v2));
	  }
	  if (fp->fh_cid)
	    untimeout(fp->fh_cid);
	  fp->fh_cid = timeout(FH_TTL, discard_fh, (voidp) fp);
	} else if (error == EACCES) {
	  /*
	   * Now decode the file handle return code.
	   */
	  plog(XLOG_INFO, "Filehandle denied for \"%s:%s\"",
	       fs->fs_host, path);
	} else {
	  errno = error;	/* XXX */
	  plog(XLOG_INFO, "Filehandle error for \"%s:%s\": %m",
	       fs->fs_host, path);
	}

	/*
	 * The error was returned from the remote mount daemon.
	 * Policy: this error will be cached for now...
	 */
	return error;

      case -1:
	/*
	 * Still thinking about it, but we can re-use.
	 */
	fp_save = fp;
	reuse_id = TRUE;
	break;

      default:
	/*
	 * Return the error.
	 * Policy: make sure we recompute if required again
	 * in case this was caused by a network failure.
	 * This can thrash mountd's though...  If you find
	 * your mountd going slowly then:
	 * 1.  Add a fork() loop to main.
	 * 2.  Remove the call to innetgr() and don't use
	 *     netgroups, especially if you don't use YP.
	 */
	error = fp->fh_error;
	fp->fh_error = -1;
	return error;
      }
      break;
    }
  }

  /*
   * Not in cache
   */
  if (fp_save) {
    fp = fp_save;
    /*
     * Re-use existing slot
     */
    untimeout(fp->fh_cid);
    free_srvr(fp->fh_fs);
    free(fp->fh_path);
  } else {
    fp = ALLOC(struct fh_cache);
    memset((voidp) fp, 0, sizeof(struct fh_cache));
    ins_que(&fp->fh_q, &fh_head);
  }
  if (!reuse_id)
    fp->fh_id = FHID_ALLOC(struct );
  fp->fh_wchan = wchan;
  fp->fh_error = -1;
  fp->fh_cid = timeout(FH_TTL, discard_fh, (voidp) fp);

  /*
   * If the address has changed then don't try to re-use the
   * port information
   */
  if (fp->fh_sin.sin_addr.s_addr != fs->fs_ip->sin_addr.s_addr) {
    fp->fh_sin = *fs->fs_ip;
    fp->fh_sin.sin_port = 0;
    fp->fh_nfs_version = fs->fs_version;
  }
  fp->fh_fs = dup_srvr(fs);
  fp->fh_path = strdup(path);

  error = call_mountd(fp, MOUNTPROC_MNT, got_nfs_fh, wchan);
  if (error) {
    /*
     * Local error - cache for a short period
     * just to prevent thrashing.
     */
    untimeout(fp->fh_cid);
    fp->fh_cid = timeout(error < 0 ? 2 * ALLOWED_MOUNT_TIME : FH_TTL_ERROR,
			 discard_fh, (voidp) fp);
    fp->fh_error = error;
  } else {
    error = fp->fh_error;
  }

  return error;
}


int
make_nfs_auth(void)
{

#ifdef HAVE_TRANSPORT_TYPE_TLI
  nfs_auth = authsys_create_default();
#else /* not HAVE_TRANSPORT_TYPE_TLI */
  nfs_auth = authunix_create_default();
#endif /* not HAVE_TRANSPORT_TYPE_TLI */

  if (!nfs_auth)
    return ENOBUFS;

  return 0;
}


static int
call_mountd(fh_cache *fp, u_long proc, fwd_fun f, voidp wchan)
{
  struct rpc_msg mnt_msg;
  int len;
  char iobuf[8192];
  int error;
  u_long mnt_version;

  if (!nfs_auth) {
    error = make_nfs_auth();
    if (error)
      return error;
  }

  if (fp->fh_sin.sin_port == 0) {
    u_short port;
    error = nfs_srvr_port(fp->fh_fs, &port, wchan);
    if (error)
      return error;
    fp->fh_sin.sin_port = port;
  }

  /* find the right version of the mount protocol */
#ifdef HAVE_FS_NFS3
  if (fp->fh_nfs_version == NFS_VERSION3)
    mnt_version = MOUNTVERS3;
  else
#endif /* HAVE_FS_NFS3 */
    mnt_version = MOUNTVERS;
  plog(XLOG_INFO, "call_mountd: NFS version %d, mount version %d",
       fp->fh_nfs_version, mnt_version);

  rpc_msg_init(&mnt_msg, MOUNTPROG, mnt_version, MOUNTPROC_NULL);
  len = make_rpc_packet(iobuf,
			sizeof(iobuf),
			proc,
			&mnt_msg,
			(voidp) &fp->fh_path,
			(XDRPROC_T_TYPE) xdr_nfspath,
			nfs_auth);

  if (len > 0) {
    error = fwd_packet(MK_RPC_XID(RPC_XID_MOUNTD, fp->fh_id),
		       (voidp) iobuf,
		       len,
		       &fp->fh_sin,
		       &fp->fh_sin,
		       (voidp) ((long) fp->fh_id), /* for 64-bit archs */
		       f);
  } else {
    error = -len;
  }

/*
 * It may be the case that we're sending to the wrong MOUNTD port.  This
 * occurs if mountd is restarted on the server after the port has been
 * looked up and stored in the filehandle cache somewhere.  The correct
 * solution, if we're going to cache port numbers is to catch the ICMP
 * port unreachable reply from the server and cause the portmap request
 * to be redone.  The quick solution here is to invalidate the MOUNTD
 * port.
 */
  fp->fh_sin.sin_port = 0;

  return error;
}


/*
 * NFS needs the local filesystem, remote filesystem
 * remote hostname.
 * Local filesystem defaults to remote and vice-versa.
 */
static char *
nfs_match(am_opts *fo)
{
  char *xmtab;

  if (fo->opt_fs && !fo->opt_rfs)
    fo->opt_rfs = fo->opt_fs;
  if (!fo->opt_rfs) {
    plog(XLOG_USER, "nfs: no remote filesystem specified");
    return NULL;
  }
  if (!fo->opt_rhost) {
    plog(XLOG_USER, "nfs: no remote host specified");
    return NULL;
  }

  /*
   * Determine magic cookie to put in mtab
   */
  xmtab = (char *) xmalloc(strlen(fo->opt_rhost) + strlen(fo->opt_rfs) + 2);
  sprintf(xmtab, "%s:%s", fo->opt_rhost, fo->opt_rfs);
#ifdef DEBUG
  dlog("NFS: mounting remote server \"%s\", remote fs \"%s\" on \"%s\"",
       fo->opt_rhost, fo->opt_rfs, fo->opt_fs);
#endif /* DEBUG */

  return xmtab;
}


/*
 * Initialise am structure for nfs
 */
static int
nfs_init(mntfs *mf)
{
  int error;
  am_nfs_handle_t fhs;
  char *colon;

  if (mf->mf_private)
    return 0;

  colon = strchr(mf->mf_info, ':');
  if (colon == 0)
    return ENOENT;

  error = prime_nfs_fhandle_cache(colon + 1, mf->mf_server, &fhs, (voidp) mf);
  if (!error) {
    mf->mf_private = (voidp) ALLOC(am_nfs_handle_t);
    mf->mf_prfree = (void (*)(voidp)) free;
    memmove(mf->mf_private, (voidp) &fhs, sizeof(fhs));
  }
  return error;
}


int
mount_nfs_fh(am_nfs_handle_t *fhp, char *dir, char *fs_name, char *opts, mntfs *mf)
{
  MTYPE_TYPE type;
  char *colon;
  char *xopts;
  char host[MAXHOSTNAMELEN + MAXPATHLEN + 2];
  fserver *fs = mf->mf_server;
  u_long nfs_version = fs->fs_version;
  char *nfs_proto = fs->fs_proto; /* "tcp" or "udp" */
  int error;
  int flags;
  int retry;
  mntent_t mnt;
  nfs_args_t nfs_args;
#ifdef HAVE_FS_NFS3
  am_nfs_fh3 fh3;
#endif /* HAVE_FS_NFS3 */
#ifdef HAVE_TRANSPORT_TYPE_TLI
  struct netbuf nb;	/* automatic space for addr field of nfs_args */
#endif /* HAVE_TRANSPORT_TYPE_TLI */
#if defined(HAVE_FS_NFS3) || defined(HAVE_TRANSPORT_TYPE_TLI)
  char *nc_protoname;
#endif /* defined(HAVE_FS_NFS3) || defined(HAVE_TRANSPORT_TYPE_TLI) */
#if defined(MNTTAB_OPT_ACREGMIN) || defined(MNTTAB_OPT_ACTIMEO)
  int acval = 0;
#endif /* defined(MNTTAB_OPT_ACREGMIN) || defined(MNTTAB_OPT_ACTIMEO) */

  memset((voidp) &nfs_args, 0, sizeof(nfs_args)); /* Paranoid */

  /*
   * Extract host name to give to kernel
   * Some systems like osf1/aix3/bsd44 variants may need old code
   * for NFS_ARGS_NEEDS_PATH.
   */
  if (!(colon = strchr(fs_name, ':')))
    return ENOENT;
#ifdef MOUNT_TABLE_ON_FILE
  *colon = '\0';
#endif /* MOUNT_TABLE_ON_FILE */
  strncpy(host, fs_name, sizeof(host));
#ifdef MOUNT_TABLE_ON_FILE
  *colon = ':';
#endif /* MOUNT_TABLE_ON_FILE */

  if (mf->mf_remopts && *mf->mf_remopts && !islocalnet(fs->fs_ip->sin_addr.s_addr))
    xopts = strdup(mf->mf_remopts);
  else
    xopts = strdup(opts);

  mnt.mnt_dir = dir;
  mnt.mnt_fsname = fs_name;
  mnt.mnt_opts = xopts;

  /*
   * Set mount types accordingly
   */
#ifndef HAVE_FS_NFS3
  type = MOUNT_TYPE_NFS;
  mnt.mnt_type = MNTTAB_TYPE_NFS;
#else /* HAVE_FS_NFS3 */
  if (nfs_version == NFS_VERSION3) {
    type = MOUNT_TYPE_NFS3;
    mnt.mnt_type = MNTTAB_TYPE_NFS3;
  } else {
    type = MOUNT_TYPE_NFS;
    mnt.mnt_type = MNTTAB_TYPE_NFS;
  }
#endif /* HAVE_FS_NFS3 */
  plog(XLOG_INFO, "mount_nfs_fh: NFS version %d", nfs_version);

#if defined(HAVE_FS_NFS3) && defined(HAVE_TRANSPORT_TYPE_TLI)
  nc_protoname = nfs_proto;
  plog(XLOG_INFO, "mount_nfs_fh: using NFS transport %s", nc_protoname);
#endif /* defined(HAVE_FS_NFS3) && defined(HAVE_TRANSPORT_TYPE_TLI) */

#ifdef MNT2_NFS_OPT_TCP
  if (STREQ(nfs_proto, "tcp"))
    nfs_args.flags |= MNT2_NFS_OPT_TCP;
#endif /* MNT2_NFS_OPT_TCP */

#ifdef HAVE_FIELD_NFS_ARGS_T_SOTYPE
  /* bsdi3 uses this */
  if (STREQ(nfs_proto, "tcp"))
    nfs_args.sotype = SOCK_STREAM;
  else
    nfs_args.sotype = SOCK_DGRAM;
#endif /* HAVE_FIELD_NFS_ARGS_T_SOTYPE */

#ifdef HAVE_FIELD_NFS_ARGS_T_PROTO
  nfs_args.proto = 0;		/* bsdi3 sets this field to zero  */
#endif /* HAVE_FIELD_NFS_ARGS_T_SOTYPE */

  retry = hasmntval(&mnt, MNTTAB_OPT_RETRY);
  if (retry <= 0)
    retry = 1;			/* XXX */

  /*
   * Set mount arguments:
   * first, the NFS file handle.
   */
#ifdef HAVE_FS_NFS3
  if (nfs_version == NFS_VERSION3) {
    fh3.fh3_length = fhp->v3.mountres3_u.mountinfo.fhandle.fhandle3_len;
    memmove(fh3.fh3_u.data,
	    fhp->v3.mountres3_u.mountinfo.fhandle.fhandle3_val,
	    fh3.fh3_length);

# if defined(HAVE_FIELD_NFS_ARGS_T_FHSIZE) || defined(HAVE_FIELD_NFS_ARGS_T_FH_LEN)
    /*
     * Some systems (Irix/bsdi3) have a separate field in nfs_args for
     * the length of the file handle for NFS V3.  They insist that
     * the file handle set in nfs_args be plain bytes, and not
     * include the length field.
     */
    NFS_FH_DREF(nfs_args.NFS_FH_FIELD, &(fh3.fh3_u.data));
# else /* not defined(HAVE_FIELD_NFS_ARGS_T_FHSIZE) || defined(HAVE_FIELD_NFS_ARGS_T_FH_LEN) */
    NFS_FH_DREF(nfs_args.NFS_FH_FIELD, &fh3);
# endif /* not defined(HAVE_FIELD_NFS_ARGS_T_FHSIZE) || defined(HAVE_FIELD_NFS_ARGS_T_FH_LEN) */
# ifdef MNT2_NFS_OPT_NFSV3
    nfs_args.flags |= MNT2_NFS_OPT_NFSV3;
# endif /* MNT2_NFS_OPT_NFSV3 */
  } else
#endif /* HAVE_FS_NFS3 */
    NFS_FH_DREF(nfs_args.NFS_FH_FIELD, &(fhp->v2.fhs_fh));


#ifdef HAVE_FIELD_NFS_ARGS_T_FHSIZE
# ifdef HAVE_FS_NFS3
  if (nfs_version == NFS_VERSION3)
    nfs_args.fhsize = fh3.fh3_length;
  else
# endif /* HAVE_FS_NFS3 */
    nfs_args.fhsize = FHSIZE;
#endif /* HAVE_FIELD_NFS_ARGS_T_FHSIZE */

#ifdef HAVE_FIELD_NFS_ARGS_T_FH_LEN
# ifdef HAVE_FS_NFS3
  if (nfs_version == NFS_VERSION3)
    nfs_args.fh_len = fh3.fh3_length;
  else
# endif /* HAVE_FS_NFS3 */
    nfs_args.fh_len = FHSIZE;
#endif /* HAVE_FIELD_NFS_ARGS_T_FH_LEN */

#ifdef HAVE_TRANSPORT_TYPE_TLI
  /* set up syncaddr field */
  nfs_args.syncaddr = (struct netbuf *) NULL;

  /* set up knconf field */
  if (get_knetconfig(&nfs_args.knconf, NULL, nc_protoname) < 0) {
    plog(XLOG_FATAL, "cannot fill knetconfig structure for nfs_args");
    going_down(1);
  }
  /* update the flags field for knconf */
  nfs_args.flags |= MNT2_NFS_OPT_KNCONF;
#endif /* HAVE_TRANSPORT_TYPE_TLI */

#ifdef MAXHOSTNAMELEN
  /*
   * Most kernels have a name length restriction.
   */
  if (strlen(host) >= MAXHOSTNAMELEN)
      strcpy(host + MAXHOSTNAMELEN - 3, "..");
#endif /* MAXHOSTNAMELEN */

  NFS_HN_DREF(nfs_args.hostname, host);

#ifdef MNT2_NFS_OPT_HOSTNAME
  nfs_args.flags |= MNT2_NFS_OPT_HOSTNAME;
#endif /* MNT2_NFS_OPT_HOSTNAME */

#ifdef MNT2_NFS_OPT_FSNAME
  nfs_args.fsname = fs_name;
  nfs_args.flags |= MNT2_NFS_OPT_FSNAME;
#endif /* MNT2_NFS_OPT_FSNAME */

  nfs_args.rsize = hasmntval(&mnt, MNTTAB_OPT_RSIZE);
#ifdef MNT2_NFS_OPT_RSIZE
  if (nfs_args.rsize)
    nfs_args.flags |= MNT2_NFS_OPT_RSIZE;
#endif /* MNT2_NFS_OPT_RSIZE */

  nfs_args.wsize = hasmntval(&mnt, MNTTAB_OPT_WSIZE);
#ifdef MNT2_NFS_OPT_WSIZE
  if (nfs_args.wsize)
    nfs_args.flags |= MNT2_NFS_OPT_WSIZE;
#endif /* MNT2_NFS_OPT_WSIZE */

  nfs_args.timeo = hasmntval(&mnt, MNTTAB_OPT_TIMEO);
#ifdef MNT2_NFS_OPT_TIMEO
  if (nfs_args.timeo)
    nfs_args.flags |= MNT2_NFS_OPT_TIMEO;
#endif /* MNT2_NFS_OPT_TIMEO */

  nfs_args.retrans = hasmntval(&mnt, MNTTAB_OPT_RETRANS);
#ifdef MNT2_NFS_OPT_RETRANS
  if (nfs_args.retrans)
    nfs_args.flags |= MNT2_NFS_OPT_RETRANS;
#endif /* MNT2_NFS_OPT_RETRANS */

#ifdef MNT2_NFS_OPT_BIODS
  if ((nfs_args.biods = hasmntval(&mnt, MNTTAB_OPT_BIODS)))
    nfs_args.flags |= MNT2_NFS_OPT_BIODS;
#endif /* MNT2_NFS_OPT_BIODS */

  if (hasmntopt(&mnt, MNTTAB_OPT_SOFT) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_SOFT;

#ifdef MNT2_NFS_OPT_SPONGY
  if (hasmntopt(&mnt, MNTTAB_OPT_SPONGY) != NULL) {
    nfs_args.flags |= MNT2_NFS_OPT_SPONGY;
    if (nfs_args.flags & MNT2_NFS_OPT_SOFT) {
      plog(XLOG_USER, "Mount opts soft and spongy are incompatible - soft ignored");
      nfs_args.flags &= ~MNT2_NFS_OPT_SOFT;
    }
  }
#endif /* MNT2_NFS_OPT_SPONGY */

#ifdef MNTTAB_OPT_INTR
  if (hasmntopt(&mnt, MNTTAB_OPT_INTR) != NULL)
    /*
     * Either turn on the "allow interrupts" option, or
     * turn off the "disallow interrupts" option"
     */
# ifdef MNT2_NFS_OPT_INT
    nfs_args.flags |= MNT2_NFS_OPT_INT;
# endif /* MNT2_NFS_OPT_INT */
# ifdef MNT2_NFS_OPT_NOINT
    nfs_args.flags &= ~MNT2_NFS_OPT_NOINT;
# endif /* MNT2_NFS_OPT_NOINT */
#endif /* MNTTAB_OPT_INTR */

#ifdef MNTTAB_OPT_NODEVS
  if (hasmntopt(&mnt, MNTTAB_OPT_NODEVS) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_NODEVS;
#endif /* MNTTAB_OPT_NODEVS */

#ifdef MNTTAB_OPT_COMPRESS
  if (hasmntopt(&mnt, MNTTAB_OPT_COMPRESS) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_COMPRESS;
#endif /* MNTTAB_OPT_COMPRESS */

#ifdef MNTTAB_OPT_NOCONN
  if (hasmntopt(&mnt, MNTTAB_OPT_NOCONN) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_NOCONN;
#endif /* MNTTAB_OPT_NOCONN */

#ifdef MNT2_NFS_OPT_RESVPORT
# ifdef MNTTAB_OPT_RESVPORT
  if (hasmntopt(&mnt, MNTTAB_OPT_RESVPORT) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_RESVPORT;
# else /* not MNTTAB_OPT_RESVPORT */
  nfs_args.flags |= MNT2_NFS_OPT_RESVPORT;
# endif /* not MNTTAB_OPT_RESVPORT */
#endif /* MNT2_NFS_OPT_RESVPORT */

#ifdef MNTTAB_OPT_NOAC		/* don't cache attributes */
  if (hasmntopt(&mnt, MNTTAB_OPT_NOAC) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_NOAC;
#endif /* MNTTAB_OPT_NOAC */

/*
 * Attribue cache settings for NFS mounts.
 * Scott Presnell, srp@cgl.ucsf.edu, Sat Nov 14 12:42:16 PST 1992
 */
#ifdef MNTTAB_OPT_ACTIMEO
				/* attr cache timeout (sec) */
  if ((acval = hasmntval(&mnt, MNTTAB_OPT_ACTIMEO))) {
# if defined(MNT2_NFS_OPT_ACREGMIN) && defined(MNT2_NFS_OPT_ACREGMAX)
    nfs_args.flags |= (MNT2_NFS_OPT_ACREGMIN | MNT2_NFS_OPT_ACREGMAX);
# endif /* defined(MNT2_NFS_OPT_ACREGMIN) && defined(MNT2_NFS_OPT_ACREGMAX) */
# ifdef HAVE_FIELD_NFS_ARGS_T_ACREGMIN
    nfs_args.acregmin = acval;
    nfs_args.acregmax = acval;
# endif /* HAVE_FIELD_NFS_ARGS_T_ACREGMIN */

#if defined(MNT2_NFS_OPT_ACDIRMIN) && defined(MNT2_NFS_OPT_ACDIRMAX)
    nfs_args.flags |= (MNT2_NFS_OPT_ACDIRMIN | MNT2_NFS_OPT_ACDIRMAX);
#endif /* defined(MNT2_NFS_OPT_ACDIRMIN) && defined(MNT2_NFS_OPT_ACDIRMAX) */
# ifdef HAVE_FIELD_NFS_ARGS_T_ACDIRMIN
    nfs_args.acdirmin = acval;
    nfs_args.acdirmax = acval;
# endif /* HAVE_FIELD_NFS_ARGS_T_ACDIRMIN */
  }
#endif /* MNTTAB_OPT_ACTIMEO */

#ifdef MNTTAB_OPT_ACREGMIN
				/* min ac timeout for reg files (sec) */
  if (!acval && (nfs_args.acregmin = hasmntval(&mnt, MNTTAB_OPT_ACREGMIN)))
# ifdef MNT2_NFS_OPT_ACREGMIN
    nfs_args.flags |= MNT2_NFS_OPT_ACREGMIN;
# else /* not MNT2_NFS_OPT_ACREGMIN */
    ;
# endif /* not MNT2_NFS_OPT_ACREGMIN */
#endif /* MNTTAB_OPT_ACREGMIN */

#ifdef MNTTAB_OPT_ACREGMAX
				/* max ac timeout for reg files (sec) */
  if (!acval && (nfs_args.acregmax = hasmntval(&mnt, MNTTAB_OPT_ACREGMAX)))
# ifdef MNT2_NFS_OPT_ACREGMAX
    nfs_args.flags |= MNT2_NFS_OPT_ACREGMAX;
# else /* not MNT2_NFS_OPT_ACREGMAX */
    ;
# endif /* not MNT2_NFS_OPT_ACREGMAX */
#endif /* MNTTAB_OPT_ACREGMAX */

#ifdef MNTTAB_OPT_ACDIRMIN
				/* min ac timeout for dirs (sec) */
  if (!acval && (nfs_args.acdirmin = hasmntval(&mnt, MNTTAB_OPT_ACDIRMIN)))
#ifdef MNT2_NFS_OPT_ACDIRMIN
    nfs_args.flags |= MNT2_NFS_OPT_ACDIRMIN;
# else /* not MNT2_NFS_OPT_ACDIRMIN */
    ;
# endif /* not MNT2_NFS_OPT_ACDIRMIN */
#endif /* MNTTAB_OPT_ACDIRMIN */

#ifdef MNTTAB_OPT_ACDIRMAX
				/* max ac timeout for dirs (sec) */
  if (!acval && (nfs_args.acdirmax = hasmntval(&mnt, MNTTAB_OPT_ACDIRMAX)))
# ifdef MNT2_NFS_OPT_ACDIRMAX
    nfs_args.flags |= MNT2_NFS_OPT_ACDIRMAX;
# else /* not MNT2_NFS_OPT_ACDIRMAX */
    ;
# endif /* not MNT2_NFS_OPT_ACDIRMAX */
#endif /* MNTTAB_OPT_ACDIRMAX */

#ifdef MNTTAB_OPT_PRIVATE	/* mount private, single-client tree */
  if (hasmntopt(&mnt, MNTTAB_OPT_PRIVATE) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_PRIVATE;
#endif /* MNTTAB_OPT_PRIVATE */

#ifdef MNTTAB_OPT_SYMTTL	/* symlink cache time-to-live */
  if ((nfs_args.symttl = hasmntval(&mnt, MNTTAB_OPT_SYMTTL)))
    nfs_args.flags |= MNT2_NFS_OPT_SYMTTL;
#endif /* MNTTAB_OPT_SYMTTL */

#ifdef MNT2_NFS_OPT_PGTHRESH
  if ((nfs_args.pg_thresh = hasmntval(&mnt, MNTTAB_OPT_PGTHRESH)))
    nfs_args.flags |= MNT2_NFS_OPT_PGTHRESH;
#endif /* MNT2_NFS_OPT_PGTHRESH */

#ifdef HAVE_TRANSPORT_TYPE_TLI
  nfs_args.addr = &nb;		/* fill in some automatic space */
#endif /* HAVE_TRANSPORT_TYPE_TLI */

  NFS_SA_DREF(nfs_args, fs->fs_ip); /* bug? do I need this for solaris? */

  flags = compute_mount_flags(&mnt);

#if defined(MNT2_NFS_OPT_NOCTO) && defined(MNTTAB_OPT_NOCTO)
  if (hasmntopt(&mnt, MNTTAB_OPT_NOCTO) != NULL)
    nfs_args.flags |= MNT2_NFS_OPT_NOCTO;
#endif /* defined(MNT2_NFS_OPT_NOCTO) && defined(MNTTAB_OPT_NOCTO) */

#ifdef HAVE_FIELD_NFS_ARGS_T_VERSION
# ifdef NFS_ARGSVERSION
  nfs_args.version = NFS_ARGSVERSION; /* BSDI 3.0 */
# endif /* NFS_ARGSVERSION */
# ifdef DG_MOUNT_NFS_VERSION
  nfs_args.version = DG_MOUNT_NFS_VERSION; /* dg-ux */
# endif /* DG_MOUNT_NFS_VERSION */
#endif /* HAVE_FIELD_NFS_ARGS_VERSION */

  error = mount_fs(&mnt, flags, (caddr_t) & nfs_args, retry, type,
		   nfs_version, nfs_proto);
  free(xopts);

#ifdef HAVE_TRANSPORT_TYPE_TLI
  free_knetconfig(nfs_args.knconf);
#endif /* HAVE_TRANSPORT_TYPE_TLI */

  return error;
}


static int
mount_nfs(char *dir, char *fs_name, char *opts, mntfs *mf)
{
  if (!mf->mf_private) {
    plog(XLOG_ERROR, "Missing filehandle for %s", fs_name);
    return EINVAL;
  }

  return mount_nfs_fh((am_nfs_handle_t *) mf->mf_private, dir, fs_name, opts, mf);
}


static int
nfs_fmount(mntfs *mf)
{
  int error;

  error = mount_nfs(mf->mf_mount, mf->mf_info, mf->mf_mopts, mf);

#ifdef DEBUG
  if (error) {
    errno = error;
    dlog("mount_nfs: %m");
  }
#endif /* DEBUG */

  return error;
}


static int
nfs_fumount(mntfs *mf)
{
  int error = UMOUNT_FS(mf->mf_mount);

  /*
   * Here is some code to unmount 'restarted' file systems.
   * The restarted file systems are marked as 'nfs', not
   * 'host', so we only have the map information for the
   * the top-level mount.  The unmount will fail (EBUSY)
   * if there are anything else from the NFS server mounted
   * below the mount-point.  This code checks to see if there
   * is anything mounted with the same prefix as the
   * file system to be unmounted ("/a/b/c" when unmounting "/a/b").
   * If there is, and it is a 'restarted' file system, we unmount
   * it.
   * Added by Mike Mitchell, mcm@unx.sas.com, 09/08/93
   */
  if (error == EBUSY) {
    mntfs *new_mf;
    int len = strlen(mf->mf_mount);
    int didsome = 0;

    ITER(new_mf, mntfs, &mfhead) {
      if (new_mf->mf_ops != mf->mf_ops ||
	  new_mf->mf_refc > 1 ||
	  mf == new_mf ||
	  ((new_mf->mf_flags & (MFF_MOUNTED | MFF_UNMOUNTING | MFF_RESTART)) == (MFF_MOUNTED | MFF_RESTART)))
	continue;

      if (strncmp(mf->mf_mount, new_mf->mf_mount, len) == 0 &&
	  new_mf->mf_mount[len] == '/') {
	UMOUNT_FS(new_mf->mf_mount);
	didsome = 1;
      }
    }
    if (didsome)
      error = UMOUNT_FS(mf->mf_mount);
  }
  if (error)
    return error;

  return 0;
}


static void
nfs_umounted(am_node *mp)
{
  /*
   * Don't bother to inform remote mountd that we are finished.  Until a
   * full track of filehandles is maintained the mountd unmount callback
   * cannot be done correctly anyway...
   */
  mntfs *mf = mp->am_mnt;
  fserver *fs;
  char *colon, *path;

  if (mf->mf_error || mf->mf_refc > 1)
    return;

  fs = mf->mf_server;

  /*
   * Call the mount daemon on the server to announce that we are not using
   * the fs any more.
   *
   * This is *wrong*.  The mountd should be called when the fhandle is
   * flushed from the cache, and a reference held to the cached entry while
   * the fs is mounted...
   */
  colon = path = strchr(mf->mf_info, ':');
  if (fs && colon) {
    fh_cache f;

#ifdef DEBUG
    dlog("calling mountd for %s", mf->mf_info);
#endif /* DEBUG */
    *path++ = '\0';
    f.fh_path = path;
    f.fh_sin = *fs->fs_ip;
    f.fh_sin.sin_port = (u_short) 0;
    f.fh_nfs_version = fs->fs_version;
    f.fh_fs = fs;
    f.fh_id = 0;
    f.fh_error = 0;
    prime_nfs_fhandle_cache(colon + 1, mf->mf_server, (am_nfs_handle_t *) 0, (voidp) mf);
    call_mountd(&f, MOUNTPROC_UMNT, (fwd_fun) 0, (voidp) 0);
    *colon = ':';
  }
}
