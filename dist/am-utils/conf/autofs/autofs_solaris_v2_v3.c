/*	$NetBSD: autofs_solaris_v2_v3.c,v 1.1.1.2 2003/03/09 01:13:20 christos Exp $	*/

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
 * Id: autofs_solaris_v2_v3.c,v 1.27 2002/12/27 22:43:54 ezk Exp
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

struct amd_rddirres {
  enum autofs_res rd_status;
  u_long rd_bufsize;
  nfsdirlist rd_dl;
};
typedef struct amd_rddirres amd_rddirres;

/*
 * VARIABLES:
 */

SVCXPRT *autofs_xprt = NULL;

/* forward declarations */
bool_t xdr_umntrequest(XDR *xdrs, umntrequest *objp);
bool_t xdr_umntres(XDR *xdrs, umntres *objp);
bool_t xdr_autofs_lookupargs(XDR *xdrs, autofs_lookupargs *objp);
bool_t xdr_autofs_mountres(XDR *xdrs, autofs_mountres *objp);
bool_t xdr_autofs_lookupres(XDR *xdrs, autofs_lookupres *objp);
bool_t xdr_autofs_rddirargs(XDR *xdrs, autofs_rddirargs *objp);
static bool_t xdr_amd_rddirres(XDR *xdrs, amd_rddirres *objp);

/*
 * These exist only in the AutoFS V2 protocol.
 */
#ifdef AUTOFS_POSTUNMOUNT
bool_t xdr_postumntreq(XDR *xdrs, postumntreq *objp);
bool_t xdr_postumntres(XDR *xdrs, postumntres *objp);
bool_t xdr_postmountreq(XDR *xdrs, postmountreq *objp);
bool_t xdr_postmountres(XDR *xdrs, postmountres *objp);
#endif /* AUTOFS_POSTUMOUNT */

/*
 * AUTOFS XDR FUNCTIONS:
 */
bool_t
xdr_autofs_stat(XDR *xdrs, autofs_stat *objp)
{
  if (!xdr_enum(xdrs, (enum_t *)objp))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_autofs_action(XDR *xdrs, autofs_action *objp)
{
  if (!xdr_enum(xdrs, (enum_t *)objp))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_linka(XDR *xdrs, linka *objp)
{
  if (!xdr_string(xdrs, &objp->dir, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->link, AUTOFS_MAXPATHLEN))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_autofs_netbuf(XDR *xdrs, struct netbuf *objp)
{
  bool_t dummy;

  if (!xdr_u_long(xdrs, (u_long *) &objp->maxlen))
    return (FALSE);
  dummy = xdr_bytes(xdrs, (char **)&(objp->buf),
		    (u_int *)&(objp->len), objp->maxlen);
  return (dummy);
}

bool_t
xdr_autofs_args(XDR *xdrs, autofs_args *objp)
{
  if (!xdr_autofs_netbuf(xdrs, &objp->addr))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->path, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->opts, AUTOFS_MAXOPTSLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->map, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->subdir, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->key, AUTOFS_MAXCOMPONENTLEN))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->mount_to))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->rpc_to))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->direct))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_mounta(XDR *xdrs, struct mounta *objp)
{
  if (!xdr_string(xdrs, &objp->spec, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->dir, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->flags))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->fstype, AUTOFS_MAXCOMPONENTLEN))
    return (FALSE);
  if (!xdr_pointer(xdrs, (char **)&objp->dataptr, sizeof (autofs_args),
		   (XDRPROC_T_TYPE) xdr_autofs_args))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->datalen))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_action_list_entry(XDR *xdrs, action_list_entry *objp)
{
  if (!xdr_autofs_action(xdrs, &objp->action))
    return (FALSE);
  switch (objp->action) {
  case AUTOFS_MOUNT_RQ:
    if (!xdr_mounta(xdrs, &objp->action_list_entry_u.mounta))
      return (FALSE);
    break;
  case AUTOFS_LINK_RQ:
    if (!xdr_linka(xdrs, &objp->action_list_entry_u.linka))
      return (FALSE);
    break;
  default:
    break;
  }
  return (TRUE);
}

