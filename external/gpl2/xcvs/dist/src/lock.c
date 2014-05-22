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
 * Set Lock
 * 
 * Lock file support for CVS.
 */

/* The node Concurrency in doc/cvs.texinfo has a brief introduction to
   how CVS locks function, and some of the user-visible consequences of
   their existence.  Here is a summary of why they exist (and therefore,
   the consequences of hacking CVS to read a repository without creating
   locks):

   There are two uses.  One is the ability to prevent there from being
   two writers at the same time.  This is necessary for any number of
   reasons (fileattr code, probably others).  Commit needs to lock the
   whole tree so that nothing happens between the up-to-date check and
   the actual checkin.

   The second use is the ability to ensure that there is not a writer
   and a reader at the same time (several readers are allowed).  Reasons
   for this are:

   * Readlocks ensure that once CVS has found a collection of rcs
   files using Find_Names, the files will still exist when it reads
   them (they may have moved in or out of the attic).

   * Readlocks provide some modicum of consistency, although this is
   kind of limited--see the node Concurrency in cvs.texinfo.

   * Readlocks ensure that the RCS file does not change between
   RCS_parse and RCS_reparsercsfile time.  This one strikes me as
   important, although I haven't thought up what bad scenarios might
   be.

   * Readlocks ensure that we won't find the file in the state in
   which it is in between the calls to add_rcs_file and RCS_checkin in
   commit.c (when a file is being added).  This state is a state in
   which the RCS file parsing routines in rcs.c cannot parse the file.

   * Readlocks ensure that a reader won't try to look at a
   half-written fileattr file (fileattr is not updated atomically).

   (see also the description of anonymous read-only access in
   "Password authentication security" node in doc/cvs.texinfo).

   While I'm here, I'll try to summarize a few random suggestions
   which periodically get made about how locks might be different:

   1.  Check for EROFS.  Maybe useful, although in the presence of NFS
   EROFS does *not* mean that the file system is unchanging.

   2.  Provide an option to disable locks for operations which only
   read (see above for some of the consequences).

   3.  Have a server internally do the locking.  Probably a good
   long-term solution, and many people have been working hard on code
   changes which would eventually make it possible to have a server
   which can handle various connections in one process, but there is
   much, much work still to be done before this is feasible.  */

#include "cvs.h"



struct lock {
    /* This is the directory in which we may have a lock named by the
       readlock variable, a lock named by the writelock variable, and/or
       a lock named CVSLCK.  The storage is not allocated along with the
       struct lock; it is allocated by the Reader_Lock caller or in the
       case of promotablelocks, it is just a pointer to the storage allocated
       for the ->key field.  */
    const char *repository;

    /* The name of the lock files. */
    char *file1;
#ifdef LOCK_COMPATIBILITY
    char *file2;
#endif /* LOCK_COMPATIBILITY */

    /* The name of the master lock dir.  Usually CVSLCK.  */
    const char *lockdirname;

    /* The full path to the lock dir, if we are currently holding it.
     *
     * This will be LOCKDIRNAME catted onto REPOSITORY.  We waste a little
     * space by storing it, but save a later malloc/free.
     */
    char *lockdir;

    /* Note there is no way of knowing whether the readlock and writelock
       exist.  The code which sets the locks doesn't use SIG_beginCrSect
       to set a flag like we do for CVSLCK.  */
    bool free_repository;
};

static void remove_locks (void);
static int set_lock (struct lock *lock, int will_wait);
static void clear_lock (struct lock *lock);
static void set_lockers_name (struct stat *statp);

/* Malloc'd array containing the username of the whoever has the lock.
   Will always be non-NULL in the cases where it is needed.  */
static char *lockers_name;
/* Malloc'd array specifying name of a readlock within a directory.
   Or NULL if none.  */
static char *readlock;
/* Malloc'd array specifying name of a writelock within a directory.
   Or NULL if none.  */
static char *writelock;
/* Malloc'd array specifying name of a promotablelock within a directory.
   Or NULL if none.  */
static char *promotablelock;
static List *locklist;

#define L_OK		0		/* success */
#define L_ERROR		1		/* error condition */
#define L_LOCKED	2		/* lock owned by someone else */

/* This is the (single) readlock which is set by Reader_Lock.  The
   repository field is NULL if there is no such lock.  */
#ifdef LOCK_COMPATIBILITY
static struct lock global_readlock = {NULL, NULL, NULL, CVSLCK, NULL, false};
static struct lock global_writelock = {NULL, NULL, NULL, CVSLCK, NULL, false};

static struct lock global_history_lock = {NULL, NULL, NULL, CVSHISTORYLCK,
					  NULL, false};
static struct lock global_val_tags_lock = {NULL, NULL, NULL, CVSVALTAGSLCK,
					   NULL, false};
#else
static struct lock global_readlock = {NULL, NULL, CVSLCK, NULL, false};
static struct lock global_writelock = {NULL, NULL, CVSLCK, NULL, false};

