/*	$NetBSD: amfs_auto.c,v 1.4 2003/07/15 09:01:15 itojun Exp $	*/

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
 * Id: amfs_auto.c,v 1.58 2003/01/25 01:46:23 ib42 Exp
 *
 */

/*
 * Automount file system
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amd.h>

/****************************************************************************
 *** MACROS                                                               ***
 ****************************************************************************/
#define	IN_PROGRESS(cp) ((cp)->mp->am_mnt->mf_flags & MFF_MOUNTING)

#define DOT_DOT_COOKIE	(u_int) 1

/****************************************************************************
 *** STRUCTURES                                                           ***
 ****************************************************************************/


/****************************************************************************
 *** FORWARD DEFINITIONS                                                  ***
 ****************************************************************************/
static int amfs_auto_bgmount(struct continuation *cp);
static int amfs_auto_mount(am_node *mp, mntfs *mf);
static int amfs_auto_readdir_browsable(am_node *mp, nfscookie cookie, nfsdirlist *dp, nfsentry *ep, int count, int fully_browsable);
static mntfs **amfs_auto_lookup_mntfs(am_node *new_mp, int *error_return);
static am_node *amfs_auto_lookup_node(am_node *mp, char *fname, int *error_return);


/****************************************************************************
 *** OPS STRUCTURES                                                       ***
 ****************************************************************************/
am_ops amfs_auto_ops =
{
  "auto",
  amfs_auto_match,
  0,				/* amfs_auto_init */
  amfs_auto_mount,
  amfs_auto_umount,
  amfs_auto_lookup_child,
  amfs_auto_mount_child,
  amfs_auto_readdir,
  0,				/* amfs_auto_readlink */
  amfs_auto_mounted,
  0,				/* amfs_auto_umounted */
  find_amfs_auto_srvr,
  FS_AMQINFO | FS_DIRECTORY | FS_AUTOFS,
#ifdef HAVE_FS_AUTOFS
  AUTOFS_AUTO_FS_FLAGS,
#endif /* HAVE_FS_AUTOFS */
};


/****************************************************************************
 *** FUNCTIONS                                                             ***
 ****************************************************************************/
/*
 * AMFS_AUTO needs nothing in particular.
 */
char *
amfs_auto_match(am_opts *fo)
{
  char *p = fo->opt_rfs;

  if (!fo->opt_rfs) {
    plog(XLOG_USER, "auto: no mount point named (rfs:=)");
    return 0;
  }
  if (!fo->opt_fs) {
    plog(XLOG_USER, "auto: no map named (fs:=)");
    return 0;
  }

  /*
   * Swap round fs:= and rfs:= options
   * ... historical (jsp)
   */
  fo->opt_rfs = fo->opt_fs;
  fo->opt_fs = p;

  /*
   * mtab entry turns out to be the name of the mount map
   */
  return strdup(fo->opt_rfs ? fo->opt_rfs : ".");
}




/*
 * Build a new map cache for this node, or re-use
 * an existing cache for the same map.
 */
void
amfs_auto_mkcacheref(mntfs *mf)
{
  char *cache;

  if (mf->mf_fo && mf->mf_fo->opt_cache)
    cache = mf->mf_fo->opt_cache;
  else
    cache = "none";
  mf->mf_private = (voidp) mapc_find(mf->mf_info, cache,
				     mf->mf_fo->opt_maptype);
  mf->mf_prfree = mapc_free;
}


/*
 * Mount a sub-mount
 */
static int
amfs_auto_mount(am_node *mp, mntfs *mf)
{
  /*
   * Pseudo-directories are used to provide some structure
   * to the automounted directories instead
   * of putting them all in the top-level automount directory.
   *
   * Here, just increment the parent's link count.
   */
  mp->am_parent->am_fattr.na_nlink++;

  /*
   * Info field of . means use parent's info field.
   * Historical - not documented.
   */
  if (mf->mf_info[0] == '.' && mf->mf_info[1] == '\0')
    mf->mf_info = strealloc(mf->mf_info, mp->am_parent->am_mnt->mf_info);

  /*
   * Compute prefix:
   *
   * If there is an option prefix then use that else
   * If the parent had a prefix then use that with name
   *      of this node appended else
   * Use the name of this node.
   *
   * That means if you want no prefix you must say so
   * in the map.
   */
  if (mf->mf_fo->opt_pref) {
    /* allow pref:=null to set a real null prefix */
    if (STREQ(mf->mf_fo->opt_pref, "null")) {
      mp->am_pref = strdup("");
    } else {
      /*
       * the prefix specified as an option
       */
      mp->am_pref = strdup(mf->mf_fo->opt_pref);
    }
  } else {
    /*
     * else the parent's prefix
     * followed by the name
     * followed by /
     */
    char *ppref = mp->am_parent->am_pref;
    if (ppref == 0)
      ppref = "";
    mp->am_pref = str3cat((char *) 0, ppref, mp->am_name, "/");
  }

#ifdef HAVE_FS_AUTOFS
  if (mf->mf_flags & MFF_AUTOFS) {
    char opts[256];
    int error;

    autofs_get_opts(opts, mf->mf_autofs_fh);

    /* now do the mount */
    error = mount_amfs_toplvl(mf, opts);
    if (error) {
      errno = error;
      plog(XLOG_FATAL, "amfs_auto_mount: mount_amfs_toplvl failed: %m");
      return error;
    }
  }
#endif /* HAVE_FS_AUTOFS */

  /*
   * Attach a map cache
   */
  amfs_auto_mkcacheref(mf);

  return 0;
}



void
amfs_auto_mounted(mntfs *mf)
{
  amfs_auto_mkcacheref(mf);
}



/*
 * Unmount an automount sub-node
 */
int
amfs_auto_umount(am_node *mp, mntfs *mf)
{
  int error = 0;

#ifdef HAVE_FS_AUTOFS
  if (mf->mf_flags & MFF_AUTOFS)
    error = UMOUNT_FS(mp->am_path, mf->mf_real_mount, mnttab_file_name);
#endif /* HAVE_FS_AUTOFS */

  return error;
}


/*
 * Discard an old continuation
 */
