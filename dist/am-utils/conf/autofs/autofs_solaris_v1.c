/*	$NetBSD: autofs_solaris_v1.c,v 1.1.1.2 2003/03/09 01:13:20 christos Exp $	*/

/*
 * Copyright (c) 1999-2003 Ion Badulescu
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
 * Id: autofs_solaris_v1.c,v 1.13 2002/12/27 22:43:54 ezk Exp
 *
 */

/*
 * Automounter filesystem
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

/*
 * MACROS:
 */
#ifndef AUTOFS_NULL
# define AUTOFS_NULL	NULLPROC
#endif /* not AUTOFS_NULL */

/*
 * STRUCTURES:
 */

/*
 * VARIABLES:
 */

/* forward declarations */
# ifndef HAVE_XDR_MNTREQUEST
bool_t xdr_mntrequest(XDR *xdrs, mntrequest *objp);
# endif /* not HAVE_XDR_MNTREQUEST */
# ifndef HAVE_XDR_MNTRES
bool_t xdr_mntres(XDR *xdrs, mntres *objp);
# endif /* not HAVE_XDR_MNTRES */
# ifndef HAVE_XDR_UMNTREQUEST
bool_t xdr_umntrequest(XDR *xdrs, umntrequest *objp);
# endif /* not HAVE_XDR_UMNTREQUEST */
# ifndef HAVE_XDR_UMNTRES
bool_t xdr_umntres(XDR *xdrs, umntres *objp);
# endif /* not HAVE_XDR_UMNTRES */
static int autofs_mount_1_req(struct mntrequest *mr, struct mntres *result, struct authunix_parms *cred);
static int autofs_unmount_1_req(struct umntrequest *ur, struct umntres *result, struct authunix_parms *cred);

/****************************************************************************
 *** VARIABLES                                                            ***
 ****************************************************************************/

/****************************************************************************
 *** FUNCTIONS                                                            ***
 ****************************************************************************/

/*
 * AUTOFS XDR FUNCTIONS:
 */
#ifndef HAVE_XDR_MNTREQUEST
bool_t
xdr_mntrequest(XDR *xdrs, mntrequest *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_mntrequest:");

  if (!xdr_string(xdrs, &objp->name, A_MAXNAME))
    return (FALSE);

  if (!xdr_string(xdrs, &objp->map, A_MAXNAME))
    return (FALSE);

  if (!xdr_string(xdrs, &objp->opts, A_MAXOPTS))
    return (FALSE);

  if (!xdr_string(xdrs, &objp->path, A_MAXPATH))
    return (FALSE);

  return (TRUE);
}
#endif /* not HAVE_XDR_MNTREQUEST */


#ifndef HAVE_XDR_MNTRES
bool_t
xdr_mntres(XDR *xdrs, mntres *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_mntres:");

  if (!xdr_int(xdrs, &objp->status))
    return (FALSE);

  return (TRUE);
}
# endif /* not HAVE_XDR_MNTRES */


#ifndef HAVE_XDR_UMNTREQUEST
bool_t
xdr_umntrequest(XDR *xdrs, umntrequest *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_umntrequest:");

  if (!xdr_int(xdrs, &objp->isdirect))
    return (FALSE);

  if (!xdr_u_int(xdrs, (u_int *) &objp->devid))
    return (FALSE);

#ifdef HAVE_UMNTREQUEST_RDEVID
  if (!xdr_u_long(xdrs, &objp->rdevid))
    return (FALSE);
#endif /* HAVE_UMNTREQUEST_RDEVID */

  if (!xdr_pointer(xdrs, (char **) &objp->next, sizeof(umntrequest), (XDRPROC_T_TYPE) xdr_umntrequest))
    return (FALSE);

  return (TRUE);
}
#endif /* not HAVE_XDR_UMNTREQUEST */


#ifndef HAVE_XDR_UMNTRES
bool_t
xdr_umntres(XDR *xdrs, umntres *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_mntres:");

  if (!xdr_int(xdrs, &objp->status))
    return (FALSE);

  return (TRUE);
}
#endif /* not HAVE_XDR_UMNTRES */