static struct lock global_history_lock = {NULL, NULL, CVSHISTORYLCK, NULL,
					  false};
static struct lock global_val_tags_lock = {NULL, NULL, CVSVALTAGSLCK, NULL,
					   false};
#endif /* LOCK_COMPATIBILITY */

/* List of locks set by lock_tree_for_write.  This is redundant
   with locklist, sort of.  */
static List *lock_tree_list;


/*
 * Find the root directory in the repository directory
 */
static int
find_root (const char *repository, char *rootdir)
{
    struct stat strep, stroot;
    char *p = NULL, *q = NULL;
    size_t len;

    if (stat (rootdir, &stroot) == -1)
	return -1;
    len = strlen (repository);
    do {
	if (p != NULL) {
	    len = p - repository;
	    *p = '\0';
	}
	if (q != NULL)
	    *q = '/';
	if (stat(repository, &strep) == -1) {
	    if (p != NULL)
		*p = '/';
	    return -1;
	}
	if (strep.st_dev == stroot.st_dev && strep.st_ino == stroot.st_ino) {
	    if (p != NULL)
		*p = '/';
	    if (q != NULL)
		*q = '/';
	    return len;
	}
	q = p;
    } while ((p = strrchr (repository, '/')) != NULL);
    return -1;
}

/* Return a newly malloc'd string containing the name of the lock for the
   repository REPOSITORY and the lock file name within that directory
   NAME.  Also create the directories in which to put the lock file
   if needed (if we need to, could save system call(s) by doing
   that only if the actual operation fails.  But for now we'll keep
   things simple).  */
static char *
lock_name (const char *repository, const char *name)
{
    char *retval;
    const char *p;
    char *q;
    const char *short_repos;
    mode_t save_umask = 0000;
    int saved_umask = 0, len;

    TRACE (TRACE_FLOW, "lock_name (%s, %s)",
	   repository  ? repository : "(null)", name ? name : "(null)");

    if (!config->lock_dir)
    {
	/* This is the easy case.  Because the lock files go directly
	   in the repository, no need to create directories or anything.  */
	assert (name != NULL);
	assert (repository != NULL);
	retval = Xasprintf ("%s/%s", repository, name);
    }
    else
    {
	struct stat sb;
	mode_t new_mode = 0;

	/* The interesting part of the repository is the part relative
	   to CVSROOT.  */
	assert (current_parsed_root != NULL);
	assert (current_parsed_root->directory != NULL);
	/* 
	 * Unfortunately, string comparisons are not enough because we
	 * might have symlinks present
	 */
	len = find_root(repository, current_parsed_root->directory);
	if (len == -1)
	    error (1, 0, "%s not found in %s",
		repository, current_parsed_root->directory);
	short_repos = repository + len + 1;

	if (strcmp (repository, current_parsed_root->directory) == 0)
	    short_repos = ".";
	else
	    assert (short_repos[-1] == '/');

	retval = xmalloc (strlen (config->lock_dir)
			  + strlen (short_repos)
			  + strlen (name)
			  + 10);
	strcpy (retval, config->lock_dir);
	q = retval + strlen (retval);
	*q++ = '/';

	strcpy (q, short_repos);

	/* In the common case, where the directory already exists, let's
	   keep it to one system call.  */
	if (stat (retval, &sb) < 0)
	{
	    /* If we need to be creating more than one directory, we'll
	       get the existence_error here.  */
	    if (!existence_error (errno))
		error (1, errno, "cannot stat directory %s", retval);
	}
	else
	{
	    if (S_ISDIR (sb.st_mode))
		goto created;
	    else
		error (1, 0, "%s is not a directory", retval);
	}

	/* Now add the directories one at a time, so we can create
	   them if needed.

	   The idea behind the new_mode stuff is that the directory we
	   end up creating will inherit permissions from its parent
	   directory (we re-set new_mode with each EEXIST).  CVSUMASK
	   isn't right, because typically the reason for LockDir is to
	   use a different set of permissions.  We probably want to
	   inherit group ownership also (but we don't try to deal with
	   that, some systems do it for us either always or when g+s is on).

	   We don't try to do anything about the permissions on the lock
	   files themselves.  The permissions don't really matter so much
	   because the locks will generally be removed by the process
	   which created them.  */

	if (stat (config->lock_dir, &sb) < 0)
	    error (1, errno, "cannot stat %s", config->lock_dir);
	new_mode = sb.st_mode;
	save_umask = umask (0000);
	saved_umask = 1;

	p = short_repos;
	while (1)
	{
	    while (!ISSLASH (*p) && *p != '\0')
		++p;
	    if (ISSLASH (*p))
	    {
		strncpy (q, short_repos, p - short_repos);
		q[p - short_repos] = '\0';
		if (!ISSLASH (q[p - short_repos - 1])
		    && CVS_MKDIR (retval, new_mode) < 0)
		{
		    int saved_errno = errno;
		    if (saved_errno != EEXIST)
			error (1, errno, "cannot make directory %s", retval);
		    else
		    {
			if (stat (retval, &sb) < 0)
			    error (1, errno, "cannot stat %s", retval);
			new_mode = sb.st_mode;
		    }
		}
		++p;
	    }
	    else
	    {
		strcpy (q, short_repos);
		if (CVS_MKDIR (retval, new_mode) < 0
		    && errno != EEXIST)
		    error (1, errno, "cannot make directory %s", retval);
		goto created;
	    }
	}
    created:;

	strcat (retval, "/");
	strcat (retval, name);

	if (saved_umask)
	{
	    assert (umask (save_umask) == 0000);
	    saved_umask = 0;
	}
    }
    return retval;
}