void
free_continuation(struct continuation *cp)
{
  mntfs **mf;

  dlog("free_continuation");
  if (cp->callout)
    untimeout(cp->callout);
  /*
   * we must free the mntfs's in the list.
   * so free all of them if there was an error,
   * or free all but the used one, if the mount succeeded.
   */
  for (mf = cp->mp->am_mfarray; *mf; mf++)
    /* don't free the mntfs attached to the am_node */
    if (cp->mp->am_mnt != *mf)
      free_mntfs(*mf);
  XFREE(cp->mp->am_mfarray);
  cp->mp->am_mfarray = 0;
  XFREE(cp);
}


/*
 * Replace mount point with a reference to an error filesystem.
 * The mount point (struct mntfs) is NOT discarded,
 * the caller must do it if it wants to _before_ calling this function.
 */
void
assign_error_mntfs(am_node *mp)
{
  dlog("assign_error_mntfs");
  if (mp->am_error > 0) {
    /*
     * Save the old error code
     */
    int error = mp->am_error;
    if (error <= 0)
      error = mp->am_mnt->mf_error;
    /*
     * Allocate a new error reference
     */
    mp->am_mnt = new_mntfs();
    /*
     * Put back the error code
     */
    mp->am_mnt->mf_error = error;
    mp->am_mnt->mf_flags |= MFF_ERROR;
    /*
     * Zero the error in the mount point
     */
    mp->am_error = 0;
  }
}


/*
 * The continuation function.  This is called by
 * the task notifier when a background mount attempt
 * completes.
 */
void
amfs_auto_cont(int rc, int term, voidp closure)
{
  struct continuation *cp = (struct continuation *) closure;
  mntfs *mf = cp->mp->am_mnt;

  /*
   * Definitely not trying to mount at the moment
   */
  mf->mf_flags &= ~MFF_MOUNTING;

  /*
   * While we are mounting - try to avoid race conditions
   */
  new_ttl(cp->mp);

  /*
   * Wakeup anything waiting for this mount
   */
  wakeup((voidp) mf);

  /*
   * Check for termination signal or exit status...
   */
  if (rc || term) {
    am_node *xmp;

    if (term) {
      /*
       * Not sure what to do for an error code.
       */
      mf->mf_error = EIO;	/* XXX ? */
      mf->mf_flags |= MFF_ERROR;
      plog(XLOG_ERROR, "mount for %s got signal %d", cp->mp->am_path, term);
    } else {
      /*
       * Check for exit status...
       */
#ifdef __linux__
      /*
       * HACK ALERT!
       *
       * On Linux (and maybe not only) it's possible to run
       * an amd which "knows" how to mount certain combinations
       * of nfs_proto/nfs_version which the kernel doesn't grok.
       * So if we got an EINVAL and we have a server that's not
       * using NFSv2/UDP, try again with NFSv2/UDP.
       */
      if (rc == EINVAL &&
	  mf->mf_server &&
	  (mf->mf_server->fs_version != 2 ||
	   !STREQ(mf->mf_server->fs_proto, "udp")))
	mf->mf_flags |= MFF_NFS_SCALEDOWN;
      else
#endif /* __linux__ */
      {
	mf->mf_error = rc;
	mf->mf_flags |= MFF_ERROR;
	errno = rc;		/* XXX */
	if (!STREQ(cp->mp->am_mnt->mf_ops->fs_type, "linkx"))
	  plog(XLOG_ERROR, "%s: mount (amfs_auto_cont): %m", cp->mp->am_path);
      }
    }

    if (mf->mf_flags & MFF_NFS_SCALEDOWN)
      amfs_auto_bgmount(cp);
    else {
      /*
       * If we get here then that attempt didn't work, so
       * move the info vector pointer along by one and
       * call the background mount routine again
       */
      amd_stats.d_merr++;
      cp->mf++;
      xmp = cp->mp;
      amfs_auto_bgmount(cp);
      assign_error_mntfs(xmp);
    }
  } else {
    /*
     * The mount worked.
     */
    dlog("Mounting %s returned success", cp->mp->am_path);
    am_mounted(cp->mp);
    free_continuation(cp);
  }

  reschedule_timeout_mp();
}


/*
 * Retry a mount
 */
void
amfs_auto_retry(int rc, int term, voidp closure)
{
  struct continuation *cp = (struct continuation *) closure;
  int error = 0;

  dlog("Commencing retry for mount of %s", cp->mp->am_path);

  new_ttl(cp->mp);

  if ((cp->start + ALLOWED_MOUNT_TIME) < clocktime()) {
    /*
     * The entire mount has timed out.  Set the error code and skip past all
     * the mntfs's so that amfs_auto_bgmount will not have any more
     * ways to try the mount, thus causing an error.
     */
    plog(XLOG_INFO, "mount of \"%s\" has timed out", cp->mp->am_path);
    error = ETIMEDOUT;
    while (*cp->mf)
      cp->mf++;
    /* explicitly forbid further retries after timeout */
    cp->retry = FALSE;
  }
  if (error || !IN_PROGRESS(cp))
    amfs_auto_bgmount(cp);

  reschedule_timeout_mp();
}


/*
 * Try to mount a file system.  Can be called
 * directly or in a sub-process by run_task.
 */
int
try_mount(voidp mvp)
{
  int error = 0;
  am_node *mp = (am_node *) mvp;

  /*
   * Mount it!
   */
  error = mount_node(mp);

  if (error > 0)
    dlog("amfs_auto: call to mount_node(%s) failed: %s",
	 mp->am_path, strerror(error));

  return error;
}


/*
 * Pick a file system to try mounting and
 * do that in the background if necessary
 *
For each location:
	discard previous mount location if required
	fetch next mount location
	if the filesystem failed to be mounted then
		this_error = error from filesystem
		goto failed
	if the filesystem is mounting or unmounting then
		goto retry;
	if the fileserver is down then
		this_error = EIO
		continue;
	if the filesystem is already mounted
		break
	fi

	this_error = initialize mount point

	if no error on this mount and mount is delayed then
		this_error = -1
	fi
	if this_error < 0 then
		retry = true
	fi
	if no error on this mount then
		if mount in background then
			run mount in background
			return -1
		else
			this_error = mount in foreground
		fi
	fi
	if an error occurred on this mount then
		update stats
		save error in mount point
	fi
endfor
 */