/*
 * AUTOFS RPC methods
 */

static int
autofs_mount_1_req(struct mntrequest *m,
		   struct mntres *res,
		   struct authunix_parms *cred)
{
  int err = 0;
  int isdirect = 0;
  am_node *mp, *ap;
  mntfs *mf;

  dlog("MOUNT REQUEST: name=%s map=%s opts=%s path=%s\n",
       m->name, m->map, m->opts, m->path);

  /* find the effective uid/gid from RPC request */
  sprintf(opt_uid, "%d", (int) cred->aup_uid);
  sprintf(opt_gid, "%d", (int) cred->aup_gid);

  mp = find_ap(m->path);
  if (!mp) {
    plog(XLOG_ERROR, "map %s not found", m->path);
    err = ENOENT;
    goto out;
  }

  mf = mp->am_mnt;
  isdirect = (mf->mf_fsflags & FS_DIRECT) ? 1 : 0;
  ap = mf->mf_ops->lookup_child(mp, m->name + isdirect, &err, VLOOK_CREATE);
  if (ap && err < 0)
    ap = mf->mf_ops->mount_child(ap, &err);
  if (ap == NULL) {
    if (err < 0) {
      /* we're working on it */
      amd_stats.d_drops++;
      return 1;
    }
    err = ENOENT;
    goto out;
  }

out:
  if (err) {
    if (isdirect) {
      /* direct mount */
      plog(XLOG_ERROR, "mount of %s failed", m->path);
    } else {
      /* indirect mount */
      plog(XLOG_ERROR, "mount of %s/%s failed", m->path, m->name);
    }
  }

  dlog("MOUNT REPLY: status=%d (%s)\n", err, strerror(err));

  res->status = err;
  return 0;
}


static int
autofs_unmount_1_req(struct umntrequest *ul,
		     struct umntres *res,
		     struct authunix_parms *cred)
{
  int i, err;

  dlog("UNMOUNT REQUEST: dev=%lx rdev=%lx %s\n",
       (u_long) ul->devid,
       (u_long) ul->rdevid,
       ul->isdirect ? "direct" : "indirect");

  /* by default, and if not found, succeed */
  res->status = 0;

  for (i = 0; i <= last_used_map; i++) {
    am_node *mp = exported_ap[i];
    if (mp && mp->am_dev == ul->devid &&
	(ul->rdevid == 0 || mp->am_rdev == ul->rdevid)) {

      /* save RPC context */
      if (!mp->am_transp && current_transp) {
	mp->am_transp = (SVCXPRT *) xmalloc(sizeof(SVCXPRT));
	*(mp->am_transp) = *current_transp;
      }

      err = unmount_mp(mp);

      if (err)
	/* backgrounded, don't reply yet */
	return 1;

      if (exported_ap[i])
	/* unmounting failed, tell the kernel */
	res->status = 1;
    }
  }

  dlog("UNMOUNT REPLY: status=%d\n", res->status);
  return 0;
}


