/*	$NetBSD: autofs_linux.c,v 1.1.1.2 2003/03/09 01:13:19 christos Exp $	*/

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
 * Id: autofs_linux.c,v 1.30 2002/12/27 22:43:54 ezk Exp
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

#define AUTOFS_MIN_VERSION 3
#define AUTOFS_MAX_VERSION 4


/*
 * STRUCTURES:
 */

/*
 * VARIABLES:
 */

static int autofs_max_fds;
static am_node **hash;
static int *list;
static int numfds = 0;
static int bind_works = 1;


static void hash_init(void)
{
  int i;
  struct rlimit rlim;

  if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
    plog(XLOG_ERROR, "getrlimit failed, defaulting to 256 fd's");
    autofs_max_fds = 256;
  } else {
    autofs_max_fds = (rlim.rlim_cur > 1024) ? 1024 : rlim.rlim_cur;
    plog(XLOG_INFO, "%d fd's available for autofs", autofs_max_fds);
  }

  list = malloc(autofs_max_fds * sizeof(*list));
  hash = malloc(autofs_max_fds * sizeof(*hash));

  for (i = 0 ; i < autofs_max_fds; i++)
    hash[i] = 0, list[i] = -1;
}


static void hash_insert(int fd, am_node *mp)
{
  if (hash[fd] != 0)
    plog(XLOG_ERROR, "file descriptor %d already in the hash", fd);

  hash[fd] = mp;
  list[numfds] = fd;
  numfds++;
}


static void hash_delete(int fd)
{
  int i;

  if (hash[fd] == 0)
    plog(XLOG_WARNING, "file descriptor %d not in the hash", fd);

  hash[fd] = 0;
  numfds--;
  for (i = 0; i < numfds; i++)
    if (list[i] == fd) {
      list[i] = list[numfds];
      break;
    }
}


autofs_fh_t *
autofs_get_fh(am_node *mp)
{
  int fds[2];
  autofs_fh_t *fh;

  plog(XLOG_DEBUG, "autofs_get_fh for %s", mp->am_path);
  if (pipe(fds) < 0)
    return 0;

  /* sanity check */
  if (fds[0] > autofs_max_fds)
    return 0;

  fh = ALLOC(autofs_fh_t);
  fh->fd = fds[0];
  fh->kernelfd = fds[1];
  fh->ioctlfd = -1;
  fh->pending_mounts = NULL;
  fh->pending_umounts = NULL;

  hash_insert(fh->fd, mp);

  return fh;
}


void
autofs_mounted(am_node *mp)
{
  mntfs *mf = mp->am_mnt;
  autofs_fh_t *fh = mf->mf_autofs_fh;
  unsigned long timeout = gopt.am_timeo;

  close(fh->kernelfd);
  fh->kernelfd = -1;

  fh->ioctlfd = open(mf->mf_mount, O_RDONLY);
  /* Get autofs protocol version */
  if (ioctl(fh->ioctlfd, AUTOFS_IOC_PROTOVER, &fh->version) < 0) {
    plog(XLOG_ERROR, "AUTOFS_IOC_PROTOVER: %s", strerror(errno));
    fh->version = AUTOFS_MIN_VERSION;
    plog(XLOG_ERROR, "autofs: assuming protocol version %d", fh->version);
  } else
    plog(XLOG_INFO, "autofs: using protocol version %d", fh->version);

  if (fh->version < 4) {
    /* no support for subdirs */
    plog(XLOG_INFO, "Turning off autofs support for host filesystems");
    amfs_host_ops.nfs_fs_flags &= ~FS_AUTOFS;
    amfs_host_ops.autofs_fs_flags &= ~FS_AUTOFS;
  }

  /* set expiration timeout */
  if (ioctl(fh->ioctlfd, AUTOFS_IOC_SETTIMEOUT, &timeout) < 0)
    plog(XLOG_ERROR, "AUTOFS_IOC_SETTIMEOUT: %s", strerror(errno));

  /* tell the daemon to call us for expirations */
  mp->am_ttl = clocktime() + gopt.am_timeo_w;
}


void
autofs_release_fh(autofs_fh_t *fh)
{
  if (fh) {
    hash_delete(fh->fd);
    /*
     * if a mount succeeded, the kernel fd was closed on
     * the amd side, so it might have been reused.
     * we set it to -1 after closing it, to avoid the problem.
     */
    if (fh->kernelfd >= 0)
      close(fh->kernelfd);

    if (fh->ioctlfd >= 0) {
      /*
       * Tell the kernel we're catatonic
       */
      ioctl(fh->ioctlfd, AUTOFS_IOC_CATATONIC, 0);
      close(fh->ioctlfd);
    }

    if (fh->fd >= 0)
      close(fh->fd);

    XFREE(fh);
  }
}


