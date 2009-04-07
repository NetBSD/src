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
 */

#include "cvs.h"
#include "lstat.h"

#ifdef SERVER_SUPPORT
static void time_stamp_server (const char *, Vers_TS *, Entnode *);
#endif

/* Fill in and return a Vers_TS structure for the file FINFO.
 *
 * INPUTS
 *   finfo		struct file_info data about the file to be examined.
 *   options		Keyword expansion options, I think generally from the
 *			command line.  Can be either NULL or "" to indicate
 *			none are specified here.
 *   tag		Tag specified by user on the command line (via -r).
 *   date		Date specified by user on the command line (via -D).
 *   force_tag_match	If set and TAG is specified, will only set RET->vn_rcs
 *   			based on TAG.  Otherwise, if TAG is specified and does
 *   			not exist in the file, RET->vn_rcs will be set to the
 *   			head revision.
 *   set_time		If set, set the last modification time of the user file
 *			specified by FINFO to the checkin time of RET->vn_rcs.
 *
 * RETURNS
 *   Vers_TS structure for FINFO.
 */
Vers_TS *
Version_TS (struct file_info *finfo, char *options, char *tag, char *date,
            int force_tag_match, int set_time)
{
    Node *p;
    RCSNode *rcsdata;
    Vers_TS *vers_ts;
    struct stickydirtag *sdtp;
    Entnode *entdata;
    char *rcsexpand = NULL;

    /* get a new Vers_TS struct */

    vers_ts = xmalloc (sizeof (Vers_TS));
    memset (vers_ts, 0, sizeof (*vers_ts));

    /*
     * look up the entries file entry and fill in the version and timestamp
     * if entries is NULL, there is no entries file so don't bother trying to
     * look it up (used by checkout -P)
     */
    if (finfo->entries == NULL)
    {
	sdtp = NULL;
	p = NULL;
    }
    else
    {
	p = findnode_fn (finfo->entries, finfo->file);
	sdtp = finfo->entries->list->data; /* list-private */
    }

    if (p == NULL)
    {
	entdata = NULL;
    }
    else
    {
	entdata = p->data;

	if (entdata->type == ENT_SUBDIR)
	{
	    /* According to cvs.texinfo, the various fields in the Entries
	       file for a directory (other than the name) do not have a
	       defined meaning.  We need to pass them along without getting
	       confused based on what is in them.  Therefore we make sure
	       not to set vn_user and the like from Entries, add.c and
	       perhaps other code will expect these fields to be NULL for
	       a directory.  */
	    vers_ts->entdata = entdata;
	}
	else
#ifdef SERVER_SUPPORT
	/* An entries line with "D" in the timestamp indicates that the
	   client sent Is-modified without sending Entry.  So we want to
	   use the entries line for the sole purpose of telling
	   time_stamp_server what is up; we don't want the rest of CVS
	   to think there is an entries line.  */
	if (strcmp (entdata->timestamp, "D") != 0)
#endif
	{
	    vers_ts->vn_user = xstrdup (entdata->version);
	    vers_ts->ts_rcs = xstrdup (entdata->timestamp);
	    vers_ts->ts_conflict = xstrdup (entdata->conflict);
	    if (!(tag || date) && !(sdtp && sdtp->aflag))
	    {
		vers_ts->tag = xstrdup (entdata->tag);
		vers_ts->date = xstrdup (entdata->date);
	    }
	    vers_ts->entdata = entdata;
	}
	/* Even if we don't have an "entries line" as such
	   (vers_ts->entdata), we want to pick up options which could
	   have been from a Kopt protocol request.  */
	if (!options || *options == '\0')
	{
	    if (!(sdtp && sdtp->aflag))
		vers_ts->options = xstrdup (entdata->options);
	}
    }

    /* Always look up the RCS keyword mode when we have an RCS archive.  It
     * will either be needed as a default or to avoid allowing the -k options
     * specified on the command line from overriding binary mode (-kb).
     */
    if (finfo->rcs != NULL)
	rcsexpand = RCS_getexpand (finfo->rcs);

    /*
     * -k options specified on the command line override (and overwrite)
     * options stored in the entries file and default options from the RCS
     * archive, except for binary mode (-kb).
     */
    if (options && *options != '\0')
    {
	if (vers_ts->options != NULL)
	    free (vers_ts->options);
	if (rcsexpand != NULL && strcmp (rcsexpand, "b") == 0)
	    vers_ts->options = xstrdup ("-kb");
	else
	    vers_ts->options = xstrdup (options);
    }
    else if ((!vers_ts->options || *vers_ts->options == '\0')
             && rcsexpand != NULL)
    {
	/* If no keyword expansion was specified on command line,
	   use whatever was in the rcs file (if there is one).  This
	   is how we, if we are the server, tell the client whether
	   a file is binary.  */
	if (vers_ts->options != NULL)
	    free (vers_ts->options);
	vers_ts->options = xmalloc (strlen (rcsexpand) + 3);
	strcpy (vers_ts->options, "-k");
	strcat (vers_ts->options, rcsexpand);
    }
    if (!vers_ts->options)
	vers_ts->options = xstrdup ("");

    /*
     * if tags were specified on the command line, they override what is in
     * the Entries file
     */
    if (tag || date)
    {
	vers_ts->tag = xstrdup (tag);
	vers_ts->date = xstrdup (date);
    }
    else if (!vers_ts->entdata && (sdtp && sdtp->aflag == 0))
    {
	if (!vers_ts->tag)
	{
	    vers_ts->tag = xstrdup (sdtp->tag);
	    vers_ts->nonbranch = sdtp->nonbranch;
	}
	if (!vers_ts->date)
	    vers_ts->date = xstrdup (sdtp->date);
    }

    /* Now look up the info on the source controlled file */
    if (finfo->rcs != NULL)
    {
	rcsdata = finfo->rcs;
	rcsdata->refcount++;
    }
    else if (finfo->repository != NULL)
	rcsdata = RCS_parse (finfo->file, finfo->repository);
    else
	rcsdata = NULL;

    if (rcsdata != NULL)
    {
	/* squirrel away the rcsdata pointer for others */
	vers_ts->srcfile = rcsdata;

	if (vers_ts->tag && strcmp (vers_ts->tag, TAG_BASE) == 0)
	{
	    vers_ts->vn_rcs = xstrdup (vers_ts->vn_user);
	    vers_ts->vn_tag = xstrdup (vers_ts->vn_user);
	}
	else
	{
	    int simple;

	    vers_ts->vn_rcs = RCS_getversion (rcsdata, vers_ts->tag,
					      vers_ts->date, force_tag_match,
					      &simple);
	    if (vers_ts->vn_rcs == NULL)
		vers_ts->vn_tag = NULL;
	    else if (simple)
		vers_ts->vn_tag = xstrdup (vers_ts->tag);
	    else
		vers_ts->vn_tag = xstrdup (vers_ts->vn_rcs);
	}

	/*
	 * If the source control file exists and has the requested revision,
	 * get the Date the revision was checked in.  If "user" exists, set
	 * its mtime.
	 */
	if (set_time && vers_ts->vn_rcs != NULL)
	{
#ifdef SERVER_SUPPORT
	    if (server_active)
		server_modtime (finfo, vers_ts);
	    else
#endif
	    {
		struct utimbuf t;

		memset (&t, 0, sizeof (t));
		t.modtime = RCS_getrevtime (rcsdata, vers_ts->vn_rcs, 0, 0);
		if (t.modtime != (time_t) -1)
		{
#ifdef UTIME_EXPECTS_WRITABLE
		    int change_it_back = 0;
#endif

		    (void) time (&t.actime);

#ifdef UTIME_EXPECTS_WRITABLE
		    if (!iswritable (finfo->file))
		    {
			xchmod (finfo->file, 1);
			change_it_back = 1;
		    }
#endif  /* UTIME_EXPECTS_WRITABLE  */

		    /* This used to need to ignore existence_errors
		       (for cases like where update.c now clears
		       set_time if noexec, but didn't used to).  I
		       think maybe now it doesn't (server_modtime does
		       not like those kinds of cases).  */
		    (void) utime (finfo->file, &t);

#ifdef UTIME_EXPECTS_WRITABLE
		    if (change_it_back)
			xchmod (finfo->file, 0);
#endif  /*  UTIME_EXPECTS_WRITABLE  */
		}
	    }
	}
    }

    /* get user file time-stamp in ts_user */
    if (finfo->entries != NULL)
    {
#ifdef SERVER_SUPPORT
	if (server_active)
	    time_stamp_server (finfo->file, vers_ts, entdata);
	else
#endif
	    vers_ts->ts_user = time_stamp (finfo->file);
    }

    return (vers_ts);
}