/* Remove the lock files.  For interrupt purposes, it can be assumed that the
 * first thing this function does is set lock->repository to NULL.
 *
 * INPUTS
 *   lock	The lock to remove.
 *   free	True if this lock directory will not be reused (free
 *		lock->repository if necessary).
 */
static void
remove_lock_files (struct lock *lock, bool free_repository)
{
    TRACE (TRACE_FLOW, "remove_lock_files (%s)", lock->repository);

    /* If lock->file is set, the lock *might* have been created, but since
     * Reader_Lock & lock_dir_for_write don't use SIG_beginCrSect the way that
     * set_lock does, we don't know that.  That is why we need to check for
     * existence_error here.
     */
    if (lock->file1)
    {
	char *tmp = lock->file1;
	lock->file1 = NULL;
	if (CVS_UNLINK (tmp) < 0 && ! existence_error (errno))
	    error (0, errno, "failed to remove lock %s", tmp);
	free (tmp);
    }
#ifdef LOCK_COMPATIBILITY
    if (lock->file2)
    {
	char *tmp = lock->file2;
	lock->file2 = NULL;
	if (CVS_UNLINK (tmp) < 0 && ! existence_error (errno))
	    error (0, errno, "failed to remove lock %s", tmp);
	free (tmp);
    }
#endif /* LOCK_COMPATIBILITY */

    clear_lock (lock);

    /* And free the repository string.  We don't really have to set the
     * repository string to NULL first since there is no harm in running any of
     * the above code twice.
     *
     * Use SIG_beginCrSect since otherwise we might be interrupted between
     * checking whether free_repository is set and freeing stuff.
     */
    if (free_repository)
    {
	SIG_beginCrSect ();
	if (lock->free_repository)
	{
	    free ((char *)lock->repository);
	    lock->free_repository = false;
	}
	lock->repository = NULL;
	SIG_endCrSect ();
    }
}



/*
 * Clean up outstanding read and write locks and free their storage.
 */
void
Simple_Lock_Cleanup (void)
{
    TRACE (TRACE_FUNCTION, "Simple_Lock_Cleanup()");

    /* Avoid interrupts while accessing globals the interrupt handlers might
     * make use of.
     */
    SIG_beginCrSect();

    /* clean up simple read locks (if any) */
    if (global_readlock.repository != NULL)
	remove_lock_files (&global_readlock, true);
    /* See note in Lock_Cleanup() below.  */
    SIG_endCrSect();

    SIG_beginCrSect();

    /* clean up simple write locks (if any) */
    if (global_writelock.repository != NULL)
	remove_lock_files (&global_writelock, true);
    /* See note in Lock_Cleanup() below.  */
    SIG_endCrSect();

    SIG_beginCrSect();

    /* clean up simple write locks (if any) */
    if (global_history_lock.repository)
	remove_lock_files (&global_history_lock, true);
    SIG_endCrSect();

    SIG_beginCrSect();

    if (global_val_tags_lock.repository)
	remove_lock_files (&global_val_tags_lock, true);
    /* See note in Lock_Cleanup() below.  */
    SIG_endCrSect();
}



/*
 * Clean up all outstanding locks and free their storage.
 *
 * NOTES
 *   This function needs to be reentrant since a call to exit() can cause a
 *   call to this function, which can then be interrupted by a signal, which
 *   can cause a second call to this function.
 *
 * RETURNS
 *   Nothing.
 */
void
Lock_Cleanup (void)
{
    TRACE (TRACE_FUNCTION, "Lock_Cleanup()");

    /* FIXME: Do not perform buffered I/O from an interrupt handler like
     * this (via error).  However, I'm leaving the error-calling code there
     * in the hope that on the rare occasion the error call is actually made
     * (e.g., a fluky I/O error or permissions problem prevents the deletion
     * of a just-created file) reentrancy won't be an issue.
     */

    remove_locks ();

    /* Avoid being interrupted during calls which set globals to NULL.  This
     * avoids having interrupt handlers attempt to use these global variables
     * in inconsistent states.
     *
     * This isn't always necessary, because sometimes we are called via exit()
     * or the interrupt handler, in which case signals will already be blocked,
     * but sometimes we might be called from elsewhere.
     */
    SIG_beginCrSect();
    dellist (&lock_tree_list);
    /*  Unblocking allows any signal to be processed as soon as possible.  This
     *  isn't really necessary, but since we know signals can cause us to be
     *  called, why not avoid having blocks of code run twice.
     */
    SIG_endCrSect();
}