void
autofs_add_fdset(fd_set *readfds)
{
  int i;
  for (i = 0; i < numfds; i++)
    FD_SET(list[i], readfds);
}


static int
autofs_get_pkt(int fd, char *buf, int bytes)
{
  int i;

  do {
    i = read(fd, buf, bytes);

    if (i <= 0)
      break;

    buf += i;
    bytes -= i;
  } while (bytes);

  return bytes;
}


static void
send_fail(int fd, unsigned long token)
{
  if (token == 0)
    return;
  if (ioctl(fd, AUTOFS_IOC_FAIL, token) < 0)
    plog(XLOG_ERROR, "AUTOFS_IOC_FAIL: %s", strerror(errno));
}


static void
send_ready(int fd, unsigned long token)
{
  if (token == 0)
    return;
  if (ioctl(fd, AUTOFS_IOC_READY, token) < 0)
    plog(XLOG_ERROR, "AUTOFS_IOC_READY: %s", strerror(errno));
}


static void
autofs_lookup_failed(am_node *mp, char *name)
{
  autofs_fh_t *fh = mp->am_mnt->mf_autofs_fh;
  struct autofs_pending_mount **pp, *p;

  pp = &fh->pending_mounts;
  while (*pp && !STREQ((*pp)->name, name))
    pp = &(*pp)->next;

  /* sanity check */
  if (*pp == NULL)
    return;

  p = *pp;
  plog(XLOG_INFO, "autofs: lookup of %s failed", name);
  send_fail(fh->ioctlfd, p->wait_queue_token);

  XFREE(p->name);
  *pp = p->next;
  XFREE(p);
}


static void
autofs_expire_one(am_node *mp, char *name, unsigned long token)
{
  autofs_fh_t *fh;
  am_node *ap;
  struct autofs_pending_umount *p;
  char *ap_path;

  fh = mp->am_mnt->mf_autofs_fh;

  ap_path = str3cat(NULL, mp->am_path, "/", name);
  if (amuDebug(D_TRACE))
    plog(XLOG_DEBUG, "\tumount(%s)", ap_path);

  p = fh->pending_umounts;
  while (p && p->wait_queue_token != token)
    p = p->next;

  if (p) {
    /* already pending */
    dlog("Umounting of %s already pending", ap_path);
    amd_stats.d_drops++;
    goto out;
  }

  ap = find_ap(ap_path);
  if (ap == NULL) {
    /* not found??? not sure what to do here... */
    send_fail(fh->ioctlfd, token);
    goto out;
  }

  p = ALLOC(struct autofs_pending_umount);
  p->wait_queue_token = token;
  p->name = strdup(name);
  p->next = fh->pending_umounts;
  fh->pending_umounts = p;

  unmount_mp(ap);

out:
  XFREE(ap_path);
}


static void
autofs_handle_expire(am_node *mp, struct autofs_packet_expire *pkt)
{
  autofs_expire_one(mp, pkt->name, 0);
}


#ifdef autofs_ptype_expire_multi
static void
autofs_handle_expire_multi(am_node *mp, struct autofs_packet_expire_multi *pkt)
{
  autofs_expire_one(mp, pkt->name, pkt->wait_queue_token);
}
#endif /* autofs_packet_expire_multi */


static void
autofs_handle_missing(am_node *mp, struct autofs_packet_missing *pkt)
{
  autofs_fh_t *fh;
  mntfs *mf;
  am_node *ap;
  struct autofs_pending_mount *p;
  int error;

  mf = mp->am_mnt;
  fh = mf->mf_autofs_fh;

  p = fh->pending_mounts;
  while (p && p->wait_queue_token != pkt->wait_queue_token)
    p = p->next;

  if (p) {
    /* already pending */
    dlog("Mounting of %s/%s already pending",
	 mp->am_path, pkt->name);
    amd_stats.d_drops++;
    return;
  }

  p = ALLOC(struct autofs_pending_mount);
  p->wait_queue_token = pkt->wait_queue_token;
  p->name = strdup(pkt->name);
  p->next = fh->pending_mounts;
  fh->pending_mounts = p;

  if (amuDebug(D_TRACE))
    plog(XLOG_DEBUG, "\tlookup(%s, %s)", mp->am_path, pkt->name);
  ap = mf->mf_ops->lookup_child(mp, pkt->name, &error, VLOOK_CREATE);
  if (ap && error < 0)
    ap = mf->mf_ops->mount_child(ap, &error);

  /* some of the rest can be done in amfs_auto_cont */

  if (ap == 0) {
    if (error < 0) {
      dlog("Mount still pending, not sending autofs reply yet");
      return;
    }
    autofs_lookup_failed(mp, pkt->name);
  }
  mp->am_stats.s_lookup++;
}