#ifdef SERVER_SUPPORT

/* Set VERS_TS->TS_USER to time stamp for FILE.  */

/* Separate these out to keep the logic below clearer.  */
#define mark_lost(V)		((V)->ts_user = 0)
#define mark_unchanged(V)	((V)->ts_user = xstrdup ((V)->ts_rcs))

static void
time_stamp_server (const char *file, Vers_TS *vers_ts, Entnode *entdata)
{
    struct stat sb;
    char *cp;

    TRACE (TRACE_FUNCTION, "time_stamp_server (%s, %s, %s, %s)",
	   file,
	   entdata && entdata->version ? entdata->version : "(null)",
	   entdata && entdata->timestamp ? entdata->timestamp : "(null)",
	   entdata && entdata->conflict ? entdata->conflict : "(null)");

    if (lstat (file, &sb) < 0)
    {
	if (! existence_error (errno))
	    error (1, errno, "cannot stat temp file");

	/* Missing file means lost or unmodified; check entries
	   file to see which.

	   XXX FIXME - If there's no entries file line, we
	   wouldn't be getting the file at all, so consider it
	   lost.  I don't know that that's right, but it's not
	   clear to me that either choice is.  Besides, would we
	   have an RCS string in that case anyways?  */
	if (entdata == NULL)
	    mark_lost (vers_ts);
	else if (entdata->timestamp
		 && entdata->timestamp[0] == '='
		 && entdata->timestamp[1] == '\0')
	    mark_unchanged (vers_ts);
	else if (entdata->conflict
		 && entdata->conflict[0] == '=')
	{
	    /* These just need matching content.  Might as well minimize it.  */
	    vers_ts->ts_user = xstrdup ("");
	    vers_ts->ts_conflict = xstrdup ("");
	}
	else if (entdata->timestamp
		 && (entdata->timestamp[0] == 'M'
		     || entdata->timestamp[0] == 'D')
		 && entdata->timestamp[1] == '\0')
	    vers_ts->ts_user = xstrdup ("Is-modified");
	else
	    mark_lost (vers_ts);
    }
    else if (sb.st_mtime == 0)
    {
	/* We shouldn't reach this case any more!  */
	abort ();
    }
    else
    {
        struct tm *tm_p;

	vers_ts->ts_user = xmalloc (25);
	/* We want to use the same timestamp format as is stored in the
	   st_mtime.  For unix (and NT I think) this *must* be universal
	   time (UT), so that files don't appear to be modified merely
	   because the timezone has changed.  For VMS, or hopefully other
	   systems where gmtime returns NULL, the modification time is
	   stored in local time, and therefore it is not possible to cause
	   st_mtime to be out of sync by changing the timezone.  */
	tm_p = gmtime (&sb.st_mtime);
	cp = tm_p ? asctime (tm_p) : ctime (&sb.st_mtime);
	cp[24] = 0;
	/* Fix non-standard format.  */
	if (cp[8] == '0') cp[8] = ' ';
	(void) strcpy (vers_ts->ts_user, cp);
    }
}