bool_t
xdr_action_list(XDR *xdrs, action_list *objp)
{
  if (!xdr_action_list_entry(xdrs, &objp->action))
    return (FALSE);
  if (!xdr_pointer(xdrs, (char **)&objp->next, sizeof (action_list),
		   (XDRPROC_T_TYPE) xdr_action_list))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_umntrequest(XDR *xdrs, umntrequest *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_umntrequest:");

  if (!xdr_bool_t(xdrs, &objp->isdirect))
    return (FALSE);
#ifdef HAVE_STRUCT_UMNTREQUEST_DEVID
  if (!xdr_dev_t(xdrs, &objp->devid))
    return (FALSE);
  if (!xdr_dev_t(xdrs, &objp->rdevid))
    return (FALSE);
#else  /* not HAVE_STRUCT_UMNTREQUEST_DEVID */
  if (!xdr_string(xdrs, &objp->mntresource, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->mntpnt, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->fstype, AUTOFS_MAXCOMPONENTLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->mntopts, AUTOFS_MAXOPTSLEN))
    return (FALSE);
#endif /* not HAVE_STRUCT_UMNTREQUEST_DEVID */
  if (!xdr_pointer(xdrs, (char **) &objp->next, sizeof(umntrequest),
		   (XDRPROC_T_TYPE) xdr_umntrequest))
    return (FALSE);

  return (TRUE);
}


bool_t
xdr_umntres(XDR *xdrs, umntres *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_mntres:");

  if (!xdr_int(xdrs, &objp->status))
    return (FALSE);
  return (TRUE);
}

/*
 * These exist only in the AutoFS V2 protocol.
 */
#ifdef AUTOFS_POSTUNMOUNT
bool_t
xdr_postumntreq(XDR *xdrs, postumntreq *objp)
{
  if (!xdr_dev_t(xdrs, &objp->devid))
    return (FALSE);
  if (!xdr_dev_t(xdrs, &objp->rdevid))
    return (FALSE);
  if (!xdr_pointer(xdrs, (char **)&objp->next,
		   sizeof (struct postumntreq),
		   (XDRPROC_T_TYPE) xdr_postumntreq))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_postumntres(XDR *xdrs, postumntres *objp)
{
  if (!xdr_int(xdrs, &objp->status))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_postmountreq(XDR *xdrs, postmountreq *objp)
{
  if (!xdr_string(xdrs, &objp->special, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->mountp, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->fstype, AUTOFS_MAXCOMPONENTLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->mntopts, AUTOFS_MAXOPTSLEN))
    return (FALSE);
  if (!xdr_dev_t(xdrs, &objp->devid))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_postmountres(XDR *xdrs, postmountres *objp)
{
  if (!xdr_int(xdrs, &objp->status))
    return (FALSE);
  return (TRUE);
}
#endif /* AUTOFS_POSTUNMOUNT */

bool_t
xdr_autofs_res(XDR *xdrs, autofs_res *objp)
{
  if (!xdr_enum(xdrs, (enum_t *)objp))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_autofs_lookupargs(XDR *xdrs, autofs_lookupargs *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_autofs_lookupargs:");

  if (!xdr_string(xdrs, &objp->map, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->path, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->name, AUTOFS_MAXCOMPONENTLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->subdir, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_string(xdrs, &objp->opts, AUTOFS_MAXOPTSLEN))
    return (FALSE);
  if (!xdr_bool_t(xdrs, &objp->isdirect))
    return (FALSE);
  return (TRUE);
}


bool_t
xdr_mount_result_type(XDR *xdrs, mount_result_type *objp)
{
  if (!xdr_autofs_stat(xdrs, &objp->status))
    return (FALSE);
  switch (objp->status) {
  case AUTOFS_ACTION:
    if (!xdr_pointer(xdrs,
		     (char **)&objp->mount_result_type_u.list,
		     sizeof (action_list), (XDRPROC_T_TYPE) xdr_action_list))
      return (FALSE);
    break;
  case AUTOFS_DONE:
    if (!xdr_int(xdrs, &objp->mount_result_type_u.error))
      return (FALSE);
    break;
  }
  return (TRUE);
}

bool_t
xdr_autofs_mountres(XDR *xdrs, autofs_mountres *objp)
{
  if (amuDebug(D_XDRTRACE))
    plog(XLOG_DEBUG, "xdr_mntres:");

  if (!xdr_mount_result_type(xdrs, &objp->mr_type))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->mr_verbose))
    return (FALSE);

  return (TRUE);
}

bool_t
xdr_lookup_result_type(XDR *xdrs, lookup_result_type *objp)
{
  if (!xdr_autofs_action(xdrs, &objp->action))
    return (FALSE);
  switch (objp->action) {
  case AUTOFS_LINK_RQ:
    if (!xdr_linka(xdrs, &objp->lookup_result_type_u.lt_linka))
      return (FALSE);
    break;
  default:
    break;
  }
  return (TRUE);
}

bool_t
xdr_autofs_lookupres(XDR *xdrs, autofs_lookupres *objp)
{
  if (!xdr_autofs_res(xdrs, &objp->lu_res))
    return (FALSE);
  if (!xdr_lookup_result_type(xdrs, &objp->lu_type))
    return (FALSE);
  if (!xdr_int(xdrs, &objp->lu_verbose))
    return (FALSE);
  return (TRUE);
}

bool_t
xdr_autofs_rddirargs(XDR *xdrs, autofs_rddirargs *objp)
{
  if (!xdr_string(xdrs, &objp->rda_map, AUTOFS_MAXPATHLEN))
    return (FALSE);
  if (!xdr_u_int(xdrs, (int *) &objp->rda_offset))
    return (FALSE);
  if (!xdr_u_int(xdrs, (int *) &objp->rda_count))
    return (FALSE);
  return (TRUE);
}


/*
 * ENCODE ONLY
 *
 * Solaris automountd uses struct autofsrddir to pass the results.
 * We use the traditional nfsreaddirres and do the conversion ourselves.
 */
static bool_t
xdr_amd_putrddirres(XDR *xdrs, nfsdirlist *dp, ulong reqsize)
{
  nfsentry *ep;
  char *name;
  u_int namlen;
  bool_t true = TRUE;
  bool_t false = FALSE;
  int entrysz;
  int tofit;
  int bufsize;
  u_long ino, off;

  bufsize = 1 * BYTES_PER_XDR_UNIT;
  for (ep = dp->dl_entries; ep; ep = ep->ne_nextentry) {
    name = ep->ne_name;
    namlen = strlen(name);
    ino = (u_long) ep->ne_fileid;
    off = (u_long) ep->ne_cookie + AUTOFS_DAEMONCOOKIE;
    entrysz = (1 + 1 + 1 + 1) * BYTES_PER_XDR_UNIT +
      roundup(namlen, BYTES_PER_XDR_UNIT);
    tofit = entrysz + 2 * BYTES_PER_XDR_UNIT;
    if (bufsize + tofit > reqsize) {
      dp->dl_eof = FALSE;
      break;
    }
    if (!xdr_bool(xdrs, &true) ||
	!xdr_u_long(xdrs, &ino) ||
	!xdr_bytes(xdrs, &name, &namlen, AUTOFS_MAXPATHLEN) ||
	!xdr_u_long(xdrs, &off)) {
      return (FALSE);
    }
    bufsize += entrysz;
  }
  if (!xdr_bool(xdrs, &false)) {
    return (FALSE);
  }
  if (!xdr_bool(xdrs, &dp->dl_eof)) {
    return (FALSE);
  }
  return (TRUE);
}


static bool_t
xdr_amd_rddirres(XDR *xdrs, amd_rddirres *objp)
{
  if (!xdr_enum(xdrs, (enum_t *)&objp->rd_status))
    return (FALSE);
  if (objp->rd_status != AUTOFS_OK)
    return (TRUE);
  return (xdr_amd_putrddirres(xdrs, &objp->rd_dl, objp->rd_bufsize));
}


/*
 * AUTOFS RPC methods
 */

static int
autofs_lookup_2_req(autofs_lookupargs *m,
		    autofs_lookupres *res,
		    struct authunix_parms *cred)
{
  int err;
  am_node *mp, *ap;
  mntfs *mf, **temp_mf;

  dlog("LOOKUP REQUEST: name=%s[%s] map=%s opts=%s path=%s direct=%d\n",
       m->name, m->subdir, m->map, m->opts,
       m->path, m->isdirect);

  /* find the effective uid/gid from RPC request */
  sprintf(opt_uid, "%d", (int) cred->aup_uid);
  sprintf(opt_gid, "%d", (int) cred->aup_gid);

  mp = find_ap(m->path);
  if (!mp) {
    plog(XLOG_ERROR, "map %s not found", m->path);
    err = AUTOFS_NOENT;
    goto out;
  }

  mf = mp->am_mnt;
  ap = mf->mf_ops->lookup_child(mp, m->name, &err, VLOOK_CREATE);
  if (!ap) {
    if (err > 0) {
      err = AUTOFS_NOENT;
      goto out;
    }
    /* we're working on it */
    amd_stats.d_drops++;
    return 1;
  }

  if (err == 0) {
    plog(XLOG_ERROR, "autofs requests to mount an already mounted node???");
    err = AUTOFS_OK;
    if (ap->am_link) {
      res->lu_type.action = AUTOFS_LINK_RQ;
      res->lu_type.lookup_result_type_u.lt_linka.dir = strdup(ap->am_name);
      res->lu_type.lookup_result_type_u.lt_linka.link = strdup(ap->am_link);
    } else
      res->lu_type.action = AUTOFS_NONE;

    if (ap->am_mfarray) {
      mntfs **temp_mf;
      for (temp_mf = ap->am_mfarray; *temp_mf; temp_mf++)
	free_mntfs(*temp_mf);
      XFREE(ap->am_mfarray);
      ap->am_mfarray = 0;
    }
    goto out;
  }

  err = AUTOFS_OK;
  for (temp_mf = ap->am_mfarray; *temp_mf; temp_mf++)
    if (!(*temp_mf)->mf_fo->opt_sublink) {
      free_mntfs(*temp_mf);
      *temp_mf = 0;
    } else if (temp_mf != ap->am_mfarray) {
      ap->am_mfarray[0] = *temp_mf;
      *temp_mf = 0;
    }

  if (!ap->am_mfarray[0]) {
    res->lu_type.action = AUTOFS_NONE;
    free_map(ap);
    goto out;
  }

  ap = mf->mf_ops->mount_child(ap, &err);
  if (!ap) {
    if (err > 0) {
      err = AUTOFS_NOENT;
      goto out;
    }
    /* we're working on it */
    amd_stats.d_drops++;
    return 1;
  }
  if (err == 0) {
    res->lu_type.action = AUTOFS_LINK_RQ;
    res->lu_type.lookup_result_type_u.lt_linka.dir = strdup(ap->am_name);
    res->lu_type.lookup_result_type_u.lt_linka.link = strdup(ap->am_link);
  } else {
    res->lu_type.action = AUTOFS_NONE;
    err = AUTOFS_OK;
  }
out:
  res->lu_res = err;
  res->lu_verbose = 1;

  dlog("LOOKUP REPLY: status=%d\n", res->lu_res);
  return 0;
}


static void
autofs_lookup_2_free(autofs_lookupres *res)
{
  struct linka link;

  if ((res->lu_res == AUTOFS_OK) &&
      (res->lu_type.action == AUTOFS_LINK_RQ)) {
    /*
     * Free link information
     */
    link = res->lu_type.lookup_result_type_u.lt_linka;
    if (link.dir)
      XFREE(link.dir);
    if (link.link)
      XFREE(link.link);
  }
}


static int
autofs_mount_2_req(autofs_lookupargs *m,
		   autofs_mountres *res,
		   struct authunix_parms *cred)
{
  int err = AUTOFS_OK;
  am_node *mp, *ap;
  mntfs *mf;

  dlog("MOUNT REQUEST: name=%s[%s] map=%s opts=%s path=%s direct=%d\n",
       m->name, m->subdir, m->map, m->opts,
       m->path, m->isdirect);

  /* find the effective uid/gid from RPC request */
  sprintf(opt_uid, "%d", (int) cred->aup_uid);
  sprintf(opt_gid, "%d", (int) cred->aup_gid);

  mp = find_ap(m->path);
  if (!mp) {
    plog(XLOG_ERROR, "map %s not found", m->path);
    res->mr_type.status = AUTOFS_DONE;
    res->mr_type.mount_result_type_u.error = AUTOFS_NOENT;
    goto out;
  }

  mf = mp->am_mnt;
  ap = mf->mf_ops->lookup_child(mp, m->name + m->isdirect, &err, VLOOK_CREATE);
  if (ap && err < 0)
    ap = mf->mf_ops->mount_child(ap, &err);
  if (ap == NULL) {
    if (err < 0) {
      /* we're working on it */
      amd_stats.d_drops++;
      return 1;
    }
    res->mr_type.status = AUTOFS_DONE;
    res->mr_type.mount_result_type_u.error = AUTOFS_NOENT;
    goto out;
  }

out:
  res->mr_verbose = 1;

  switch (res->mr_type.status) {
  case AUTOFS_ACTION:
    dlog("MOUNT REPLY: status=%d, AUTOFS_ACTION\n",
	 err);
    break;
  case AUTOFS_DONE:
    dlog("MOUNT REPLY: status=%d, AUTOFS_DONE\n",
	 err);
    break;
  default:
    dlog("MOUNT REPLY: status=%d, UNKNOWN\n",
	 err);
  }

  if (err) {
    if (m->isdirect) {
      /* direct mount */
      plog(XLOG_ERROR, "mount of %s failed", m->path);
    } else {
      /* indirect mount */
      plog(XLOG_ERROR, "mount of %s/%s failed", m->path, m->name);
    }
  }
  return 0;
}


/* XXX not implemented */
static void
autofs_mount_2_free(struct autofs_mountres *res)
{
  if (res->mr_type.status == AUTOFS_ACTION) {
    dlog("freeing action list\n");
    /*     free_action_list(res->mr_type.mount_result_type_u.list); */
  }
}


static int
autofs_unmount_2_req(umntrequest *ul,
		     umntres *res,
		     struct authunix_parms *cred)
{
  int mapno, err;
  am_node *mp = NULL;

#ifdef HAVE_STRUCT_UMNTREQUEST_DEVID
  dlog("UNMOUNT REQUEST: dev=%lx rdev=%lx %s\n",
       (u_long) ul->devid,
       (u_long) ul->rdevid,
       ul->isdirect ? "direct" : "indirect");
#else  /* not HAVE_STRUCT_UMNTREQUEST_DEVID */
  dlog("UNMOUNT REQUEST: mntresource='%s' mntpnt='%s' fstype='%s' mntopts='%s' %s\n",
       ul->mntresource,
       ul->mntpnt,
       ul->fstype,
       ul->mntopts,
       ul->isdirect ? "direct" : "indirect");
#endif /* not HAVE_STRUCT_UMNTREQUEST_DEVID */

  /* by default, and if not found, succeed */
  res->status = 0;

#ifdef HAVE_STRUCT_UMNTREQUEST_DEVID
  for (mapno = 0; mapno <= last_used_map; mapno++) {
    mp = exported_ap[mapno];
    if (mp &&
	mp->am_dev == ul->devid &&
	mp->am_rdev == ul->rdevid)
      break;
    mp = NULL;
  }
#else  /* not HAVE_STRUCT_UMNTREQUEST_DEVID */
  mp = find_ap(ul->mntpnt);
#endif /* not HAVE_STRUCT_UMNTREQUEST_DEVID */

  if (mp) {
    /* save RPC context */
    if (!mp->am_transp && current_transp) {
      mp->am_transp = (SVCXPRT *) xmalloc(sizeof(SVCXPRT));
      *(mp->am_transp) = *current_transp;
    }

    mapno = mp->am_mapno;
    err = unmount_mp(mp);

    if (err)
      /* backgrounded, don't reply yet */
      return 1;

    if (exported_ap[mapno])
      /* unmounting failed, tell the kernel */
      res->status = 1;
  }

  dlog("UNMOUNT REPLY: status=%d\n", res->status);
  return 0;
}


/*
 * These exist only in the AutoFS V2 protocol.
 */
#ifdef AUTOFS_POSTUNMOUNT
/* XXX not implemented */
static int
autofs_postunmount_2_req(postumntreq *req,
			 postumntres *res,
			 struct authunix_parms *cred,
			 SVCXPRT *transp)
{
  postumntreq *ul = req;

  dlog("POSTUNMOUNT REQUEST: dev=%lx rdev=%lx\n",
       (u_long) ul->devid,
       (u_long) ul->rdevid);

  /* succeed unconditionally */
  res->status = 0;

  dlog("POSTUNMOUNT REPLY: status=%d\n", res->status);
  return 0;
}


/* XXX not implemented */
static int
autofs_postmount_2_req(postmountreq *req,
		       postmountres *res,
		       struct authunix_parms *cred)
{
  dlog("POSTMOUNT REQUEST: %s\tdev=%lx\tspecial=%s %s\n",
       req->mountp, (u_long) req->devid, req->special, req->mntopts);

  /* succeed unconditionally */
  res->status = 0;

  dlog("POSTMOUNT REPLY: status=%d\n", res->status);
  return 0;
}
#endif /* AUTOFS_POSTUNMOUNT */


static int
autofs_readdir_2_req(struct autofs_rddirargs *req,
		     struct amd_rddirres *res,
		     struct authunix_parms *cred)
{
  am_node *mp;
  int err;
  static nfsentry e_res[MAX_READDIR_ENTRIES];

  dlog("READDIR REQUEST: %s @ %d\n",
       req->rda_map, (int) req->rda_offset);

  mp = find_ap(req->rda_map);
  if (!mp) {
    plog(XLOG_ERROR, "map %s not found", req->rda_map);
    res->rd_status = AUTOFS_NOENT;
    goto out;
  }

  mp->am_stats.s_readdir++;
  req->rda_offset -= AUTOFS_DAEMONCOOKIE;
  err = mp->am_mnt->mf_ops->readdir(mp, (char *)&req->rda_offset,
				    &res->rd_dl, e_res, req->rda_count);
  if (err) {
    res->rd_status = AUTOFS_ECOMM;
    goto out;
  }

  res->rd_status = AUTOFS_OK;
  res->rd_bufsize = req->rda_count;

out:
  dlog("READDIR REPLY: status=%d\n", res->rd_status);
  return 0;
}


/****************************************************************************/
/* autofs program dispatcher */
static void
autofs_program_2(struct svc_req *rqstp, SVCXPRT *transp)
{
  union {
    autofs_lookupargs autofs_mount_2_arg;
    autofs_lookupargs autofs_lookup_2_arg;
    umntrequest autofs_umount_2_arg;
    autofs_rddirargs autofs_readdir_2_arg;
#ifdef AUTOFS_POSTUNMOUNT
    postmountreq autofs_postmount_2_arg;
    postumntreq autofs_postumnt_2_arg;
#endif /* AUTOFS_POSTUNMOUNT */
  } argument;

  union {
    autofs_mountres mount_res;
    autofs_lookupres lookup_res;
    umntres umount_res;
    amd_rddirres readdir_res;
#ifdef AUTOFS_POSTUNMOUNT
    postumntres postumnt_res;
    postmountres postmnt_res;
#endif /* AUTOFS_POSTUNMOUNT */
  } result;
  int ret;

  bool_t (*xdr_argument)();
  bool_t (*xdr_result)();
  int (*local)();
  void (*local_free)() = NULL;

  current_transp = transp;

  switch (rqstp->rq_proc) {

  case AUTOFS_NULL:
    svc_sendreply(transp,
		  (XDRPROC_T_TYPE) xdr_void,
		  (SVC_IN_ARG_TYPE) NULL);
    return;

  case AUTOFS_LOOKUP:
    xdr_argument = xdr_autofs_lookupargs;
    xdr_result = xdr_autofs_lookupres;
    local = autofs_lookup_2_req;
    local_free = autofs_lookup_2_free;
    break;

  case AUTOFS_MOUNT:
    xdr_argument = xdr_autofs_lookupargs;
    xdr_result = xdr_autofs_mountres;
    local = autofs_mount_2_req;
    local_free = autofs_mount_2_free;
    break;

  case AUTOFS_UNMOUNT:
    xdr_argument = xdr_umntrequest;
    xdr_result = xdr_umntres;
    local = autofs_unmount_2_req;
    break;

/*
 * These exist only in the AutoFS V2 protocol.
 */
#ifdef AUTOFS_POSTUNMOUNT
  case AUTOFS_POSTUNMOUNT:
    xdr_argument = xdr_postumntreq;
    xdr_result = xdr_postumntres;
    local = autofs_postunmount_2_req;
    break;

  case AUTOFS_POSTMOUNT:
    xdr_argument = xdr_postmountreq;
    xdr_result = xdr_postmountres;
    local = autofs_postmount_2_req;
    break;
#endif /* AUTOFS_POSTUNMOUNT */

  case AUTOFS_READDIR:
    xdr_argument = xdr_autofs_rddirargs;
    xdr_result = xdr_amd_rddirres;
    local = autofs_readdir_2_req;
    break;

  default:
    svcerr_noproc(transp);
    return;
  }

  memset((char *) &argument, 0, sizeof(argument));
  if (!svc_getargs(transp,
		   (XDRPROC_T_TYPE) xdr_argument,
		   (SVC_IN_ARG_TYPE) &argument)) {
    plog(XLOG_ERROR, "AUTOFS xdr decode failed for %d %d %d",
	 (int) rqstp->rq_prog, (int) rqstp->rq_vers, (int) rqstp->rq_proc);
    svcerr_decode(transp);
    return;
  }

  memset((char *)&result, 0, sizeof (result));
  ret = (*local) (&argument, &result, rqstp->rq_clntcred, transp);

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
    plog(XLOG_FATAL, "unable to free rpc arguments in autofs_program_2");
  }

  if (local_free)
    (*local_free)(&result);
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

  fh->direct = ((mf->mf_ops->autofs_fs_flags & FS_DIRECT) == FS_DIRECT);
  fh->rpc_to = 1;		/* XXX: arbitrary */
  fh->mount_to = mp->am_timeo;
  fh->path = mp->am_path;
  fh->opts = "";		/* XXX: arbitrary */
  fh->map = mp->am_path;	/* this is what we get back in readdir */
  fh->subdir = "";
  if (fh->direct)
    fh->key = mp->am_name;
  else
    fh->key = "";

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
  return register_autofs_service(AUTOFS_CONFTYPE, autofs_program_2);
}


int
destroy_autofs_service(void)
{
  dlog("destroying autofs service listener");
  return unregister_autofs_service(AUTOFS_CONFTYPE);
}


static void
autofs_free_data(autofs_data_t *data)
{
  if (data->next)
    autofs_free_data(data->next);
  XFREE(data);
}


int
autofs_link_mount(am_node *mp)
{
  return 0;
}


int
autofs_link_umount(am_node *mp)
{
  return 0;
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

  /*
   * Store dev and rdev -- but not for symlinks
   */
  if (!mp->am_link) {
    if (!lstat(mp->am_path, &stb)) {
      mp->am_dev = stb.st_dev;
      mp->am_rdev = stb.st_rdev;
    }
    /* don't expire the entries -- the kernel will do it for us */
    mp->am_flags |= AMF_NOTIMEOUT;
  }

  if (transp) {
    if (mp->am_link) {
      /* this was a lookup request */
      autofs_lookupres res;
      res.lu_type.action = AUTOFS_LINK_RQ;
      res.lu_type.lookup_result_type_u.lt_linka.dir = strdup(mp->am_name);
      res.lu_type.lookup_result_type_u.lt_linka.link = strdup(mp->am_link);
      res.lu_res = AUTOFS_OK;
      res.lu_verbose = 1;

      if (!svc_sendreply(transp,
			 (XDRPROC_T_TYPE) xdr_autofs_lookupres,
			 (SVC_IN_ARG_TYPE) &res))
	svcerr_systemerr(transp);
    } else {
      /* this was a mount request */
      autofs_mountres res;
      res.mr_type.status = AUTOFS_DONE;
      res.mr_type.mount_result_type_u.error = AUTOFS_OK;
      res.mr_verbose = 1;

      if (!svc_sendreply(transp,
			 (XDRPROC_T_TYPE) xdr_autofs_mountres,
			 (SVC_IN_ARG_TYPE) &res))
	svcerr_systemerr(transp);
    }

    dlog("Quick reply sent for %s", mp->am_mnt->mf_mount);
    XFREE(transp);
    mp->am_transp = NULL;
  }

  plog(XLOG_INFO, "autofs: mounting %s succeeded", mp->am_path);
}


void
autofs_mount_failed(am_node *mp)
{
  SVCXPRT *transp = mp->am_transp;

  if (transp) {
    if (mp->am_link) {
      /* this was a lookup request */
      autofs_lookupres res;
      res.lu_res = AUTOFS_NOENT;
      res.lu_verbose = 1;
      if (!svc_sendreply(transp,
			 (XDRPROC_T_TYPE) xdr_autofs_lookupres,
			 (SVC_IN_ARG_TYPE) &res))
	svcerr_systemerr(transp);
    } else {
      /* this was a mount request */
      autofs_mountres res;
      res.mr_type.status = AUTOFS_DONE;
      res.mr_type.mount_result_type_u.error = AUTOFS_NOENT;
      res.mr_verbose = 1;

      if (!svc_sendreply(transp,
			 (XDRPROC_T_TYPE) xdr_autofs_mountres,
			 (SVC_IN_ARG_TYPE) &res))
	svcerr_systemerr(transp);
    }

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