/*
 * walklist proc for removing a list of locks
 */
static int
unlock_proc (Node *p, void *closure)
{
    remove_lock_files (p->data, false);
    return 0;
}



/*
 * Remove locks without discarding the lock information.
 */
static void
remove_locks (void)
{
    TRACE (TRACE_FLOW, "remove_locks()");

    Simple_Lock_Cleanup ();

    /* clean up promotable locks (if any) */
    SIG_beginCrSect();
    if (locklist != NULL)
    {
	/* Use a tmp var since any of these functions could call exit, causing
	 * us to be called a second time.
	 */
	List *tmp = locklist;
	locklist = NULL;
	walklist (tmp, unlock_proc, NULL);
    }
    SIG_endCrSect();
}



/*
 * Set the global readlock variable if it isn't already.
 */
static void
set_readlock_name (void)
{
    if (readlock == NULL)
    {
	readlock = Xasprintf (
#ifdef HAVE_LONG_FILE_NAMES
			      "%s.%s.%ld", CVSRFL, hostname,
#else
			      "%s.%ld", CVSRFL,
#endif
			      (long) getpid ());
    }
}



/*
 * Create a lock file for readers
 */
int
Reader_Lock (char *xrepository)
{
    int err = 0;
    FILE *fp;

    if (nolock)
	return (0);

    TRACE (TRACE_FUNCTION, "Reader_Lock(%s)", xrepository);

    if (noexec || readonlyfs)
	return 0;

    /* we only do one directory at a time for read locks!  */
    if (global_readlock.repository != NULL)
    {
	error (0, 0, "Reader_Lock called while read locks set - Help!");
	return 1;
    }

    set_readlock_name ();

    /* remember what we're locking (for Lock_Cleanup) */
    global_readlock.repository = xstrdup (xrepository);
    global_readlock.free_repository = true;

    /* get the lock dir for our own */
    if (set_lock (&global_readlock, 1) != L_OK)
    {
	error (0, 0, "failed to obtain dir lock in repository `%s'",
	       xrepository);
	if (readlock != NULL)
	    free (readlock);
	readlock = NULL;
	/* We don't set global_readlock.repository to NULL.  I think this
	   only works because recurse.c will give a fatal error if we return
	   a nonzero value.  */
	return 1;
    }

    /* write a read-lock */
    global_readlock.file1 = lock_name (xrepository, readlock);
    if ((fp = CVS_FOPEN (global_readlock.file1, "w+")) == NULL
	|| fclose (fp) == EOF)
    {
	error (0, errno, "cannot create read lock in repository `%s'",
	       xrepository);
	err = 1;
    }

    /* free the lock dir */
    clear_lock (&global_readlock);

    return err;
}



/*
 * lock_exists() returns 0 if there is no lock file matching FILEPAT in
 * the repository but not IGNORE; else 1 is returned, to indicate that the
 * caller should sleep a while and try again.
 *
 * INPUTS
 *   repository		The repository directory to search for locks.
 *   filepat		The file name pattern to search for.
 *   ignore		The name of a single file which can be ignored.
 *
 * GLOBALS
 *   lockdir		The lock dir external to the repository, if any.
 *
 * RETURNS
 *   0		No lock matching FILEPAT and not IGNORE exists.
 *   1		Otherwise and on error.
 *
 * ERRORS
 *  In the case where errors are encountered reading the directory, a warning
 *  message is printed, 1 is is returned and ERRNO is left set.
 */
static int
lock_exists (const char *repository, const char *filepat, const char *ignore)
{
    char *lockdir;
    char *line;
    DIR *dirp;
    struct dirent *dp;
    struct stat sb;
    int ret;
#ifdef CVS_FUDGELOCKS
    time_t now;
    (void)time (&now);
#endif

    TRACE (TRACE_FLOW, "lock_exists (%s, %s, %s)",
	   repository, filepat, ignore ? ignore : "(null)");

    lockdir = lock_name (repository, "");
    lockdir[strlen (lockdir) - 1] = '\0';   /* remove trailing slash */

    do {
	if ((dirp = CVS_OPENDIR (lockdir)) == NULL)
	    error (1, 0, "cannot open directory %s", lockdir);

	ret = 0;
	errno = 0;
	while ((dp = CVS_READDIR (dirp)) != NULL)
	{
	    if (CVS_FNMATCH (filepat, dp->d_name, 0) == 0)
	    {
		/* FIXME: the basename conversion below should be replaced with
		 * a call to the GNULIB basename function once it is imported.
		 */
		/* ignore our plock, if any */
		if (ignore && !fncmp (ignore, dp->d_name))
		    continue;

		line = Xasprintf ("%s/%s", lockdir, dp->d_name);
		if (stat (line, &sb) != -1)
		{
#ifdef CVS_FUDGELOCKS
		    /*
		     * If the create time of the file is more than CVSLCKAGE 
		     * seconds ago, try to clean-up the lock file, and if
		     * successful, re-open the directory and try again.
		     */
		    if (now >= (sb.st_ctime + CVSLCKAGE) &&
                        CVS_UNLINK (line) != -1)
		    {
			free (line);
			ret = -1;
			break;
		    }
#endif
		    set_lockers_name (&sb);
		}
		else
		{
		    /* If the file doesn't exist, it just means that it
		     * disappeared between the time we did the readdir and the
		     * time we did the stat.
		     */
		    if (!existence_error (errno))
			error (0, errno, "cannot stat %s", line);
		}
		errno = 0;
		free (line);
		ret = 1;
		break;
	    }
	    errno = 0;
	}
	if (errno != 0)
	    error (0, errno, "error reading directory %s", repository);

	CVS_CLOSEDIR (dirp);
    } while (ret < 0);

    if (lockdir != NULL)
	free (lockdir);
    return ret;
}