static int
amfs_auto_bgmount(struct continuation *cp)
{
  am_node *mp = cp->mp;
  mntfs *mf;			/* Current mntfs */
  int this_error = -1;		/* Per-mount error */
  int hard_error = -1;		/* Cumulative per-node error */

  /*
   * Try to mount each location.
   * At the end:
   * hard_error == 0 indicates something was mounted.
   * hard_error > 0 indicates everything failed with a hard error
   * hard_error < 0 indicates nothing could be mounted now
   */
  for (mp->am_mnt = *cp->mf; *cp->mf; cp->mf++, mp->am_mnt = *cp->mf) {
    am_ops *p;

    mf = mp->am_mnt;
    p = mf->mf_ops;

    if (hard_error < 0)
      hard_error = this_error;
    this_error = 0;

    if (mf->mf_fo->fs_mtab) {
      plog(XLOG_MAP, "Trying mount of %s on %s fstype %s mount_type %s",
	   mf->mf_fo->fs_mtab, mf->mf_fo->opt_fs, p->fs_type,
	   mp->am_parent->am_mnt->mf_fo ? mp->am_parent->am_mnt->mf_fo->opt_mount_type : "root");
    }

    if (mp->am_link) {
      XFREE(mp->am_link);
      mp->am_link = 0;
    }
    if (mf->mf_fo->opt_sublink)
      mp->am_link = strdup(mf->mf_fo->opt_sublink);

    if (mf->mf_error > 0) {
      this_error = mf->mf_error;
      goto failed;
    }

    if (mf->mf_flags & (MFF_MOUNTING | MFF_UNMOUNTING)) {
      /*
       * Still mounting - retry later
       */
      dlog("Duplicate pending mount fstype %s", p->fs_type);
      goto retry;
    }

    if (FSRV_ISDOWN(mf->mf_server)) {
      /*
       * Would just mount from the same place
       * as a hung mount - so give up
       */
      dlog("%s is already hung - giving up", mf->mf_server->fs_host);
      this_error = EIO;
      if (mf->mf_error < 0)
	mf->mf_error = EIO;
      continue;
    }

    if (mf->mf_flags & MFF_MOUNTED) {
      dlog("duplicate mount of \"%s\" ...", mf->mf_info);

      /*
       * Just call am_mounted()
       */
      am_mounted(mp);
      break;
    }

    /*
     * Will usually need to play around with the mount nodes
     * file attribute structure.  This must be done here.
     * Try and get things initialized, even if the fileserver
     * is not known to be up.  In the common case this will
     * progress things faster.
     */

    /*
     * Fill in attribute fields.
     */
    if (mf->mf_fsflags & FS_DIRECTORY)
      mk_fattr(mp, NFDIR);
    else
      mk_fattr(mp, NFLNK);

    if (p->fs_init)
      this_error = (*p->fs_init) (mf);

    if (this_error > 0)
      continue;
    if (this_error < 0)
      goto retry;

    if (mf->mf_fo->opt_delay) {
      /*
       * If there is a delay timer on the mount
       * then don't try to mount if the timer
       * has not expired.
       */
      int i = atoi(mf->mf_fo->opt_delay);
      if (i > 0 && clocktime() < (cp->start + i)) {
	dlog("Mount of %s delayed by %lds", mf->mf_mount, (long) (i - clocktime() + cp->start));
	goto retry;
      }
    }

    /*
     * If the directory is not yet made and it needs to be made, then make it!
     */
    if (!(mf->mf_flags & MFF_MKMNT) && mf->mf_fsflags & FS_MKMNT) {
      plog(XLOG_INFO, "creating mountpoint directory '%s'", mf->mf_real_mount);
      this_error = mkdirs(mf->mf_real_mount, 0555);
      if (this_error) {
	plog(XLOG_ERROR, "mkdirs failed: %s", strerror(this_error));
	continue;
      }
      mf->mf_flags |= MFF_MKMNT;
    }

    if (mf->mf_fsflags & FS_MBACKGROUND) {
      mf->mf_flags |= MFF_MOUNTING;	/* XXX */
      dlog("backgrounding mount of \"%s\"", mf->mf_mount);
      if (cp->callout) {
	untimeout(cp->callout);
	cp->callout = 0;
      }

      /* actually run the task, backgrounding as necessary */
      run_task(try_mount, (voidp) mp, amfs_auto_cont, (voidp) cp);
      return -1;
    } else {
      dlog("foreground mount of \"%s\" ...", mf->mf_info);
      this_error = try_mount((voidp) mp);
      /* do this again, it might have changed */
      mf = mp->am_mnt;
    }

    if (this_error > 0)
      goto failed;
    break;					/* Success */

  retry:
    if (!cp->retry)
      continue;
    dlog("will retry ...\n");

    /*
     * Arrange that amfs_auto_bgmount is called
     * after anything else happens.
     */
    dlog("Arranging to retry mount of %s", mp->am_path);
    sched_task(amfs_auto_retry, (voidp) cp, (voidp) mf);
    if (cp->callout)
      untimeout(cp->callout);
    cp->callout = timeout(RETRY_INTERVAL, wakeup, (voidp) mf);

    mp->am_ttl = clocktime() + RETRY_INTERVAL;

    /*
     * Not done yet - so don't return anything
     */
    return -1;

  failed:
    amd_stats.d_merr++;
    mf->mf_error = this_error;
    mf->mf_flags |= MFF_ERROR;
    if (mf->mf_flags & MFF_MKMNT) {
      rmdirs(mf->mf_real_mount);
      mf->mf_flags &= ~MFF_MKMNT;
    }
    /*
     * Wakeup anything waiting for this mount
     */
    wakeup((voidp) mf);
    /* continue */
  }

  /*
   * If we get here, then either the mount succeeded or
   * there is no more mount information available.
   */
  if (this_error) {
    mp->am_mnt = mf = new_mntfs();

#ifdef HAVE_FS_AUTOFS
    if (mp->am_flags & AMF_AUTOFS)
      autofs_mount_failed(mp);
#endif /* HAVE_FS_AUTOFS */

    if (hard_error <= 0)
      hard_error = this_error;
    if (hard_error < 0)
      hard_error = ETIMEDOUT;

    /*
     * Set a small(ish) timeout on an error node if
     * the error was not a time out.
     */
    switch (hard_error) {
    case ETIMEDOUT:
    case EWOULDBLOCK:
    case EIO:
      mp->am_timeo = 17;
      break;
    default:
      mp->am_timeo = 5;
      break;
    }
    new_ttl(mp);
  } else {
    mf = mp->am_mnt;
    /*
     * Wakeup anything waiting for this mount
     */
    wakeup((voidp) mf);
    hard_error = 0;
  }

  /*
   * Make sure that the error value in the mntfs has a
   * reasonable value.
   */
  if (mf->mf_error < 0) {
    mf->mf_error = hard_error;
    if (hard_error)
      mf->mf_flags |= MFF_ERROR;
  }

  /*
   * In any case we don't need the continuation any more
   */
  free_continuation(cp);

  return hard_error;
}


