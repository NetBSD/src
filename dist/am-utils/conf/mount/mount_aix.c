/*	$NetBSD: mount_aix.c,v 1.1.1.4 2001/05/13 17:50:17 veego Exp $	*/

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
 * Id: mount_aix.c,v 1.4.2.1 2001/01/10 03:23:16 ezk Exp
 *
 */

/*
 * AIX 3 Mount helper
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>

#define	VMT_ROUNDUP(len) (4 * ((len + 3) / 4))
#define VMT_ASSIGN(vp, idx, data, size) \
	vp->vmt_data[idx].vmt_off = p - (char *) vp; \
	vp->vmt_data[idx].vmt_size = size; \
	memmove(p, data, size); \
	p += VMT_ROUNDUP(size);

/* missing external definitions from AIX's headers */
extern int vmount(struct vmount *vmount, int size);


static int
aix3_mkvp(char *p, int gfstype, int flags, char *object, char *stub, char *host, char *info, int info_size, char *args)
{
  struct vmount *vp = (struct vmount *) p;

  memset((voidp) vp, 0, sizeof(*vp));
  /*
   * Fill in standard fields
   */
  vp->vmt_revision = VMT_REVISION;
  vp->vmt_flags = flags;
  vp->vmt_gfstype = gfstype;

  /*
   * Fill in all variable length data
   */
  p += sizeof(*vp);

  VMT_ASSIGN(vp, VMT_OBJECT, object, strlen(object) + 1);
  VMT_ASSIGN(vp, VMT_STUB, stub, strlen(stub) + 1);
  VMT_ASSIGN(vp, VMT_HOST, host, strlen(host) + 1);
  VMT_ASSIGN(vp, VMT_HOSTNAME, host, strlen(host) + 1);
  VMT_ASSIGN(vp, VMT_INFO, info, info_size);
  VMT_ASSIGN(vp, VMT_ARGS, args, strlen(args) + 1);

  /*
   * Return length
   */
  return vp->vmt_length = p - (char *) vp;
}


/*
 * Map from conventional mount arguments
 * to AIX 3-style arguments.
 */
int
mount_aix3(char *fsname, char *dir, int flags, int type, void *data, char *mnt_opts)
{
  char buf[4096];
  int size, ret;
  int real_size = sizeof(nfs_args_t); /* size passed to aix3_mkvp() */
  char *real_args = data;	/* args passed to aix3_mkvp() */
  char *host = strdup(fsname);
  char *rfs = strchr(host, ':');
  int free_rfs = 0;
#ifdef HAVE_FS_NFS3
  struct nfs_args v2args;
  nfs_args_t *v3args = (nfs_args_t *) data;
#endif /* HAVE_FS_NFS3 */

#ifdef DEBUG
  dlog("mount_aix3: fsname %s, dir %s, type %d", fsname, dir, type);
#endif /* DEBUG */

  switch (type) {

  case MOUNT_TYPE_NFS:

#ifdef HAVE_FS_NFS3
    /*
     * This is tricky.  If we have v3 support, but this nfs mount is v2,
     * then I must copy the arguments from the v3 nfs_args to the v2 one.
     */
    memmove((voidp) &v2args.addr, (voidp) &v3args->addr, sizeof(struct sockaddr_in));
    /*     v2args.u0 = v3args->u0; */
    /*     v2args.proto = v3args->proto; */
    v2args.hostname = v3args->hostname;
    v2args.netname = v3args->netname;
    memmove(v2args.fh.x, ((fhandle_t *)v3args->fh)->x, FHSIZE);
    v2args.flags = v3args->flags;
    v2args.wsize = v3args->wsize;
    v2args.rsize = v3args->rsize;
    v2args.timeo = v3args->timeo;
    v2args.retrans = v3args->retrans;
    v2args.acregmin = v3args->acregmin;
    v2args.acregmax = v3args->acregmax;
    v2args.acdirmin = v3args->acdirmin;
    v2args.acdirmax = v3args->acdirmax;
    /*     v2args.u14 = v3args->u14; */
    v2args.pathconf = v3args->pathconf;

    /* now set real_* stuff */
    real_size = sizeof(struct nfs_args);
    real_args = (char *) &v2args;

  case MOUNT_TYPE_NFS3:
    /* do nothing, because the nfs_args passed is already a v3 one */
    /* just fall through */
#endif /* HAVE_FS_NFS3 */

    if (rfs) {
      *rfs++ = '\0';
    } else {
      rfs = host;
      free_rfs = 1;
      host = strdup(am_get_hostname());
    }

    size = aix3_mkvp(buf, type, flags, rfs, dir, host,
		     real_args, real_size, mnt_opts);
    if (free_rfs)
      XFREE(rfs);
    XFREE(host);
  break;

  case MOUNT_TYPE_UFS:
    /* Need to open block device and extract log device info from sblk. */
    return EINVAL;

  default:
    return EINVAL;
  }

  ret = vmount((struct vmount *)buf, size);
  if (ret < 0)
    plog(XLOG_ERROR, "mount_aix3: vmount failed with errno %d", errno);
  return ret;
}