/*
 * readers_exist() returns 0 if there are no reader lock files remaining in
 * the repository; else 1 is returned, to indicate that the caller should
 * sleep a while and try again.
 *
 * See lock_exists() for argument detail.
 */
static int
readers_exist (const char *repository)
{
    TRACE (TRACE_FLOW, "readers_exist (%s)", repository);

    /* It is only safe to ignore a readlock set by our process if it was set as
     * a safety measure to prevent older CVS processes from ignoring our
     * promotable locks.  The code to ignore these readlocks can be removed
     * once it is deemed unlikely that anyone will be using CVS servers earlier
     * than version 1.12.4.
     */
    return lock_exists (repository, CVSRFLPAT,
#ifdef LOCK_COMPATIBILITY
                         findnode (locklist, repository) ? readlock : 
#endif /* LOCK_COMPATIBILITY */
			 NULL);
}



/*
 * promotable_exists() returns 0 if there is no promotable lock file in
 * the repository; else 1 is returned, to indicate that the caller should
 * sleep a while and try again.
 *
 * See lock_exists() for argument detail.
 */
static int
promotable_exists (const char *repository)
{
    TRACE (TRACE_FLOW, "promotable_exists (%s)", repository);
    return lock_exists (repository, CVSPFLPAT, promotablelock);
}



/*
 * Lock a list of directories for writing
 */
static char *lock_error_repos;
static int lock_error;



/*
 * Create a lock file for potential writers returns L_OK if lock set ok,
 * L_LOCKED if lock held by someone else or L_ERROR if an error occurred.
 */
static int
set_promotable_lock (struct lock *lock)
{
    int status;
    FILE *fp;

    TRACE (TRACE_FUNCTION, "set_promotable_lock(%s)",
	   lock->repository ? lock->repository : "(null)");

    if (promotablelock == NULL)
    {
	promotablelock = Xasprintf (
#ifdef HAVE_LONG_FILE_NAMES
				    "%s.%s.%ld", CVSPFL, hostname,
#else
				    "%s.%ld", CVSPFL,
#endif
				    (long) getpid());
    }

    /* make sure the lock dir is ours (not necessarily unique to us!) */
    status = set_lock (lock, 0);
    if (status == L_OK)
    {
	/* we now own a promotable lock - make sure there are no others */
	if (promotable_exists (lock->repository))
	{
	    /* clean up the lock dir */
	    clear_lock (lock);

	    /* indicate we failed due to read locks instead of error */
	    return L_LOCKED;
	}

	/* write the promotable-lock file */
	lock->file1 = lock_name (lock->repository, promotablelock);
	if ((fp = CVS_FOPEN (lock->file1, "w+")) == NULL || fclose (fp) == EOF)
	{
	    int xerrno = errno;

	    if (CVS_UNLINK (lock->file1) < 0 && ! existence_error (errno))
		error (0, errno, "failed to remove lock %s", lock->file1);

	    /* free the lock dir */
	    clear_lock (lock);

	    /* return the error */
	    error (0, xerrno,
		   "cannot create promotable lock in repository `%s'",
		   lock->repository);
	    return L_ERROR;
	}

#ifdef LOCK_COMPATIBILITY
	/* write the read-lock file.  We only do this so that older versions of
	 * CVS will not think it is okay to create a write lock.  When it is
	 * decided that versions of CVS earlier than 1.12.4 are not likely to
	 * be used, this code can be removed.
	 */
	set_readlock_name ();
	lock->file2 = lock_name (lock->repository, readlock);
	if ((fp = CVS_FOPEN (lock->file2, "w+")) == NULL || fclose (fp) == EOF)
	{
	    int xerrno = errno;

	    if ( CVS_UNLINK (lock->file2) < 0 && ! existence_error (errno))
		error (0, errno, "failed to remove lock %s", lock->file2);

	    /* free the lock dir */
	    clear_lock (lock);

	    /* Remove the promotable lock.  */
	    lock->file2 = NULL;
	    remove_lock_files (lock, false);

	    /* return the error */
	    error (0, xerrno,
		   "cannot create read lock in repository `%s'",
		   lock->repository);
	    return L_ERROR;
	}
#endif /* LOCK_COMPATIBILITY */

	clear_lock (lock);

	return L_OK;
    }
    else
	return status;
}