static char *
amfs_parse_defaults(am_node *mp, mntfs *mf, char *def_opts)
{
  char *dflts;
  char *dfl;
  char **rvec;

  dlog("searching for /defaults entry");
  if (mapc_search((mnt_map *) mf->mf_private, "/defaults", &dflts) == 0) {
    dlog("/defaults gave %s", dflts);
    if (*dflts == '-')
      dfl = dflts + 1;
    else
      dfl = dflts;

    /*
     * Chop the defaults up
     */
    rvec = strsplit(dfl, ' ', '\"');

    if (gopt.flags & CFM_SELECTORS_IN_DEFAULTS) {
      /*
       * Pick whichever first entry matched the list of selectors.
       * Strip the selectors from the string, and assign to dfl the
       * rest of the string.
       */
      if (rvec) {
	am_opts ap;
	am_ops *pt;
	char **sp = rvec;
	while (*sp) {		/* loop until you find something, if any */
	  memset((char *) &ap, 0, sizeof(am_opts));
	  /*
	   * This next routine cause many spurious "expansion of ... is"
	   * messages, which are ignored, b/c all we need out of this
	   * routine is to match selectors.  These spurious messages may
	   * be wrong, esp. if they try to expand ${key} b/c it will
	   * get expanded to "/defaults"
	   */
	  pt = ops_match(&ap, *sp, "", mp->am_path, "/defaults",
			 mp->am_parent->am_mnt->mf_info);
	  free_opts(&ap);	/* don't leak */
	  if (pt == &amfs_error_ops) {
	    plog(XLOG_MAP, "did not match defaults for \"%s\"", *sp);
	  } else {
	    dfl = strip_selectors(*sp, "/defaults");
	    plog(XLOG_MAP, "matched default selectors \"%s\"", dfl);
	    break;
	  }
	  ++sp;
	}
      }
    } else {			/* not selectors_in_defaults */
      /*
       * Extract first value
       */
      dfl = rvec[0];
    }

    /*
     * If there were any values at all...
     */
    if (dfl) {
      /*
       * Log error if there were other values
       */
      if (!(gopt.flags & CFM_SELECTORS_IN_DEFAULTS) && rvec[1]) {
	dlog("/defaults chopped into %s", dfl);
	plog(XLOG_USER, "More than a single value for /defaults in %s", mf->mf_info);
      }

      /*
       * Prepend to existing defaults if they exist,
       * otherwise just use these defaults.
       */
      if (*def_opts && *dfl) {
	char *nopts = (char *) xmalloc(strlen(def_opts) + strlen(dfl) + 2);
	sprintf(nopts, "%s;%s", dfl, def_opts);
	XFREE(def_opts);
	def_opts = nopts;
      } else if (*dfl) {
	def_opts = strealloc(def_opts, dfl);
      }
    }
    XFREE(dflts);
    /*
     * Don't need info vector any more
     */
    XFREE(rvec);
  }

  return def_opts;
}



static am_node *
amfs_auto_lookup_node(am_node *mp, char *fname, int *error_return)
{
  am_node *new_mp;
  int error = 0;		/* Error so far */
  int in_progress = 0;		/* # of (un)mount in progress */
  mntfs *mf;
  char *expanded_fname = 0;

  dlog("in amfs_auto_lookup_node");

  /*
   * If the server is shutting down
   * then don't return information
   * about the mount point.
   */
  if (amd_state == Finishing) {
    if (mp->am_mnt == 0 || mp->am_mnt->mf_fsflags & FS_DIRECT) {
      dlog("%s mount ignored - going down", fname);
    } else {
      dlog("%s/%s mount ignored - going down", mp->am_path, fname);
    }
    ereturn(ENOENT);
  }

  /*
   * Handle special case of "." and ".."
   */
  if (fname[0] == '.') {
    if (fname[1] == '\0')
      return mp;		/* "." is the current node */
    if (fname[1] == '.' && fname[2] == '\0') {
      if (mp->am_parent) {
	dlog(".. in %s gives %s", mp->am_path, mp->am_parent->am_path);
	return mp->am_parent;	/* ".." is the parent node */
      }
      ereturn(ESTALE);
    }
  }

  /*
   * Check for valid key name.
   * If it is invalid then pretend it doesn't exist.
   */
  if (!valid_key(fname)) {
    plog(XLOG_WARNING, "Key \"%s\" contains a disallowed character", fname);
    ereturn(ENOENT);
  }

  /*
   * Expand key name.
   * expanded_fname is now a private copy.
   */
  expanded_fname = expand_selectors(fname);

  /*
   * Search children of this node
   */
  for (new_mp = mp->am_child; new_mp; new_mp = new_mp->am_osib) {
    if (FSTREQ(new_mp->am_name, expanded_fname)) {
      if (new_mp->am_error) {
	error = new_mp->am_error;
	continue;
      }

      /*
       * If the error code is undefined then it must be
       * in progress.
       */
      mf = new_mp->am_mnt;
      if (mf->mf_error < 0)
	goto in_progrss;

      /*
       * If there was a previous error with this node
       * then return that error code.
       */
      if (mf->mf_flags & MFF_ERROR) {
	error = mf->mf_error;
	continue;
      }
      if (!(mf->mf_flags & MFF_MOUNTED) || (mf->mf_flags & MFF_UNMOUNTING)) {
      in_progrss:
	/*
	 * If the fs is not mounted or it is unmounting then there
	 * is a background (un)mount in progress.  In this case
	 * we just drop the RPC request (return nil) and
	 * wait for a retry, by which time the (un)mount may
	 * have completed.
	 */
	dlog("ignoring mount of %s in %s -- %smounting in progress, flags %x",
	     expanded_fname, mf->mf_mount,
	     (mf->mf_flags & MFF_UNMOUNTING) ? "un" : "", mf->mf_flags);
	in_progress++;
	if (mf->mf_flags & MFF_UNMOUNTING) {
	  dlog("will remount later");
	  new_mp->am_flags |= AMF_REMOUNT;
	}
	continue;
      }

      /*
       * Otherwise we have a hit: return the current mount point.
       */
      dlog("matched %s in %s", expanded_fname, new_mp->am_path);
      XFREE(expanded_fname);
      return new_mp;
    }
  }

  if (in_progress) {
    dlog("Waiting while %d mount(s) in progress", in_progress);
    XFREE(expanded_fname);
    ereturn(-1);
  }

  /*
   * If an error occurred then return it.
   */
  if (error) {
    dlog("Returning error: %s", strerror(error));
    XFREE(expanded_fname);
    ereturn(error);
  }

  /*
   * If the server is going down then just return,
   * don't try to mount any more file systems
   */
  if ((int) amd_state >= (int) Finishing) {
    dlog("not found - server going down anyway");
    ereturn(ENOENT);
  }

  /*
   * Allocate a new map
   */
  new_mp = get_ap_child(mp, expanded_fname);
  XFREE(expanded_fname);
  if (new_mp == 0)
    ereturn(ENOSPC);

  *error_return = -1;
  return new_mp;
}