#endif /* SERVER_SUPPORT */



/* Given a UNIX seconds since the epoch, return a string in the format used by
 * the Entries file.
 *
 *
 * INPUTS
 *   UNIXTIME	The timestamp to be formatted.
 *
 * RETURNS
 *   A freshly allocated string the caller is responsible for disposing of.
 */
char *
entries_time (time_t unixtime)
{
    struct tm *tm_p;
    char *cp;

    /* We want to use the same timestamp format as is stored in the
       st_mtime.  For unix (and NT I think) this *must* be universal
       time (UT), so that files don't appear to be modified merely
       because the timezone has changed.  For VMS, or hopefully other
       systems where gmtime returns NULL, the modification time is
       stored in local time, and therefore it is not possible to cause
       st_mtime to be out of sync by changing the timezone.  */
    tm_p = gmtime (&unixtime);
    cp = tm_p ? asctime (tm_p) : ctime (&unixtime);
    /* Get rid of the EOL */
    cp[24] = '\0';
    /* Fix non-standard format.  */
    if (cp[8] == '0') cp[8] = ' ';

    return Xasprintf ("%s", cp);
}



time_t
unix_time_stamp (const char *file)
{
    struct stat sb;
    time_t mtime = 0L;

    if (!lstat (file, &sb))
    {
	mtime = sb.st_mtime;
    }

    /* If it's a symlink, return whichever is the newest mtime of
       the link and its target, for safety.
    */
    if (!stat (file, &sb))
    {
        if (mtime < sb.st_mtime)
	    mtime = sb.st_mtime;
    }

    return mtime;
}



/*
 * Gets the time-stamp for the file "file" and returns it in space it
 * allocates
 */
char *
time_stamp (const char *file)
{
    time_t mtime = unix_time_stamp (file);
    return mtime ? entries_time (mtime) : NULL;
}



/*
 * free up a Vers_TS struct
 */
void
freevers_ts (Vers_TS **versp)
{
    if ((*versp)->srcfile)
	freercsnode (&((*versp)->srcfile));
    if ((*versp)->vn_user)
	free ((*versp)->vn_user);
    if ((*versp)->vn_rcs)
	free ((*versp)->vn_rcs);
    if ((*versp)->vn_tag)
	free ((*versp)->vn_tag);
    if ((*versp)->ts_user)
	free ((*versp)->ts_user);
    if ((*versp)->ts_rcs)
	free ((*versp)->ts_rcs);
    if ((*versp)->options)
	free ((*versp)->options);
    if ((*versp)->tag)
	free ((*versp)->tag);
    if ((*versp)->date)
	free ((*versp)->date);
    if ((*versp)->ts_conflict)
	free ((*versp)->ts_conflict);
    free ((char *) *versp);
    *versp = NULL;
}