/*
 * walklist proc for setting write locks.  Mostly just a wrapper for the
 * set_promotable_lock function, which has a prettier API, but no other good
 * reason for existing separately.
 *
 * INPUTS
 *   p		The current node, as determined by walklist().
 *   closure	Not used.
 *
 * GLOBAL INPUTS
 *   lock_error		Any previous error encountered while attempting to get
 *                      a lock.
 *
 * GLOBAL OUTPUTS
 *   lock_error		Set if we encounter an error attempting to get axi
 *			promotable lock.
 *   lock_error_repos	Set so that if we set lock_error later functions will
 *			be able to report where the other process's lock was
 *			encountered.
 *
 * RETURNS
 *   0 for no error.
 */
static int
set_promotablelock_proc (Node *p, void *closure)
{
    /* if some lock was not OK, just skip this one */
    if (lock_error != L_OK)
	return 0;

    /* apply the write lock */
    lock_error_repos = p->key;
    lock_error = set_promotable_lock ((struct lock *)p->data);
    return 0;
}



/*
 * Print out a message that the lock is still held, then sleep a while.
 */
static void
lock_wait (const char *repos)
{
    time_t now;
    char *msg;
    struct tm *tm_p;

    (void) time (&now);
    tm_p = gmtime (&now);
    msg = Xasprintf ("[%8.8s] waiting for %s's lock in %s",
		     (tm_p ? asctime (tm_p) : ctime (&now)) + 11,
		     lockers_name, repos);
    error (0, 0, "%s", msg);
    /* Call cvs_flusherr to ensure that the user sees this message as
       soon as possible.  */
    cvs_flusherr ();
    free (msg);
    (void)sleep (CVSLCKSLEEP);
}



/*
 * Print out a message when we obtain a lock.
 */
static void
lock_obtained (const char *repos)
{
    time_t now;
    char *msg;
    struct tm *tm_p;

    (void) time (&now);
    tm_p = gmtime (&now);
    msg = Xasprintf ("[%8.8s] obtained lock in %s",
		     (tm_p ? asctime (tm_p) : ctime (&now)) + 11, repos);
    error (0, 0, "%s", msg);
    /* Call cvs_flusherr to ensure that the user sees this message as
       soon as possible.  */
    cvs_flusherr ();
    free (msg);
}



static int
lock_list_promotably (List *list)
{
    char *wait_repos;

    TRACE (TRACE_FLOW, "lock_list_promotably ()");

    if (nolock)
	return (0);
    if (noexec)
	return 0;

    if (readonlyfs) {
	error (0, 0,
	       "promotable lock failed.\n\
WARNING: Read-only repository access mode selected via `cvs -R'.\n\
Attempting to write to a read-only filesystem is not allowed.");
	return 1;
    }

    /* We only know how to do one list at a time */
    if (locklist != NULL)
    {
	error (0, 0,
	       "lock_list_promotably called while promotable locks set - Help!");
	return 1;
    }

    wait_repos = NULL;
    for (;;)
    {
	/* try to lock everything on the list */
	lock_error = L_OK;		/* init for set_promotablelock_proc */
	lock_error_repos = NULL;	/* init for set_promotablelock_proc */
	locklist = list;		/* init for Lock_Cleanup */
	if (lockers_name != NULL)
	    free (lockers_name);
	lockers_name = xstrdup ("unknown");

	(void) walklist (list, set_promotablelock_proc, NULL);

	switch (lock_error)
	{
	    case L_ERROR:		/* Real Error */
		if (wait_repos != NULL)
		    free (wait_repos);
		Lock_Cleanup ();	/* clean up any locks we set */
		error (0, 0, "lock failed - giving up");
		return 1;

	    case L_LOCKED:		/* Someone already had a lock */
		remove_locks ();	/* clean up any locks we set */
		lock_wait (lock_error_repos); /* sleep a while and try again */
		wait_repos = xstrdup (lock_error_repos);
		continue;

	    case L_OK:			/* we got the locks set */
	        if (wait_repos != NULL)
		{
		    lock_obtained (wait_repos);
		    free (wait_repos);
		}
		return 0;

	    default:
		if (wait_repos != NULL)
		    free (wait_repos);
		error (0, 0, "unknown lock status %d in lock_list_promotably",
		       lock_error);
		return 1;
	}
    }
}



/*
 * Set the static variable lockers_name appropriately, based on the stat
 * structure passed in.
 */