static mntfs *
amfs_auto_lookup_one_mntfs(am_node *new_mp, mntfs *mf, char *ivec,
			   char *def_opts, char *pfname)
{
  am_ops *p;
  am_opts *fs_opts;
  mntfs *new_mf;
  char *link_dir;

  /* match the operators */
  fs_opts = calloc(1, sizeof(am_opts));
  p = ops_match(fs_opts, ivec, def_opts, new_mp->am_path,
		pfname, mf->mf_info);
#ifdef HAVE_FS_AUTOFS
  if (new_mp->am_flags & AMF_AUTOFS) {
    /* ignore user-provided fs if we're using autofs */
    if (fs_opts->opt_sublink) {
      if (fs_opts->opt_sublink[0] == '/') {
	XFREE(fs_opts->opt_fs);
	fs_opts->opt_fs = strdup(new_mp->am_path);
      } else {
	/*
	 * For a relative sublink we need to use a hack with autofe:
	 * mount the filesystem on the original opt_fs (which is NOT an
	 * autofs mountpoint) and symlink (or lofs-mount) to it from
	 * the autofs dir.
	 *
	 * In other words, we make no changes here.
	 */
      }
    } else {
      XFREE(fs_opts->opt_fs);
      fs_opts->opt_fs = strdup(new_mp->am_path);
    }
  }
#endif /* HAVE_FS_AUTOFS */

  /*
   * Find or allocate a filesystem for this node.
   */
  new_mf = find_mntfs(p, fs_opts,
		      fs_opts->opt_fs,
		      fs_opts->fs_mtab,
		      def_opts,
		      fs_opts->opt_opts,
		      fs_opts->opt_remopts);

  /*
   * See whether this is a real filesystem
   */
  p = new_mf->mf_ops;
  if (p == &amfs_error_ops) {
    plog(XLOG_MAP, "Map entry %s for %s did not match", ivec, new_mp->am_path);
    free_mntfs(new_mf);
    return NULL;
  }

  dlog("Got a hit with %s", p->fs_type);

#ifdef HAVE_FS_AUTOFS
  if (new_mp->am_flags & AMF_AUTOFS) {
    new_mf->mf_fsflags = new_mf->mf_ops->autofs_fs_flags;
#ifdef NEED_AUTOFS_SPACE_HACK
    free(new_mf->mf_real_mount);
    new_mf->mf_real_mount = autofs_strdup_space_hack(new_mf->mf_mount);
#endif /* NEED_AUTOFS_SPACE_HACK */
  }
  if (new_mf->mf_fsflags & FS_AUTOFS &&
      mf->mf_flags & MFF_AUTOFS)
    new_mf->mf_flags |= MFF_AUTOFS;
#endif /* HAVE_FS_AUTOFS */

  link_dir = new_mf->mf_fo->opt_sublink;
  if (link_dir && link_dir[0] && link_dir[0] != '/') {
    link_dir = str3cat((char *) 0,
		       new_mf->mf_fo->opt_fs, "/", link_dir);
    normalize_slash(link_dir);
    XFREE(new_mf->mf_fo->opt_sublink);
    new_mf->mf_fo->opt_sublink = link_dir;
  }
  return new_mf;
}