/****************************************************************************/
/* autofs program dispatcher */
static void
autofs_program_1(struct svc_req *rqstp, SVCXPRT *transp)
{
  union {
    mntrequest autofs_mount_1_arg;
    umntrequest autofs_umount_1_arg;
  } argument;
  union {
    mntres mount_res;
    umntres umount_res;
  } result;
  int ret;

  bool_t (*xdr_argument)();
  bool_t (*xdr_result)();
  int (*local)();

  current_transp = transp;

  switch (rqstp->rq_proc) {

  case AUTOFS_NULL:
    svc_sendreply(transp,
		  (XDRPROC_T_TYPE) xdr_void,
		  (SVC_IN_ARG_TYPE) NULL);
    return;

  case AUTOFS_MOUNT:
    xdr_argument = xdr_mntrequest;
    xdr_result = xdr_mntres;
    local = autofs_mount_1_req;
    break;

  case AUTOFS_UNMOUNT:
    xdr_argument = xdr_umntrequest;
    xdr_result = xdr_umntres;
    local = autofs_unmount_1_req;
    break;

  default:
    svcerr_noproc(transp);
    return;
  }

  memset((char *) &argument, 0, sizeof(argument));
  if (!svc_getargs(transp,
		   (XDRPROC_T_TYPE) xdr_argument,
		   (SVC_IN_ARG_TYPE) &argument)) {
    plog(XLOG_ERROR,
	 "AUTOFS xdr decode failed for %d %d %d",
	 (int) rqstp->rq_prog, (int) rqstp->rq_vers, (int) rqstp->rq_proc);
    svcerr_decode(transp);
    return;
  }

  memset((char *)&result, 0, sizeof (result));
  ret = (*local) (&argument, &result, rqstp);

  current_transp = NULL;

  /* send reply only if the RPC method returned 0 */
  if (!ret) {
    if (!svc_sendreply(transp,
		       (XDRPROC_T_TYPE) xdr_result,
		       (SVC_IN_ARG_TYPE) &result)) {
      svcerr_systemerr(transp);
    }
  }

  if (!svc_freeargs(transp,
		    (XDRPROC_T_TYPE) xdr_argument,
		    (SVC_IN_ARG_TYPE) &argument)) {
    plog(XLOG_FATAL, "unable to free rpc arguments in autofs_program_1");
  }
}


autofs_fh_t *
autofs_get_fh(am_node *mp)
{
  autofs_fh_t *fh;
  char buf[MAXHOSTNAMELEN];
  mntfs *mf = mp->am_mnt;
  struct utsname utsname;

  plog(XLOG_DEBUG, "autofs_get_fh for %s", mp->am_path);
  fh = ALLOC(autofs_fh_t);
  memset((voidp) fh, 0, sizeof(autofs_fh_t)); /* Paranoid */

  /*
   * SET MOUNT ARGS
   */
  if (uname(&utsname) < 0) {
    strcpy(buf, "localhost.autofs");
  } else {
    strcpy(buf, utsname.nodename);
    strcat(buf, ".autofs");
  }
#ifdef HAVE_AUTOFS_ARGS_T_ADDR
  fh->addr.buf = strdup(buf);
  fh->addr.len = fh->addr.maxlen = strlen(buf);
#endif /* HAVE_AUTOFS_ARGS_T_ADDR */

  fh->direct = (mf->mf_fsflags & FS_DIRECT) ? 1 : 0;
  fh->rpc_to = 1;		/* XXX: arbitrary */
  fh->mount_to = mp->am_timeo;
  fh->path = mp->am_path;
  fh->opts = "";		/* XXX: arbitrary */
  fh->map = mp->am_path;	/* this is what we get back in readdir */

  return fh;
}


void
autofs_mounted(am_node *mp)
{
  /* We don't want any timeouts on autofs nodes */
  mp->am_ttl = NEVER;
}


void
autofs_release_fh(autofs_fh_t *fh)
{
  if (fh) {
#ifdef HAVE_AUTOFS_ARGS_T_ADDR
    free(fh->addr.buf);
#endif /* HAVE_AUTOFS_ARGS_T_ADDR */
    XFREE(fh);
  }
}


void
autofs_add_fdset(fd_set *readfds)
{
  /* nothing to do */
}

int
autofs_handle_fdset(fd_set *readfds, int nsel)
{
  /* nothing to do */
  return nsel;
}


/*
 * Create the autofs service for amd
 */
int
create_autofs_service(void)
{
  dlog("creating autofs service listener");
  return register_autofs_service(AUTOFS_CONFTYPE, autofs_program_1);
}


int
destroy_autofs_service(void)
{
  dlog("destroying autofs service listener");
  return unregister_autofs_service(AUTOFS_CONFTYPE);
}