static void
set_lockers_name (struct stat *statp)
{
    struct passwd *pw;

    if (lockers_name != NULL)
	free (lockers_name);
    pw = (struct passwd *) getpwuid (statp->st_uid);
    if (pw != NULL)
	lockers_name = xstrdup (pw->pw_name);
    else
	lockers_name = Xasprintf ("uid%lu", (unsigned long) statp->st_uid);
}



/*
 * Persistently tries to make the directory "lckdir", which serves as a
 * lock.
 *
 * #ifdef CVS_FUDGELOCKS
 * If the create time on the directory is greater than CVSLCKAGE
 * seconds old, just try to remove the directory.
 * #endif
 *
 */
static int
set_lock (struct lock *lock, int will_wait)
{
    int waited;
    long us;
    struct stat sb;
    mode_t omask;
    char *masterlock;
    int status;
#ifdef CVS_FUDGELOCKS
    time_t now;
#endif

    TRACE (TRACE_FLOW, "set_lock (%s, %d)",
	   lock->repository ? lock->repository : "(null)", will_wait);

    masterlock = lock_name (lock->repository, lock->lockdirname);

    /*
     * Note that it is up to the callers of set_lock() to arrange for signal
     * handlers that do the appropriate things, like remove the lock
     * directory before they exit.
     */
    waited = 0;
    us = 1;
    for (;;)
    {
	status = -1;
	omask = umask (cvsumask);
	SIG_beginCrSect ();
	if (CVS_MKDIR (masterlock, 0777) == 0)
	{
	    lock->lockdir = masterlock;
	    SIG_endCrSect ();
	    status = L_OK;
	    if (waited)
	        lock_obtained (lock->repository);
	    goto after_sig_unblock;
	}
	SIG_endCrSect ();
    after_sig_unblock:
	(void) umask (omask);
	if (status != -1)
	    goto done;

	if (errno != EEXIST)
	{
	    error (0, errno,
		   "failed to create lock directory for `%s' (%s)",
		   lock->repository, masterlock);
	    status = L_ERROR;
	    goto done;
	}

	/* Find out who owns the lock.  If the lock directory is
	   non-existent, re-try the loop since someone probably just
	   removed it (thus releasing the lock).  */
	if (stat (masterlock, &sb) < 0)
	{
	    if (existence_error (errno))
		continue;

	    error (0, errno, "couldn't stat lock directory `%s'", masterlock);
	    status = L_ERROR;
	    goto done;
	}

#ifdef CVS_FUDGELOCKS
	/*
	 * If the create time of the directory is more than CVSLCKAGE seconds
	 * ago, try to clean-up the lock directory, and if successful, just
	 * quietly retry to make it.
	 */
	(void) time (&now);
	if (now >= (sb.st_ctime + CVSLCKAGE))
	{
	    if (CVS_RMDIR (masterlock) >= 0)
		continue;
	}
#endif

	/* set the lockers name */
	set_lockers_name (&sb);

	/* if he wasn't willing to wait, return an error */
	if (!will_wait)
	{
	    status = L_LOCKED;
	    goto done;
	}

	/* if possible, try a very short sleep without a message */
	if (!waited && us < 1000)
	{
	    us += us;
	    {
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = us * 1000;
		(void)nanosleep (&ts, NULL);
		continue;
	    }
	}

	lock_wait (lock->repository);
	waited = 1;
    }

done:
    if (!lock->lockdir)
	free (masterlock);
    return status;
}



/*
 * Clear master lock.
 *
 * INPUTS
 *   lock	The lock information.
 *
 * OUTPUTS
 *   Sets LOCK->lockdir to NULL after removing the directory it names and
 *   freeing the storage.
 *
 * ASSUMPTIONS
 *   If we own the master lock directory, its name is stored in LOCK->lockdir.
 *   We may free LOCK->lockdir.
 */
static void
clear_lock (struct lock *lock)
{
    SIG_beginCrSect ();
    if (lock->lockdir)
    {
	if (CVS_RMDIR (lock->lockdir) < 0)
	    error (0, errno, "failed to remove lock dir `%s'", lock->lockdir);
	free (lock->lockdir);
	lock->lockdir = NULL;
    }
    SIG_endCrSect ();
}



/*
 * Create a list of repositories to lock
 */
/* ARGSUSED */
static int
lock_filesdoneproc (void *callerdat, int err, const char *repository,
                    const char *update_dir, List *entries)
{
    Node *p;

    p = getnode ();
    p->type = LOCK;
    p->key = xstrdup (repository);
    p->data = xmalloc (sizeof (struct lock));
    ((struct lock *)p->data)->repository = p->key;
    ((struct lock *)p->data)->file1 = NULL;
#ifdef LOCK_COMPATIBILITY
    ((struct lock *)p->data)->file2 = NULL;
#endif /* LOCK_COMPATIBILITY */
    ((struct lock *)p->data)->lockdirname = CVSLCK;
    ((struct lock *)p->data)->lockdir = NULL;
    ((struct lock *)p->data)->free_repository = false;

    /* FIXME-KRP: this error condition should not simply be passed by. */
    if (p->key == NULL || addnode (lock_tree_list, p) != 0)
	freenode (p);
    return err;
}