static mntfs **
amfs_auto_lookup_mntfs(am_node *new_mp, int *error_return)
{
  am_node *mp;
  char *info;			/* Mount info - where to get the file system */
  char **ivecs, **cur_ivec;	/* Split version of info */
  int num_ivecs;
  char *def_opts;	       	/* Automount options */
  int error = 0;		/* Error so far */
  char path_name[MAXPATHLEN];	/* General path name buffer */
  char *pfname;			/* Path for database lookup */
  mntfs *mf, **mf_array;
  int count;

  dlog("in amfs_auto_lookup_mntfs");

  mp = new_mp->am_parent;

  /*
   * If we get here then this is a reference to an,
   * as yet, unknown name so we need to search the mount
   * map for it.
   */
  if (mp->am_pref) {
    if (strlen(mp->am_pref) + strlen(new_mp->am_name) >= sizeof(path_name))
      ereturn(ENAMETOOLONG);
    snprintf(path_name, sizeof(path_name), "%s%s", mp->am_pref, new_mp->am_name);
    pfname = path_name;
  } else {
    pfname = new_mp->am_name;
  }

  mf = mp->am_mnt;

  dlog("will search map info in %s to find %s", mf->mf_info, pfname);
  /*
   * Consult the oracle for some mount information.
   * info is malloc'ed and belongs to this routine.
   * It ends up being free'd in free_continuation().
   *
   * Note that this may return -1 indicating that information
   * is not yet available.
   */
  error = mapc_search((mnt_map *) mf->mf_private, pfname, &info);
  if (error) {
    if (error > 0)
      plog(XLOG_MAP, "No map entry for %s", pfname);
    else
      plog(XLOG_MAP, "Waiting on map entry for %s", pfname);
    ereturn(error);
  }
  dlog("mount info is %s", info);

  /*
   * Split info into an argument vector.
   * The vector is malloc'ed and belongs to
   * this routine.  It is free'd further down.
   *
   * Note: the vector pointers point into info, so don't free it!
   */
  ivecs = strsplit(info, ' ', '\"');

  if (mf->mf_auto)
    def_opts = mf->mf_auto;
  else
    def_opts = "";

  def_opts = amfs_parse_defaults(mp, mf, strdup(def_opts));

  /* first build our defaults */
  num_ivecs = 0;
  for (cur_ivec = ivecs; *cur_ivec; cur_ivec++) {
    if (**cur_ivec == '-') {
      /*
       * Pick up new defaults
       */
      char *old_def_opts = def_opts;
      def_opts = str3cat((char *) 0, old_def_opts, ";", *cur_ivec + 1);
      dlog("Setting def_opts to \"%s\"", def_opts);
      continue;
    } else
      num_ivecs++;
  }

  mf_array = calloc(num_ivecs + 1, sizeof(mntfs *));

  /* construct the array of struct mntfs for this mount point */
  for (count = 0, cur_ivec = ivecs; *cur_ivec; cur_ivec++) {
    mntfs *new_mf;

    /* we've dealt with the defaults already */
    if (**cur_ivec == '-')
      continue;

    /*
     * If a mntfs has already been found, and we find
     * a cut then don't try any more locations.
     */
    if (STREQ(*cur_ivec, "/") || STREQ(*cur_ivec, "||")) {
      if (count > 0) {
	dlog("Cut: not trying any more locations for %s", mp->am_path);
	break;
      }
      continue;
    }

    new_mf = amfs_auto_lookup_one_mntfs(new_mp, mf, *cur_ivec, def_opts, pfname);
    if (new_mf == NULL)
      continue;
    mf_array[count++] = new_mf;
  }

  /* We're done with ivecs */
  XFREE(ivecs);
  XFREE(info);
  if (count == 0) {			/* no match */
    XFREE(mf_array);
    ereturn(ENOENT);
  }

  return mf_array;
}


am_node *
amfs_auto_mount_child(am_node *new_mp, int *error_return)
{
  int error;
  struct continuation *cp;	/* Continuation structure if need to mount */

  dlog("in amfs_auto_mount_child");

  *error_return = error = 0;	/* Error so far */

  /* we have an errorfs attached to the am_node, free it */
  free_mntfs(new_mp->am_mnt);
  new_mp->am_mnt = 0;

  /*
   * Construct a continuation
   */
  cp = ALLOC(struct continuation);
  cp->callout = 0;
  cp->mp = new_mp;
  cp->retry = TRUE;
  cp->start = clocktime();
  cp->mf = new_mp->am_mfarray;

  /*
   * Try and mount the file system.  If this succeeds immediately (possible
   * for a ufs file system) then return the attributes, otherwise just
   * return an error.
   */
  error = amfs_auto_bgmount(cp);
  reschedule_timeout_mp();
  if (!error)
    return new_mp;

  /*
   * Code for quick reply.  If current_transp is set, then it's the
   * transp that's been passed down from nfs_program_2() or from
   * autofs_program_[123]().
   * If new_mp->am_transp is not already set, set it by copying in
   * current_transp.  Once am_transp is set, nfs_quick_reply() and
   * autofs_mount_succeeded() can use it to send a reply to the
   * client that requested this mount.
   */
  if (current_transp && !new_mp->am_transp) {
    new_mp->am_transp = (SVCXPRT *) xmalloc(sizeof(SVCXPRT));
    *(new_mp->am_transp) = *current_transp;
  }
  if (error && (new_mp->am_mnt->mf_ops == &amfs_error_ops))
    new_mp->am_error = error;

  assign_error_mntfs(new_mp);

  ereturn(error);
}


/*
 * Automount interface to RPC lookup routine
 * Find the corresponding entry and return
 * the file handle for it.
 */
am_node *
amfs_auto_lookup_child(am_node *mp, char *fname, int *error_return, int op)
{
  am_node *new_mp;
  mntfs **mf_array;
  int mp_error;

  dlog("in amfs_auto_lookup_child");

  *error_return = 0;
  new_mp = amfs_auto_lookup_node(mp, fname, error_return);

  /* return if we got an error */
  if (!new_mp || *error_return > 0)
    return new_mp;

  /* also return if it's already mounted and known to be up */
  if (*error_return == 0 && FSRV_ISUP(new_mp->am_mnt->mf_server))
    return new_mp;

  if (op == VLOOK_DELETE)
    /*
     * If doing a delete then don't create again!
     */
    ereturn(ENOENT);

  /* save error_return */
  mp_error = *error_return;

  mf_array = amfs_auto_lookup_mntfs(new_mp, error_return);
  if (!mf_array) {
    new_mp->am_error = new_mp->am_mnt->mf_error = *error_return;
    free_map(new_mp);
    return NULL;
  }

  /*
   * Already mounted but known to be down:
   * check if we have any alternatives to mount
   */
  if (mp_error == 0) {
    mntfs **mfp;
    for (mfp = mf_array; *mfp; mfp++)
      if (*mfp != new_mp->am_mnt)
	break;
    if (*mfp != NULL) {
      /*
       * we found an alternative, so try mounting again.
       */
      *error_return = -1;
    } else {
      for (mfp = mf_array; *mfp; mfp++)
	free_mntfs(*mfp);
      XFREE(mf_array);
      if (new_mp->am_flags & AMF_SOFTLOOKUP) {
	ereturn(EIO);
      } else {
	*error_return = 0;
	return new_mp;
      }
    }
  }

  /* store the array inside the am_node */
  new_mp->am_mfarray = mf_array;

  /*
   * XXX: we need to sort the mntfs's we got.
   * the order should be something like:
   * 1. link
   * 2. anything else (XXX -- any other preferences?)
   */
  return new_mp;
}


