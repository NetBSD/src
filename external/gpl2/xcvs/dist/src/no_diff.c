/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * No Difference
 * 
 * The user file looks modified judging from its time stamp; however it needn't
 * be.  No_Difference() finds out whether it is or not. If it is not, it
 * updates the administration.
 * 
 * returns 0 if no differences are found and non-zero otherwise
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: no_diff.c,v 1.2 2016/05/17 14:00:09 christos Exp $");

#include "cvs.h"
#include <assert.h>

int
No_Difference (struct file_info *finfo, Vers_TS *vers)
{
    Node *p;
    int ret;
    char *ts, *options;
    int retcode = 0;
    char *tocvsPath;

    /* If ts_user is "Is-modified", we can only conclude the files are
       different (since we don't have the file's contents).  */
    if (vers->ts_user != NULL
	&& strcmp (vers->ts_user, "Is-modified") == 0)
	return -1;

    if (!vers->srcfile || !vers->srcfile->path)
	return (-1);			/* different since we couldn't tell */

#ifdef PRESERVE_PERMISSIONS_SUPPORT
    /* If special files are in use, then any mismatch of file metadata
       information also means that the files should be considered different. */
    if (preserve_perms && special_file_mismatch (finfo, vers->vn_user, NULL))
	return 1;
#endif

    if (vers->entdata && vers->entdata->options)
	options = xstrdup (vers->entdata->options);
    else
	options = xstrdup ("");

    tocvsPath = wrap_tocvs_process_file (finfo->file);
    retcode = RCS_cmp_file (vers->srcfile, vers->vn_user, NULL, NULL, options,
			    tocvsPath == NULL ? finfo->file : tocvsPath);
    if (retcode == 0)
    {
	/* no difference was found, so fix the entries file */
	ts = time_stamp (finfo->file);
	Register (finfo->entries, finfo->file,
		  vers->vn_user ? vers->vn_user : vers->vn_rcs, ts,
		  options, vers->tag, vers->date, NULL);
#ifdef SERVER_SUPPORT
	if (server_active)
	{
	    /* We need to update the entries line on the client side.  */
	    server_update_entries (finfo->file, finfo->update_dir,
				   finfo->repository, SERVER_UPDATED);
	}
#endif
	free (ts);

	/* update the entdata pointer in the vers_ts structure */
	p = findnode (finfo->entries, finfo->file);
	assert (p);
	vers->entdata = p->data;

	ret = 0;
    }
    else
	ret = 1;			/* files were really different */

    if (tocvsPath)
    {
	/* Need to call unlink myself because the noexec variable
	 * has been set to 1.  */
	TRACE (TRACE_FUNCTION, "unlink (%s)", tocvsPath);
	if ( CVS_UNLINK (tocvsPath) < 0)
	    error (0, errno, "could not remove %s", tocvsPath);
    }

    free (options);
    return ret;
}
