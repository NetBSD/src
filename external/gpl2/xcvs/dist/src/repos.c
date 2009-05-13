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
#include "getline.h"



/* Determine the name of the RCS repository for directory DIR in the
   current working directory, or for the current working directory
   itself if DIR is NULL.  Returns the name in a newly-malloc'd
   string.  On error, gives a fatal error and does not return.
   UPDATE_DIR is the path from where cvs was invoked (for use in error
   messages), and should contain DIR as its last component.
   UPDATE_DIR can be NULL to signify the directory in which cvs was
   invoked.  */

char *
Name_Repository (const char *dir, const char *update_dir)
{
    FILE *fpin;
    const char *xupdate_dir;
    char *repos = NULL;
    size_t repos_allocated = 0;
    char *tmp;
    char *cp;

    if (update_dir && *update_dir)
	xupdate_dir = update_dir;
    else
	xupdate_dir = ".";

    if (dir != NULL)
	tmp = Xasprintf ("%s/%s", dir, CVSADM_REP);
    else
	tmp = xstrdup (CVSADM_REP);

    /*
     * The assumption here is that the repository is always contained in the
     * first line of the "Repository" file.
     */
    fpin = CVS_FOPEN (tmp, "r");

    if (fpin == NULL)
    {
	int save_errno = errno;
	char *cvsadm;

	if (dir != NULL)
	    cvsadm = Xasprintf ("%s/%s", dir, CVSADM);
	else
	    cvsadm = xstrdup (CVSADM);

	if (!isdir (cvsadm))
	{
	    error (0, 0, "in directory `%s':", xupdate_dir);
	    error (1, 0, "there is no version here; do `%s checkout' first",
		   program_name);
	}
	free (cvsadm);

	if (existence_error (save_errno))
	{
	    /* This occurs at least in the case where the user manually
	     * creates a directory named CVS.
	     */
	    error (0, 0, "in directory `%s':", xupdate_dir);
	    error (0, 0, "CVS directory found without administrative files.");
	    error (0, 0, "Use CVS to create the CVS directory, or rename the");
	    error (0, 0, "directory if it is intended to store something");
	    error (0, 0, "besides CVS administrative files.");
	    error (1, 0, "*PANIC* administration files missing!");
	}

	error (1, save_errno, "cannot open `%s'", tmp);
    }

    if (getline (&repos, &repos_allocated, fpin) < 0)
    {
	/* FIXME: should be checking for end of file separately.  */
	error (0, 0, "in directory `%s':", xupdate_dir);
	error (1, errno, "cannot read `%s'", CVSADM_REP);
    }
    if (fclose (fpin) < 0)
	error (0, errno, "cannot close `%s'", tmp);
    free (tmp);

    if ((cp = strrchr (repos, '\n')) != NULL)
	*cp = '\0';			/* strip the newline */

    /* If this is a relative repository pathname, turn it into an absolute
     * one by tacking on the current root.  There is no need to grab it from
     * the CVS/Root file via the Name_Root() function because by the time
     * this function is called, we the contents of CVS/Root have already been
     * compared to original_root and found to match.
     */
    if (!ISABSOLUTE (repos))
    {
	char *newrepos;

	if (current_parsed_root == NULL)
	{
	    error (0, 0, "in directory `%s:", xupdate_dir);
	    error (0, 0, "must set the CVSROOT environment variable\n");
	    error (0, 0, "or specify the '-d' option to `%s'.", program_name);
	    error (1, 0, "invalid repository setting");
	}
	if (pathname_levels (repos) > 0)
	{
	    error (0, 0, "in directory `%s':", xupdate_dir);
	    error (0, 0, "`..'-relative repositories are not supported.");
	    error (1, 0, "invalid source repository");
	}
	newrepos = Xasprintf ("%s/%s", original_parsed_root->directory, repos);
	free (repos);
	repos = newrepos;
    }

    Sanitize_Repository_Name (repos);

    return repos;
}



/*
 * Return a pointer to the repository name relative to CVSROOT from a
 * possibly fully qualified repository
 */
const char *
Short_Repository (const char *repository)
{
    if (repository == NULL)
	return NULL;

    /* If repository matches CVSroot at the beginning, strip off CVSroot */
    /* And skip leading '/' in rep, in case CVSroot ended with '/'. */
    if (strncmp (original_parsed_root->directory, repository,
		 strlen (original_parsed_root->directory)) == 0)
    {
	const char *rep = repository + strlen (original_parsed_root->directory);
	return (*rep == '/') ? rep+1 : rep;
    }
    else
	return repository;
}



/* Sanitize the repository name (in place) by removing trailing
 * slashes and a trailing "." if present.  It should be safe for
 * callers to use strcat and friends to create repository names.
 * Without this check, names like "/path/to/repos/./foo" and
 * "/path/to/repos//foo" would be created.  For example, one
 * significant case is the CVSROOT-detection code in commit.c.  It
 * decides whether or not it needs to rebuild the administrative file
 * database by doing a string compare.  If we've done a `cvs co .' to
 * get the CVSROOT files, "/path/to/repos/./CVSROOT" and
 * "/path/to/repos/CVSROOT" are the arguments that are compared!
 *
 * This function ends up being called from the same places as
 * strip_path, though what it does is much more conservative.  Many
 * comments about this operation (which was scattered around in
 * several places in the source code) ran thus:
 *
 *    ``repository ends with "/."; omit it.  This sort of thing used
 *    to be taken care of by strip_path.  Now we try to be more
 *    selective.  I suspect that it would be even better to push it
 *    back further someday, so that the trailing "/." doesn't get into
 *    repository in the first place, but we haven't taken things that
 *    far yet.''        --Jim Kingdon (recurse.c, 07-Sep-97)
 */

void
Sanitize_Repository_Name (char *repository)
{
    size_t len;

    assert (repository != NULL);

    strip_trailing_slashes (repository);

    len = strlen (repository);
    if (len >= 2
	&& repository[len - 1] == '.'
	&& ISSLASH (repository[len - 2]))
    {
	repository[len - 2] = '\0';
    }
}