int
autofs_handle_fdset(fd_set *readfds, int nsel)
{
  int i;
  union autofs_packet_union pkt;
  autofs_fh_t *fh;
  am_node *mp;

  for (i = 0; nsel && i < numfds; i++) {
    if (!FD_ISSET(list[i], readfds))
      continue;

    nsel--;
    FD_CLR(list[i], readfds);
    mp = hash[list[i]];
    fh = mp->am_mnt->mf_autofs_fh;

    if (autofs_get_pkt(fh->fd, (char *) &pkt,
		       sizeof(union autofs_packet_union)))
      continue;

    switch (pkt.hdr.type) {
    case autofs_ptype_missing:
      autofs_handle_missing(mp, &pkt.missing);
      break;
    case autofs_ptype_expire:
      autofs_handle_expire(mp, &pkt.expire);
      break;
#ifdef autofs_ptype_expire_multi
    case autofs_ptype_expire_multi:
      autofs_handle_expire_multi(mp, &pkt.expire_multi);
#endif /* autofs_handle_expire_multi */
    default:
      plog(XLOG_ERROR, "Unknown autofs packet type %d",
	   pkt.hdr.type);
    }
  }
  return nsel;
}


int
create_autofs_service(void)
{
  hash_init();

  /* not the best place, but... */
  if (linux_version_code() < KERNEL_VERSION(2,4,0))
    bind_works = 0;

  return 0;
}


int
destroy_autofs_service(void)
{
  /* Nothing to do */
  return 0;
}


int
autofs_link_mount(am_node *mp)
{
  int err = -1;

#ifdef MNT2_GEN_OPT_BIND
  if (bind_works) {
    mntent_t mnt;
    struct stat buf;

    /*
     * we need to stat() the destination, because the bind mount does not
     * follow symlinks and/or allow for non-existent destinations.
     * we fall back to symlinks if there are problems.
     *
     * we need to temporarily change pgrp, otherwise our stat() won't
     * trigger whatever cascading mounts are needed.
     *
     * WARNING: we will deadlock if this function is called from the master
     * amd process and it happens to trigger another auto mount. Therefore,
     * this function should be called only from a child amd process, or
     * at the very least it should not be called from the parent unless we
     * know for sure that it won't cause a recursive mount. We refuse to
     * cause the recursive mount anyway if called from the parent amd.
     */
    if (!foreground) {
      pid_t pgrp = getpgrp();
      setpgrp();
      err = stat(mp->am_link, &buf);
      if (setpgid(0, pgrp)) {
	plog(XLOG_ERROR, "autofs: cannot restore pgrp: %s", strerror(errno));
	plog(XLOG_ERROR, "autofs: aborting the mount");
	return errno;
      }
      if (err)
	goto use_symlink;
    }
    if ((err = lstat(mp->am_link, &buf)))
      goto use_symlink;
    if (S_ISLNK(buf.st_mode))
      goto use_symlink;
    plog(XLOG_INFO, "autofs: bind-mounting %s -> %s", mp->am_path, mp->am_link);
    memset(&mnt, 0, sizeof(mnt));
    mnt.mnt_dir = mp->am_path;
    mnt.mnt_fsname = mp->am_link;
    mnt.mnt_type = "bind";
    mnt.mnt_opts = "";
    mkdirs(mp->am_path, 0555);
    err = mount_fs(&mnt, MNT2_GEN_OPT_BIND, NULL, 0, "bind", 0, NULL, mnttab_file_name);
    if (err)
      rmdir(mp->am_path);
  }
use_symlink:
#endif /* MNT2_GEN_OPT_BIND */
  if (err) {
    plog(XLOG_INFO, "autofs: symlinking %s -> %s", mp->am_path, mp->am_link);
    err = symlink(mp->am_link, mp->am_path);
  }
  if (err)
    return errno;
  return 0;
}


int
autofs_link_umount(am_node *mp)
{
  struct stat buf;
  int err;

  if ((err = lstat(mp->am_path, &buf)))
    return errno;

  if (S_ISDIR(buf.st_mode)) {
    plog(XLOG_INFO, "autofs: un-bind-mounting %s", mp->am_path);
    err = umount_fs(mp->am_path, mnttab_file_name);
    if (err)
      plog(XLOG_INFO, "autofs: unmounting %s failed: %m", mp->am_path);
    else
      rmdir(mp->am_path);
  } else {
    plog(XLOG_INFO, "autofs: deleting symlink %s", mp->am_path);
    err = unlink(mp->am_path);
  }
  if (err)
    return errno;
  return err;
}