void
lock_tree_promotably (int argc, char **argv, int local, int which, int aflag)
{
    TRACE (TRACE_FUNCTION, "lock_tree_promotably (%d, argv, %d, %d, %d)",
	   argc, local, which, aflag);

    /*
     * Run the recursion processor to find all the dirs to lock and lock all
     * the dirs
     */
    lock_tree_list = getlist ();
    start_recursion
	(NULL, lock_filesdoneproc,
	 NULL, NULL, NULL, argc,
	 argv, local, which, aflag, CVS_LOCK_NONE,
	 NULL, 0, NULL );
    sortlist (lock_tree_list, fsortcmp);
    if (lock_list_promotably (lock_tree_list) != 0)
	error (1, 0, "lock failed - giving up");
}



/* Lock a single directory in REPOSITORY.  It is OK to call this if
 * a lock has been set with lock_dir_for_write; the new lock will replace
 * the old one.  If REPOSITORY is NULL, don't do anything.
 *
 * We do not clear the dir lock after writing the lock file name since write
 * locks are exclusive to all other locks.
 */
void
lock_dir_for_write (const char *repository)
{
    int waiting = 0;

    TRACE (TRACE_FLOW, "lock_dir_for_write (%s)", repository);

    if (repository != NULL
	&& (global_writelock.repository == NULL
	    || !strcmp (global_writelock.repository, repository)))
    {
	if (writelock == NULL)
	{
	    writelock = Xasprintf (
#ifdef HAVE_LONG_FILE_NAMES
				   "%s.%s.%ld", CVSWFL, hostname,
#else
				   "%s.%ld", CVSWFL,
#endif
				   (long) getpid());
	}

	if (global_writelock.repository != NULL)
	    remove_lock_files (&global_writelock, true);

	global_writelock.repository = xstrdup (repository);
	global_writelock.free_repository = true;

	for (;;)
	{
	    FILE *fp;

	    if (set_lock (&global_writelock, 1) != L_OK)
		error (1, 0, "failed to obtain write lock in repository `%s'",
		       repository);

	    /* check if readers exist */
	    if (readers_exist (repository)
		|| promotable_exists (repository))
	    {
		clear_lock (&global_writelock);
		lock_wait (repository); /* sleep a while and try again */
		waiting = 1;
		continue;
	    }

	    if (waiting)
		lock_obtained (repository);

	    /* write the write-lock file */
	    global_writelock.file1 = lock_name (global_writelock.repository,
	                                        writelock);
	    if ((fp = CVS_FOPEN (global_writelock.file1, "w+")) == NULL
		|| fclose (fp) == EOF)
	    {
		int xerrno = errno;

		if (CVS_UNLINK (global_writelock.file1) < 0
		    && !existence_error (errno))
		{
		    error (0, errno, "failed to remove write lock %s",
			   global_writelock.file1);
		}

		/* free the lock dir */
		clear_lock (&global_writelock);

		/* return the error */
		error (1, xerrno,
		       "cannot create write lock in repository `%s'",
		       global_writelock.repository);
	    }

	    /* If we upgraded from a promotable lock, remove it. */
	    if (locklist)
	    {
		Node *p = findnode (locklist, repository);
		if (p)
		{
		    remove_lock_files (p->data, true);
		    delnode (p);
		}
	    }

	    break;
	}
    }
}



/* This is the internal implementation behind history_lock & val_tags_lock.  It
 * gets a write lock for the history or val-tags file.
 *
 * RETURNS
 *   true, on success
 *   false, on error
 */
static inline int
internal_lock (struct lock *lock, const char *xrepository)
{
    /* remember what we're locking (for Lock_Cleanup) */
    assert (!lock->repository);
    lock->repository = Xasprintf ("%s/%s", xrepository, CVSROOTADM);
    lock->free_repository = true;

    /* get the lock dir for our own */
    if (set_lock (lock, 1) != L_OK)
    {
	if (!really_quiet)
	    error (0, 0, "failed to obtain history lock in repository `%s'",
		   xrepository);

	return 0;
    }

    return 1;
}



/* Lock the CVSROOT/history file for write.
 */
int
history_lock (const char *xrepository)
{
    return internal_lock (&global_history_lock, xrepository);
}



/* Remove the CVSROOT/history lock, if it exists.
 */
void
clear_history_lock ()
{
    remove_lock_files (&global_history_lock, true);
}



/* Lock the CVSROOT/val-tags file for write.
 */
int
val_tags_lock (const char *xrepository)
{
    return internal_lock (&global_val_tags_lock, xrepository);
}



/* Remove the CVSROOT/val-tags lock, if it exists.
 */
void
clear_val_tags_lock ()
{
    remove_lock_files (&global_val_tags_lock, true);
}