int
autofs_link_mount(am_node *mp)
{
  int err;
  mntfs *mf = mp->am_mnt;

  plog(XLOG_INFO, "autofs: converting from link to lofs (%s -> %s)", mp->am_path, mp->am_link);
  if ((err = mkdirs(mf->mf_real_mount, 0555)))
    goto out;
  err = lofs_ops.mount_fs(mp, mf);

out:
  if (err)
    return errno;
  return 0;
}


int
autofs_link_umount(am_node *mp)
{
  mntfs *mf = mp->am_mnt;
  return lofs_ops.umount_fs(mp, mf);
}


int
autofs_umount_succeeded(am_node *mp)
{
  umntres res;
  SVCXPRT *transp = mp->am_transp;

  if (transp) {
    res.status = 0;

    if (!svc_sendreply(transp,
		       (XDRPROC_T_TYPE) xdr_umntres,
		       (SVC_IN_ARG_TYPE) &res))
      svcerr_systemerr(transp);

    dlog("Quick reply sent for %s", mp->am_mnt->mf_mount);
    XFREE(transp);
    mp->am_transp = NULL;
  }

  plog(XLOG_INFO, "autofs: unmounting %s succeeded", mp->am_path);
  return 0;
}

int
autofs_umount_failed(am_node *mp)
{
  umntres res;
  SVCXPRT *transp = mp->am_transp;

  if (transp) {
    res.status = 1;

    if (!svc_sendreply(transp,
		       (XDRPROC_T_TYPE) xdr_umntres,
		       (SVC_IN_ARG_TYPE) &res))
      svcerr_systemerr(transp);

    dlog("Quick reply sent for %s", mp->am_mnt->mf_mount);
    XFREE(transp);
    mp->am_transp = NULL;
  }

  plog(XLOG_INFO, "autofs: unmounting %s failed", mp->am_path);
  return 0;
}


void
autofs_mount_succeeded(am_node *mp)
{
  SVCXPRT *transp = mp->am_transp;
  struct stat stb;

  if (transp) {
    /* this was a mount request */
    mntres res;
    res.status = 0;

    if (!svc_sendreply(transp,
		       (XDRPROC_T_TYPE) xdr_mntres,
		       (SVC_IN_ARG_TYPE) &res))
      svcerr_systemerr(transp);

    dlog("Quick reply sent for %s", mp->am_mnt->mf_mount);
    XFREE(transp);
    mp->am_transp = NULL;
  }

  /*
   * Store dev and rdev -- but not for symlinks
   */
  if (!mp->am_link) {
    if (!lstat(mp->am_mnt->mf_real_mount, &stb)) {
      mp->am_dev = stb.st_dev;
      mp->am_rdev = stb.st_rdev;
    }
    /* don't expire the entries -- the kernel will do it for us */
    mp->am_flags |= AMF_NOTIMEOUT;
  }

  plog(XLOG_INFO, "autofs: mounting %s succeeded", mp->am_path);
}


void
autofs_mount_failed(am_node *mp)
{
  SVCXPRT *transp = mp->am_transp;

  if (transp) {
    /* this was a mount request */
    mntres res;
    res.status = ENOENT;

    if (!svc_sendreply(transp,
		       (XDRPROC_T_TYPE) xdr_mntres,
		       (SVC_IN_ARG_TYPE) &res))
      svcerr_systemerr(transp);

    dlog("Quick reply sent for %s", mp->am_mnt->mf_mount);
    XFREE(transp);
    mp->am_transp = NULL;
  }

  plog(XLOG_INFO, "autofs: mounting %s failed", mp->am_path);
}


void
autofs_get_opts(char *opts, autofs_fh_t *fh)
{
  sprintf(opts, "%sdirect",
	  fh->direct ? "" : "in");
}


int
autofs_compute_mount_flags(mntent_t *mntp)
{
  /* Must use overlay mounts */
  return MNT2_GEN_OPT_OVERLAY;
}


void autofs_timeout_mp(am_node *mp)
{
  /* We don't want any timeouts on autofs nodes */
  mp->am_ttl = NEVER;
}