/*
 * Locate next node in sibling list which is mounted
 * and is not an error node.
 */
am_node *
next_nonerror_node(am_node *xp)
{
  mntfs *mf;

  /*
   * Bug report (7/12/89) from Rein Tollevik <rein@ifi.uio.no>
   * Fixes a race condition when mounting direct automounts.
   * Also fixes a problem when doing a readdir on a directory
   * containing hung automounts.
   */
  while (xp &&
	 (!(mf = xp->am_mnt) ||	/* No mounted filesystem */
	  mf->mf_error != 0 ||	/* There was a mntfs error */
	  xp->am_error != 0 ||	/* There was a mount error */
	  !(mf->mf_flags & MFF_MOUNTED) ||	/* The fs is not mounted */
	  (mf->mf_server->fs_flags & FSF_DOWN))	/* The fs may be down */
	 )
    xp = xp->am_osib;

  return xp;
}


/*
 * This readdir function which call a special version of it that allows
 * browsing if browsable_dirs=yes was set on the map.
 */
int
amfs_auto_readdir(am_node *mp, nfscookie cookie, nfsdirlist *dp, nfsentry *ep, int count)
{
  u_int gen = *(u_int *) cookie;
  am_node *xp;
  mntent_t mnt;

  dp->dl_eof = FALSE;		/* assume readdir not done */

  /* check if map is browsable */
  if (mp->am_mnt && mp->am_mnt->mf_mopts) {
    mnt.mnt_opts = mp->am_mnt->mf_mopts;
    if (amu_hasmntopt(&mnt, "fullybrowsable"))
      return amfs_auto_readdir_browsable(mp, cookie, dp, ep, count, TRUE);
    if (amu_hasmntopt(&mnt, "browsable"))
      return amfs_auto_readdir_browsable(mp, cookie, dp, ep, count, FALSE);
  }

  /* when gen is 0, we start reading from the beginning of the directory */
  if (gen == 0) {
    /*
     * In the default instance (which is used to start a search) we return
     * "." and "..".
     *
     * This assumes that the count is big enough to allow both "." and ".."
     * to be returned in a single packet.  If it isn't (which would be
     * fairly unbelievable) then tough.
     */
    dlog("amfs_auto_readdir: default search");
    /*
     * Check for enough room.  This is extremely approximate but is more
     * than enough space.  Really need 2 times:
     *      4byte fileid
     *      4byte cookie
     *      4byte name length
     *      4byte name
     * plus the dirlist structure */
    if (count < (2 * (2 * (sizeof(*ep) + sizeof("..") + 4) + sizeof(*dp))))
      return EINVAL;

    xp = next_nonerror_node(mp->am_child);
    dp->dl_entries = ep;

    /* construct "." */
    ep[0].ne_fileid = mp->am_gen;
    ep[0].ne_name = ".";
    ep[0].ne_nextentry = &ep[1];
    *(u_int *) ep[0].ne_cookie = 0;

    /* construct ".." */
    if (mp->am_parent)
      ep[1].ne_fileid = mp->am_parent->am_gen;
    else
      ep[1].ne_fileid = mp->am_gen;
    ep[1].ne_name = "..";
    ep[1].ne_nextentry = 0;
    *(u_int *) ep[1].ne_cookie = (xp ? xp->am_gen : DOT_DOT_COOKIE);

    if (!xp)
      dp->dl_eof = TRUE;	/* by default assume readdir done */

    if (amuDebug(D_READDIR)) {
      nfsentry *ne;
      int j;
      for (j = 0, ne = ep; ne; ne = ne->ne_nextentry)
	plog(XLOG_DEBUG, "gen1 key %4d \"%s\" fi=%d ck=%d",
	     j++, ne->ne_name, ne->ne_fileid, *(u_int *)ne->ne_cookie);
    }
    return 0;
  }
  dlog("amfs_auto_readdir: real child");

  if (gen == DOT_DOT_COOKIE) {
    dlog("amfs_auto_readdir: End of readdir in %s", mp->am_path);
    dp->dl_eof = TRUE;
    dp->dl_entries = 0;
    if (amuDebug(D_READDIR))
      plog(XLOG_DEBUG, "end of readdir eof=TRUE, dl_entries=0\n");
    return 0;
  }

  /* non-browsable directories code */
  xp = mp->am_child;
  while (xp && xp->am_gen != gen)
    xp = xp->am_osib;

  if (xp) {
    int nbytes = count / 2;	/* conservative */
    int todo = MAX_READDIR_ENTRIES;

    dp->dl_entries = ep;
    do {
      am_node *xp_next = next_nonerror_node(xp->am_osib);

      if (xp_next) {
	*(u_int *) ep->ne_cookie = xp_next->am_gen;
      } else {
	*(u_int *) ep->ne_cookie = DOT_DOT_COOKIE;
	dp->dl_eof = TRUE;
      }

      ep->ne_fileid = xp->am_gen;
      ep->ne_name = xp->am_name;
      nbytes -= sizeof(*ep) + 1;
      if (xp->am_name)
	nbytes -= strlen(xp->am_name);

      xp = xp_next;

      if (nbytes > 0 && !dp->dl_eof && todo > 1) {
	ep->ne_nextentry = ep + 1;
	ep++;
	--todo;
      } else {
	todo = 0;
      }
    } while (todo > 0);

    ep->ne_nextentry = 0;

    if (amuDebug(D_READDIR)) {
      nfsentry *ne;
      int j;
      for (j=0,ne=ep; ne; ne=ne->ne_nextentry)
	plog(XLOG_DEBUG, "gen2 key %4d \"%s\" fi=%d ck=%d",
	     j++, ne->ne_name, ne->ne_fileid, *(u_int *)ne->ne_cookie);
    }
    return 0;
  }
  return ESTALE;
}