int
autofs_umount_succeeded(am_node *mp)
{
  autofs_fh_t *fh = mp->am_parent->am_mnt->mf_autofs_fh;
  struct autofs_pending_umount **pp, *p;

  pp = &fh->pending_umounts;
  while (*pp && !STREQ((*pp)->name, mp->am_name))
    pp = &(*pp)->next;

  /* sanity check */
  if (*pp == NULL)
    return -1;

  p = *pp;
  plog(XLOG_INFO, "autofs: unmounting %s succeeded", mp->am_path);
  send_ready(fh->ioctlfd, p->wait_queue_token);

  XFREE(p->name);
  *pp = p->next;
  XFREE(p);
  return 0;
}


int
autofs_umount_failed(am_node *mp)
{
  autofs_fh_t *fh = mp->am_parent->am_mnt->mf_autofs_fh;
  struct autofs_pending_umount **pp, *p;

  pp = &fh->pending_umounts;
  while (*pp && !STREQ((*pp)->name, mp->am_name))
    pp = &(*pp)->next;

  /* sanity check */
  if (*pp == NULL)
    return -1;

  p = *pp;
  plog(XLOG_INFO, "autofs: unmounting %s failed", mp->am_path);
  send_fail(fh->ioctlfd, p->wait_queue_token);

  XFREE(p->name);
  *pp = p->next;
  XFREE(p);
  return 0;
}


void
autofs_mount_succeeded(am_node *mp)
{
  autofs_fh_t *fh = mp->am_parent->am_mnt->mf_autofs_fh;
  struct autofs_pending_mount **pp, *p;

  /* don't expire the entries -- the kernel will do it for us */
  mp->am_flags |= AMF_NOTIMEOUT;

  pp = &fh->pending_mounts;
  while (*pp && !STREQ((*pp)->name, mp->am_name))
    pp = &(*pp)->next;

  /* sanity check */
  if (*pp == NULL)
    return;

  p = *pp;
  plog(XLOG_INFO, "autofs: mounting %s succeeded", mp->am_path);
  send_ready(fh->ioctlfd, p->wait_queue_token);

  XFREE(p->name);
  *pp = p->next;
  XFREE(p);
}


void
autofs_mount_failed(am_node *mp)
{
  autofs_fh_t *fh = mp->am_parent->am_mnt->mf_autofs_fh;
  struct autofs_pending_mount **pp, *p;

  pp = &fh->pending_mounts;
  while (*pp && !STREQ((*pp)->name, mp->am_name))
    pp = &(*pp)->next;

  /* sanity check */
  if (*pp == NULL)
    return;

  p = *pp;
  plog(XLOG_INFO, "autofs: mounting %s failed", mp->am_path);
  send_fail(fh->ioctlfd, p->wait_queue_token);

  XFREE(p->name);
  *pp = p->next;
  XFREE(p);
}


void
autofs_get_opts(char *opts, autofs_fh_t *fh)
{
  sprintf(opts, "fd=%d,minproto=%d,maxproto=%d",
	  fh->kernelfd, AUTOFS_MIN_VERSION, AUTOFS_MAX_VERSION);
}


int
autofs_compute_mount_flags(mntent_t *mnt)
{
  return 0;
}


#ifdef autofs_ptype_expire_multi
static int autofs_timeout_mp_task(void *arg)
{
  am_node *mp = (am_node *)arg;
  autofs_fh_t *fh = mp->am_mnt->mf_autofs_fh;
  int now = 0;

  while (ioctl(fh->ioctlfd, AUTOFS_IOC_EXPIRE_MULTI, &now) == 0);
  return 0;
}
#endif /* autofs_ptype_expire_multi */


void autofs_timeout_mp(am_node *mp)
{
  autofs_fh_t *fh = mp->am_mnt->mf_autofs_fh;
  time_t now = clocktime();

  /* update the ttl, but only if we're not going down */
  if (mp->am_flags & AMF_NOTIMEOUT)
    mp->am_ttl = now + gopt.am_timeo_w;

  if (fh->version < 4) {
    struct autofs_packet_expire pkt;
    while (ioctl(fh->ioctlfd, AUTOFS_IOC_EXPIRE, &pkt) == 0)
      autofs_handle_expire(mp, &pkt);
    return;
  }

#ifdef autofs_ptype_expire_multi
  run_task(autofs_timeout_mp_task, mp, NULL, NULL);
#endif /* autofs_ptype_expire_multi */
}