/* This one is called only if map is browsable */
static int
amfs_auto_readdir_browsable(am_node *mp, nfscookie cookie, nfsdirlist *dp, nfsentry *ep, int count, int fully_browsable)
{
  u_int gen = *(u_int *) cookie;
  int chain_length, i;
  static nfsentry *te, *te_next;
  static int j;

  dp->dl_eof = FALSE;		/* assume readdir not done */

  if (amuDebug(D_READDIR))
    plog(XLOG_DEBUG, "amfs_auto_readdir_browsable gen=%u, count=%d",
	 gen, count);

  if (gen == 0) {
    /*
     * In the default instance (which is used to start a search) we return
     * "." and "..".
     *
     * This assumes that the count is big enough to allow both "." and ".."
     * to be returned in a single packet.  If it isn't (which would be
     * fairly unbelievable) then tough.
     */
    dlog("amfs_auto_readdir_browsable: default search");
    /*
     * Check for enough room.  This is extremely approximate but is more
     * than enough space.  Really need 2 times:
     *      4byte fileid
     *      4byte cookie
     *      4byte name length
     *      4byte name
     * plus the dirlist structure */
    if (count < (2 * (2 * (sizeof(*ep) + sizeof("..") + 4) + sizeof(*dp))))
      return EINVAL;

    /*
     * compute # of entries to send in this chain.
     * heuristics: 128 bytes per entry.
     * This is too much probably, but it seems to work better because
     * of the re-entrant nature of nfs_readdir, and esp. on systems
     * like OpenBSD 2.2.
     */
    chain_length = count / 128;

    /* reset static state counters */
    te = te_next = NULL;

    dp->dl_entries = ep;

    /* construct "." */
    ep[0].ne_fileid = mp->am_gen;
    ep[0].ne_name = ".";
    ep[0].ne_nextentry = &ep[1];
    *(u_int *) ep[0].ne_cookie = 0;

    /* construct ".." */
    if (mp->am_parent)
      ep[1].ne_fileid = mp->am_parent->am_gen;
    else
      ep[1].ne_fileid = mp->am_gen;

    ep[1].ne_name = "..";
    ep[1].ne_nextentry = 0;
    *(u_int *) ep[1].ne_cookie = DOT_DOT_COOKIE;

    /*
     * If map is browsable, call a function make_entry_chain() to construct
     * a linked list of unmounted keys, and return it.  Then link the chain
     * to the regular list.  Get the chain only once, but return
     * chunks of it each time.
     */
    te = make_entry_chain(mp, dp->dl_entries, fully_browsable);
    if (!te)
      return 0;
    if (amuDebug(D_READDIR)) {
      nfsentry *ne;
      for (j = 0, ne = te; ne; ne = ne->ne_nextentry)
	plog(XLOG_DEBUG, "gen1 key %4d \"%s\"", j++, ne->ne_name);
    }

    /* return only "chain_length" entries */
    te_next = te;
    for (i=1; i<chain_length; ++i) {
      te_next = te_next->ne_nextentry;
      if (!te_next)
	break;
    }
    if (te_next) {
      nfsentry *te_saved = te_next->ne_nextentry;
      te_next->ne_nextentry = NULL; /* terminate "te" chain */
      te_next = te_saved;	/* save rest of "te" for next iteration */
      dp->dl_eof = FALSE;	/* tell readdir there's more */
    } else {
      dp->dl_eof = TRUE;	/* tell readdir that's it */
    }
    ep[1].ne_nextentry = te;	/* append this chunk of "te" chain */
    if (amuDebug(D_READDIR)) {
      nfsentry *ne;
      for (j = 0, ne = te; ne; ne = ne->ne_nextentry)
	plog(XLOG_DEBUG, "gen2 key %4d \"%s\"", j++, ne->ne_name);
      for (j = 0, ne = ep; ne; ne = ne->ne_nextentry)
	plog(XLOG_DEBUG, "gen2+ key %4d \"%s\" fi=%d ck=%d",
	     j++, ne->ne_name, ne->ne_fileid, *(u_int *)ne->ne_cookie);
      plog(XLOG_DEBUG, "EOF is %d", dp->dl_eof);
    }
    return 0;
  } /* end of "if (gen == 0)" statement */

  dlog("amfs_auto_readdir_browsable: real child");

  if (gen == DOT_DOT_COOKIE) {
    dlog("amfs_auto_readdir_browsable: End of readdir in %s", mp->am_path);
    dp->dl_eof = TRUE;
    dp->dl_entries = 0;
    return 0;
  }

  /*
   * If browsable directories, then continue serving readdir() with another
   * chunk of entries, starting from where we left off (when gen was equal
   * to 0).  Once again, assume last chunk served to readdir.
   */
  dp->dl_eof = TRUE;
  dp->dl_entries = ep;

  te = te_next;			/* reset 'te' from last saved te_next */
  if (!te) {			/* another indicator of end of readdir */
    dp->dl_entries = 0;
    return 0;
  }
  /*
   * compute # of entries to send in this chain.
   * heuristics: 128 bytes per entry.
   */
  chain_length = count / 128;

  /* return only "chain_length" entries */
  for (i = 1; i < chain_length; ++i) {
    te_next = te_next->ne_nextentry;
    if (!te_next)
      break;
  }
  if (te_next) {
    nfsentry *te_saved = te_next->ne_nextentry;
    te_next->ne_nextentry = NULL; /* terminate "te" chain */
    te_next = te_saved;		/* save rest of "te" for next iteration */
    dp->dl_eof = FALSE;		/* tell readdir there's more */
  }
  ep = te;			/* send next chunk of "te" chain */
  dp->dl_entries = ep;
  if (amuDebug(D_READDIR)) {
    nfsentry *ne;
    plog(XLOG_DEBUG, "dl_entries=0x%lx, te_next=0x%lx, dl_eof=%d",
	 (u_long) dp->dl_entries,
	 (u_long) te_next,
	 dp->dl_eof);
    for (ne = te; ne; ne = ne->ne_nextentry)
      plog(XLOG_DEBUG, "gen3 key %4d \"%s\"", j++, ne->ne_name);
  }
  return 0;
}
