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
 * Commit Files
 *
 * "commit" commits the present version to the RCS repository, AFTER
 * having done a test on conflicts.
 *
 * The call is: cvs commit [options] files...
 *
 */

#include "cvs.h"
#include "getline.h"
#include "edit.h"
#include "fileattr.h"
#include "hardlink.h"

static Dtype check_direntproc (void *callerdat, const char *dir,
                               const char *repos, const char *update_dir,
                               List *entries);
static int check_fileproc (void *callerdat, struct file_info *finfo);
static int check_filesdoneproc (void *callerdat, int err, const char *repos,
				const char *update_dir, List *entries);
static int checkaddfile (const char *file, const char *repository,
                         const char *tag, const char *options,
                         RCSNode **rcsnode);
static Dtype commit_direntproc (void *callerdat, const char *dir,
                                const char *repos, const char *update_dir,
                                List *entries);
static int commit_dirleaveproc (void *callerdat, const char *dir, int err,
				const char *update_dir, List *entries);
static int commit_fileproc (void *callerdat, struct file_info *finfo);
static int commit_filesdoneproc (void *callerdat, int err,
                                 const char *repository,
				 const char *update_dir, List *entries);
static int finaladd (struct file_info *finfo, char *revision, char *tag,
		     char *options);
static int findmaxrev (Node * p, void *closure);
static int lock_RCS (const char *user, RCSNode *rcs, const char *rev,
                     const char *repository);
static int precommit_list_to_args_proc (Node * p, void *closure);
static int precommit_proc (const char *repository, const char *filter,
                           void *closure);
static int remove_file (struct file_info *finfo, char *tag,
			char *message);
static void fixaddfile (const char *rcs);
static void fixbranch (RCSNode *, char *branch);
static void unlockrcs (RCSNode *rcs);
static void ci_delproc (Node *p);
static void masterlist_delproc (Node *p);

struct commit_info
{
    Ctype status;			/* as returned from Classify_File() */
    char *rev;				/* a numeric rev, if we know it */
    char *tag;				/* any sticky tag, or -r option */
    char *options;			/* Any sticky -k option */
};
struct master_lists
{
    List *ulist;			/* list for Update_Logfile */
    List *cilist;			/* list with commit_info structs */
};

static int check_valid_edit = 0;
static int force_ci = 0;
static int got_message;
static int aflag;
static char *saved_tag;
static char *write_dirtag;
static int write_dirnonbranch;
static char *logfile;
static List *mulist;
static char *saved_message;
static time_t last_register_time;

static const char *const commit_usage[] =
{
    "Usage: %s %s [-cRlf] [-m msg | -F logfile] [-r rev] files...\n",
    "    -c          Check for valid edits before committing.\n",
    "    -R          Process directories recursively.\n",
    "    -l          Local directory only (not recursive).\n",
    "    -f          Force the file to be committed; disables recursion.\n",
    "    -F logfile  Read the log message from file.\n",
    "    -m msg      Log message.\n",
    "    -r rev      Commit to this branch or trunk revision.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

#ifdef CLIENT_SUPPORT
/* Identify a file which needs "? foo" or a Questionable request.  */
struct question
{
    /* The two fields for the Directory request.  */
    char *dir;
    char *repos;

    /* The file name.  */
    char *file;

    struct question *next;
};

struct find_data
{
    List *ulist;
    int argc;
    char **argv;

    /* This is used from dirent to filesdone time, for each directory,
       to make a list of files we have already seen.  */
    List *ignlist;

    /* Linked list of files which need "? foo" or a Questionable request.  */
    struct question *questionables;

    /* Only good within functions called from the filesdoneproc.  Stores
       the repository (pointer into storage managed by the recursion
       processor.  */
    const char *repository;

    /* Non-zero if we should force the commit.  This is enabled by
       either -f or -r options, unlike force_ci which is just -f.  */
    int force;
};



static Dtype
find_dirent_proc (void *callerdat, const char *dir, const char *repository,
                  const char *update_dir, List *entries)
{
    struct find_data *find_data = callerdat;

    /* This check seems to slowly be creeping throughout CVS (update
       and send_dirent_proc by CVS 1.5, diff in 31 Oct 1995.  My guess
       is that it (or some variant thereof) should go in all the
       dirent procs.  Unless someone has some better idea...  */
    if (!isdir (dir))
	return R_SKIP_ALL;

    /* initialize the ignore list for this directory */
    find_data->ignlist = getlist ();

    /* Print the same warm fuzzy as in check_direntproc, since that
       code will never be run during client/server operation and we
       want the messages to match. */
    if (!quiet)
	error (0, 0, "Examining %s", update_dir);

    return R_PROCESS;
}



/* Here as a static until we get around to fixing ignore_files to pass
   it along as an argument.  */
static struct find_data *find_data_static;



static void
find_ignproc (const char *file, const char *dir)
{
    struct question *p;

    p = xmalloc (sizeof (struct question));
    p->dir = xstrdup (dir);
    p->repos = xstrdup (find_data_static->repository);
    p->file = xstrdup (file);
    p->next = find_data_static->questionables;
    find_data_static->questionables = p;
}



static int
find_filesdoneproc (void *callerdat, int err, const char *repository,
                    const char *update_dir, List *entries)
{
    struct find_data *find_data = callerdat;
    find_data->repository = repository;

    /* if this directory has an ignore list, process it then free it */
    if (find_data->ignlist)
    {
	find_data_static = find_data;
	ignore_files (find_data->ignlist, entries, update_dir, find_ignproc);
	dellist (&find_data->ignlist);
    }

    find_data->repository = NULL;

    return err;
}



/* Machinery to find out what is modified, added, and removed.  It is
   possible this should be broken out into a new client_classify function;
   merging it with classify_file is almost sure to be a mess, though,
   because classify_file has all kinds of repository processing.  */
static int
find_fileproc (void *callerdat, struct file_info *finfo)
{
    Vers_TS *vers;
    enum classify_type status;
    Node *node;
    struct find_data *args = callerdat;
    struct logfile_info *data;
    struct file_info xfinfo;

    /* if this directory has an ignore list, add this file to it */
    if (args->ignlist)
    {
	Node *p;

	p = getnode ();
	p->type = FILES;
	p->key = xstrdup (finfo->file);
	if (addnode (args->ignlist, p) != 0)
	    freenode (p);
    }

    xfinfo = *finfo;
    xfinfo.repository = NULL;
    xfinfo.rcs = NULL;

    vers = Version_TS (&xfinfo, NULL, saved_tag, NULL, 0, 0);
    if (vers->vn_user == NULL)
    {
	if (vers->ts_user == NULL)
	    error (0, 0, "nothing known about `%s'", finfo->fullname);
	else
	    error (0, 0, "use `%s add' to create an entry for `%s'",
		   program_name, finfo->fullname);
	freevers_ts (&vers);
	return 1;
    }
    if (vers->vn_user[0] == '-')
    {
	if (vers->ts_user != NULL)
	{
	    error (0, 0,
		   "`%s' should be removed and is still there (or is back"
		   " again)", finfo->fullname);
	    freevers_ts (&vers);
	    return 1;
	}
	/* else */
	status = T_REMOVED;
    }
    else if (strcmp (vers->vn_user, "0") == 0)
    {
	if (vers->ts_user == NULL)
	{
	    /* This happens when one has `cvs add'ed a file, but it no
	       longer exists in the working directory at commit time.
	       FIXME: What classify_file does in this case is print
	       "new-born %s has disappeared" and removes the entry.
	       We probably should do the same.  */
	    if (!really_quiet)
		error (0, 0, "warning: new-born %s has disappeared",
		       finfo->fullname);
	    status = T_REMOVE_ENTRY;
	}
	else
	    status = T_ADDED;
    }
    else if (vers->ts_user == NULL)
    {
	/* FIXME: What classify_file does in this case is print
	   "%s was lost".  We probably should do the same.  */
	freevers_ts (&vers);
	return 0;
    }
    else if (vers->ts_rcs != NULL
	     && (args->force || strcmp (vers->ts_user, vers->ts_rcs) != 0))
	/* If we are forcing commits, pretend that the file is
           modified.  */
	status = T_MODIFIED;
    else
    {
	/* This covers unmodified files, as well as a variety of other
	   cases.  FIXME: we probably should be printing a message and
	   returning 1 for many of those cases (but I'm not sure
	   exactly which ones).  */
	freevers_ts (&vers);
	return 0;
    }

    node = getnode ();
    node->key = xstrdup (finfo->fullname);

    data = xmalloc (sizeof (struct logfile_info));
    data->type = status;
    data->tag = xstrdup (vers->tag);
    data->rev_old = data->rev_new = NULL;

    node->type = UPDATE;
    node->delproc = update_delproc;
    node->data = data;
    (void)addnode (args->ulist, node);

    ++args->argc;

    freevers_ts (&vers);
    return 0;
}



static int
copy_ulist (Node *node, void *data)
{
    struct find_data *args = data;
    args->argv[args->argc++] = node->key;
    return 0;
}
#endif /* CLIENT_SUPPORT */



#ifdef SERVER_SUPPORT
# define COMMIT_OPTIONS "+cnlRm:fF:r:"
#else /* !SERVER_SUPPORT */
# define COMMIT_OPTIONS "+clRm:fF:r:"
#endif /* SERVER_SUPPORT */
int
commit (int argc, char **argv)
{
    int c;
    int err = 0;
    int local = 0;

    if (argc == -1)
	usage (commit_usage);

#ifdef CVS_BADROOT
    /*
     * For log purposes, do not allow "root" to commit files.  If you look
     * like root, but are really logged in as a non-root user, it's OK.
     */
    /* FIXME: Shouldn't this check be much more closely related to the
       readonly user stuff (CVSROOT/readers, &c).  That is, why should
       root be able to "cvs init", "cvs import", &c, but not "cvs ci"?  */
    /* Who we are on the client side doesn't affect logging.  */
    if (geteuid () == (uid_t) 0 && !current_parsed_root->isremote)
    {
	struct passwd *pw;

	if ((pw = getpwnam (getcaller ())) == NULL)
	    error (1, 0,
                   "your apparent username (%s) is unknown to this system",
                   getcaller ());
	if (pw->pw_uid == (uid_t) 0)
	    error (1, 0, "'root' is not allowed to commit files");
    }
#endif /* CVS_BADROOT */

    getoptreset ();
    while ((c = getopt (argc, argv, COMMIT_OPTIONS)) != -1)
    {
	switch (c)
	{
            case 'c':
                check_valid_edit = 1;
                break;
#ifdef SERVER_SUPPORT
	    case 'n':
		/* Silently ignore -n for compatibility with old
		 * clients.
		 */
		break;
#endif /* SERVER_SUPPORT */
	    case 'm':
#ifdef FORCE_USE_EDITOR
		use_editor = 1;
#else
		use_editor = 0;
#endif
		if (saved_message)
		{
		    free (saved_message);
		    saved_message = NULL;
		}

		saved_message = xstrdup (optarg);
		break;
	    case 'r':
		if (saved_tag)
		    free (saved_tag);
		saved_tag = xstrdup (optarg);
		break;
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case 'f':
		force_ci = 1;
                check_valid_edit = 0;
		local = 1;		/* also disable recursion */
		break;
	    case 'F':
#ifdef FORCE_USE_EDITOR
		use_editor = 1;
#else
		use_editor = 0;
#endif
		logfile = optarg;
		break;
	    case '?':
	    default:
		usage (commit_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    /* numeric specified revision means we ignore sticky tags... */
    if (saved_tag && isdigit ((unsigned char) *saved_tag))
    {
	char *p = saved_tag + strlen (saved_tag);
	aflag = 1;
	/* strip trailing dots and leading zeros */
	while (*--p == '.') ;
	p[1] = '\0';
	while (saved_tag[0] == '0' && isdigit ((unsigned char) saved_tag[1]))
	    ++saved_tag;
    }

    /* some checks related to the "-F logfile" option */
    if (logfile)
    {
	size_t size = 0, len;

	if (saved_message)
	    error (1, 0, "cannot specify both a message and a log file");

	get_file (logfile, logfile, "r", &saved_message, &size, &len);
    }

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	struct find_data find_args;

	ign_setup ();

	find_args.ulist = getlist ();
	find_args.argc = 0;
	find_args.questionables = NULL;
	find_args.ignlist = NULL;
	find_args.repository = NULL;

	/* It is possible that only a numeric tag should set this.
	   I haven't really thought about it much.
	   Anyway, I suspect that setting it unnecessarily only causes
	   a little unneeded network traffic.  */
	find_args.force = force_ci || saved_tag != NULL;

	err = start_recursion
	    (find_fileproc, find_filesdoneproc, find_dirent_proc, NULL,
	     &find_args, argc, argv, local, W_LOCAL, 0, CVS_LOCK_NONE,
	     NULL, 0, NULL );
	if (err)
	    error (1, 0, "correct above errors first!");

	if (find_args.argc == 0)
	{
	    /* Nothing to commit.  Exit now without contacting the
	       server (note that this means that we won't print "?
	       foo" for files which merit it, because we don't know
	       what is in the CVSROOT/cvsignore file).  */
	    dellist (&find_args.ulist);
	    return 0;
	}

	/* Now we keep track of which files we actually are going to
	   operate on, and only work with those files in the future.
	   This saves time--we don't want to search the file system
	   of the working directory twice.  */
	if (size_overflow_p (xtimes (find_args.argc, sizeof (char **))))
	{
	    find_args.argc = 0;
	    return 0;
	}
	find_args.argv = xnmalloc (find_args.argc, sizeof (char **));
	find_args.argc = 0;
	walklist (find_args.ulist, copy_ulist, &find_args);

	/* Do this before calling do_editor; don't ask for a log
	   message if we can't talk to the server.  But do it after we
	   have made the checks that we can locally (to more quickly
	   catch syntax errors, the case where no files are modified,
	   added or removed, etc.).

	   On the other hand, calling start_server before do_editor
	   means that we chew up server resources the whole time that
	   the user has the editor open (hours or days if the user
	   forgets about it), which seems dubious.  */
	start_server ();

	/*
	 * We do this once, not once for each directory as in normal CVS.
	 * The protocol is designed this way.  This is a feature.
	 */
	if (use_editor)
	    do_editor (".", &saved_message, NULL, find_args.ulist);

	/* We always send some sort of message, even if empty.  */
	option_with_arg ("-m", saved_message ? saved_message : "");

	/* OK, now process all the questionable files we have been saving
	   up.  */
	{
	    struct question *p;
	    struct question *q;

	    p = find_args.questionables;
	    while (p != NULL)
	    {
		if (ign_inhibit_server || !supported_request ("Questionable"))
		{
		    cvs_output ("? ", 2);
		    if (p->dir[0] != '\0')
		    {
			cvs_output (p->dir, 0);
			cvs_output ("/", 1);
		    }
		    cvs_output (p->file, 0);
		    cvs_output ("\n", 1);
		}
		else
		{
		    /* This used to send the Directory line of its own accord,
		     * but skipped some of the other processing like checking
		     * for whether the server would accept "Relative-directory"
		     * requests.  Relying on send_a_repository() to do this
		     * picks up these checks but also:
		     *
		     *   1. Causes the "Directory" request to be sent only once
		     *      per directory.
		     *   2. Causes the global TOPLEVEL_REPOS to be set.
		     *   3. Causes "Static-directory" and "Sticky" requests
		     *      to sometimes be sent.
		     *
		     * (1) is almost certainly a plus.  (2) & (3) may or may
		     * not be useful sometimes, and will ocassionally cause a
		     * little extra network traffic.  The additional network
		     * traffic is probably already saved several times over and
		     * certainly cancelled out via the multiple "Directory"
		     * request suppression of (1).
		     */
		    send_a_repository (p->dir, p->repos, p->dir);

		    send_to_server ("Questionable ", 0);
		    send_to_server (p->file, 0);
		    send_to_server ("\012", 1);
		}
		free (p->dir);
		free (p->repos);
		free (p->file);
		q = p->next;
		free (p);
		p = q;
	    }
	}

	if (local)
	    send_arg ("-l");
        if (check_valid_edit)
            send_arg ("-c");
	if (force_ci)
	    send_arg ("-f");
	option_with_arg ("-r", saved_tag);
	send_arg ("--");

	/* FIXME: This whole find_args.force/SEND_FORCE business is a
	   kludge.  It would seem to be a server bug that we have to
	   say that files are modified when they are not.  This makes
	   "cvs commit -r 2" across a whole bunch of files a very slow
	   operation (and it isn't documented in cvsclient.texi).  I
	   haven't looked at the server code carefully enough to be
	   _sure_ why this is needed, but if it is because the "ci"
	   program, which we used to call, wanted the file to exist,
	   then it would be relatively simple to fix in the server.  */
	send_files (find_args.argc, find_args.argv, local, 0,
		    find_args.force ? SEND_FORCE : 0);

	/* Sending only the names of the files which were modified, added,
	   or removed means that the server will only do an up-to-date
	   check on those files.  This is different from local CVS and
	   previous versions of client/server CVS, but it probably is a Good
	   Thing, or at least Not Such A Bad Thing.  */
	send_file_names (find_args.argc, find_args.argv, 0);
	free (find_args.argv);
	dellist (&find_args.ulist);

	send_to_server ("ci\012", 0);
	err = get_responses_and_close ();
	if (err != 0 && use_editor && saved_message != NULL)
	{
	    /* If there was an error, don't nuke the user's carefully
	       constructed prose.  This is something of a kludge; a better
	       solution is probably more along the lines of #150 in TODO
	       (doing a second up-to-date check before accepting the
	       log message has also been suggested, but that seems kind of
	       iffy because the real up-to-date check could still fail,
	       another error could occur, &c.  Also, a second check would
	       slow things down).  */

	    char *fname;
	    FILE *fp;

	    fp = cvs_temp_file (&fname);
	    if (fp == NULL)
		error (1, 0, "cannot create temporary file %s", fname);
	    if (fwrite (saved_message, 1, strlen (saved_message), fp)
		!= strlen (saved_message))
		error (1, errno, "cannot write temporary file %s", fname);
	    if (fclose (fp) < 0)
		error (0, errno, "cannot close temporary file %s", fname);
	    error (0, 0, "saving log message in %s", fname);
	    free (fname);
	}
	return err;
    }
#endif

    if (saved_tag != NULL)
	tag_check_valid (saved_tag, argc, argv, local, aflag, "", false);

    /* XXX - this is not the perfect check for this */
    if (argc <= 0)
	write_dirtag = saved_tag;

    wrap_setup ();

    lock_tree_promotably (argc, argv, local, W_LOCAL, aflag);

    /*
     * Set up the master update list and hard link list
     */
    mulist = getlist ();

#ifdef PRESERVE_PERMISSIONS_SUPPORT
    if (preserve_perms)
    {
	hardlist = getlist ();

	/*
	 * We need to save the working directory so that
	 * check_fileproc can construct a full pathname for each file.
	 */
	working_dir = xgetcwd ();
    }
#endif

    /*
     * Run the recursion processor to verify the files are all up-to-date
     */
    err = start_recursion (check_fileproc, check_filesdoneproc,
                           check_direntproc, NULL, NULL, argc, argv, local,
                           W_LOCAL, aflag, CVS_LOCK_NONE, NULL, 1, NULL);
    if (err)
	error (1, 0, "correct above errors first!");

    /*
     * Run the recursion processor to commit the files
     */
    write_dirnonbranch = 0;
    if (noexec == 0)
	err = start_recursion (commit_fileproc, commit_filesdoneproc,
                               commit_direntproc, commit_dirleaveproc, NULL,
                               argc, argv, local, W_LOCAL, aflag,
                               CVS_LOCK_WRITE, NULL, 1, NULL);

    /*
     * Unlock all the dirs and clean up
     */
    Lock_Cleanup ();
    dellist (&mulist);

    /* see if we need to sleep before returning to avoid time-stamp races */
    if (!server_active && last_register_time)
    {
	sleep_past (last_register_time);
    }

    return err;
}



/* This routine determines the status of a given file and retrieves
   the version information that is associated with that file. */

static
Ctype
classify_file_internal (struct file_info *finfo, Vers_TS **vers)
{
    int save_noexec, save_quiet, save_really_quiet;
    Ctype status;

    /* FIXME: Do we need to save quiet as well as really_quiet?  Last
       time I glanced at Classify_File I only saw it looking at really_quiet
       not quiet.  */
    save_noexec = noexec;
    save_quiet = quiet;
    save_really_quiet = really_quiet;
    noexec = quiet = really_quiet = 1;

    /* handle specified numeric revision specially */
    if (saved_tag && isdigit ((unsigned char) *saved_tag))
    {
	/* If the tag is for the trunk, make sure we're at the head */
	if (numdots (saved_tag) < 2)
	{
	    status = Classify_File (finfo, NULL, NULL,
				    NULL, 1, aflag, vers, 0);
	    if (status == T_UPTODATE || status == T_MODIFIED ||
		status == T_ADDED)
	    {
		Ctype xstatus;

		freevers_ts (vers);
		xstatus = Classify_File (finfo, saved_tag, NULL,
					 NULL, 1, aflag, vers, 0);
		if (xstatus == T_REMOVE_ENTRY)
		    status = T_MODIFIED;
		else if (status == T_MODIFIED && xstatus == T_CONFLICT)
		    status = T_MODIFIED;
		else
		    status = xstatus;
	    }
	}
	else
	{
	    char *xtag, *cp;

	    /*
	     * The revision is off the main trunk; make sure we're
	     * up-to-date with the head of the specified branch.
	     */
	    xtag = xstrdup (saved_tag);
	    if ((numdots (xtag) & 1) != 0)
	    {
		cp = strrchr (xtag, '.');
		*cp = '\0';
	    }
	    status = Classify_File (finfo, xtag, NULL,
				    NULL, 1, aflag, vers, 0);
	    if ((status == T_REMOVE_ENTRY || status == T_CONFLICT)
		&& (cp = strrchr (xtag, '.')) != NULL)
	    {
		/* pluck one more dot off the revision */
		*cp = '\0';
		freevers_ts (vers);
		status = Classify_File (finfo, xtag, NULL,
					NULL, 1, aflag, vers, 0);
		if (status == T_UPTODATE || status == T_REMOVE_ENTRY)
		    status = T_MODIFIED;
	    }
	    /* now, muck with vers to make the tag correct */
	    free ((*vers)->tag);
	    (*vers)->tag = xstrdup (saved_tag);
	    free (xtag);
	}
    }
    else
	status = Classify_File (finfo, saved_tag, NULL, NULL, 1, 0, vers, 0);
    noexec = save_noexec;
    quiet = save_quiet;
    really_quiet = save_really_quiet;

    return status;
}



/*
 * Check to see if a file is ok to commit and make sure all files are
 * up-to-date
 */
/* ARGSUSED */
static int
check_fileproc (void *callerdat, struct file_info *finfo)
{
    Ctype status;
    const char *xdir;
    Node *p;
    List *ulist, *cilist;
    Vers_TS *vers;
    struct commit_info *ci;
    struct logfile_info *li;
    int retval = 1;

    size_t cvsroot_len = strlen (current_parsed_root->directory);

    if (!finfo->repository)
    {
	error (0, 0, "nothing known about `%s'", finfo->fullname);
	return 1;
    }

    if (strncmp (finfo->repository, current_parsed_root->directory,
                 cvsroot_len) == 0
	&& ISSLASH (finfo->repository[cvsroot_len])
	&& strncmp (finfo->repository + cvsroot_len + 1,
		    CVSROOTADM,
		    sizeof (CVSROOTADM) - 1) == 0
	&& ISSLASH (finfo->repository[cvsroot_len + sizeof (CVSROOTADM)])
	&& strcmp (finfo->repository + cvsroot_len + sizeof (CVSROOTADM) + 1,
		   CVSNULLREPOS) == 0
	)
	error (1, 0, "cannot check in to %s", finfo->repository);

    status = classify_file_internal (finfo, &vers);

    /*
     * If the force-commit option is enabled, and the file in question
     * appears to be up-to-date, just make it look modified so that
     * it will be committed.
     */
    if (force_ci && status == T_UPTODATE)
	status = T_MODIFIED;

    switch (status)
    {
	case T_CHECKOUT:
	case T_PATCH:
	case T_NEEDS_MERGE:
	case T_REMOVE_ENTRY:
	    error (0, 0, "Up-to-date check failed for `%s'", finfo->fullname);
	    goto out;
	case T_CONFLICT:
	case T_MODIFIED:
	case T_ADDED:
	case T_REMOVED:
        {
            char *editor;

	    /*
	     * some quick sanity checks; if no numeric -r option specified:
	     *	- can't have a sticky date
	     *	- can't have a sticky tag that is not a branch
	     * Also,
	     *	- if status is T_REMOVED, file must not exist and its entry
	     *	  can't have a numeric sticky tag.
	     *	- if status is T_ADDED, rcs file must not exist unless on
	     *    a branch or head is dead
	     *	- if status is T_ADDED, can't have a non-trunk numeric rev
	     *	- if status is T_MODIFIED and a Conflict marker exists, don't
	     *    allow the commit if timestamp is identical or if we find
	     *    an RCS_MERGE_PAT in the file.
	     */
	    if (!saved_tag || !isdigit ((unsigned char) *saved_tag))
	    {
		if (vers->date)
		{
		    error (0, 0,
			   "cannot commit with sticky date for file `%s'",
			   finfo->fullname);
		    goto out;
		}
		if (status == T_MODIFIED && vers->tag &&
		    !RCS_isbranch (finfo->rcs, vers->tag))
		{
		    error (0, 0,
			   "sticky tag `%s' for file `%s' is not a branch",
			   vers->tag, finfo->fullname);
		    goto out;
		}
	    }
	    if (status == T_CONFLICT && !force_ci)
	    {
		error (0, 0,
		      "file `%s' had a conflict and has not been modified",
		       finfo->fullname);
		goto out;
	    }
	    if (status == T_MODIFIED && !force_ci && file_has_markers (finfo))
	    {
		/* Make this a warning, not an error, because we have
		   no way of knowing whether the "conflict indicators"
		   are really from a conflict or whether they are part
		   of the document itself (cvs.texinfo and sanity.sh in
		   CVS itself, for example, tend to want to have strings
		   like ">>>>>>>" at the start of a line).  Making people
		   kludge this the way they need to kludge keyword
		   expansion seems undesirable.  And it is worse than
		   keyword expansion, because there is no -ko
		   analogue.  */
		error (0, 0,
		       "\
warning: file `%s' seems to still contain conflict indicators",
		       finfo->fullname);
	    }

	    if (status == T_REMOVED)
	    {
		if (vers->ts_user != NULL)
		{
		    error (0, 0,
			   "`%s' should be removed and is still there (or is"
			   " back again)", finfo->fullname);
		    goto out;
		}

		if (vers->tag && isdigit ((unsigned char) *vers->tag))
		{
		    /* Remove also tries to forbid this, but we should check
		       here.  I'm only _sure_ about somewhat obscure cases
		       (hacking the Entries file, using an old version of
		       CVS for the remove and a new one for the commit), but
		       there might be other cases.  */
		    error (0, 0,
			   "cannot remove file `%s' which has a numeric sticky"
			   " tag of `%s'", finfo->fullname, vers->tag);
		    freevers_ts (&vers);
		    goto out;
		}
	    }
	    if (status == T_ADDED)
	    {
	        if (vers->tag == NULL)
		{
		    if (finfo->rcs != NULL &&
			!RCS_isdead (finfo->rcs, finfo->rcs->head))
		    {
			error (0, 0,
		    "cannot add file `%s' when RCS file `%s' already exists",
			       finfo->fullname, finfo->rcs->path);
			goto out;
		    }
		}
		else if (isdigit ((unsigned char) *vers->tag) &&
		    numdots (vers->tag) > 1)
		{
		    error (0, 0,
		"cannot add file `%s' with revision `%s'; must be on trunk",
			       finfo->fullname, vers->tag);
		    goto out;
		}
	    }

	    /* done with consistency checks; now, to get on with the commit */
	    if (finfo->update_dir[0] == '\0')
		xdir = ".";
	    else
		xdir = finfo->update_dir;
	    if ((p = findnode (mulist, xdir)) != NULL)
	    {
		ulist = ((struct master_lists *) p->data)->ulist;
		cilist = ((struct master_lists *) p->data)->cilist;
	    }
	    else
	    {
		struct master_lists *ml;

		ml = xmalloc (sizeof (struct master_lists));
		ulist = ml->ulist = getlist ();
		cilist = ml->cilist = getlist ();

		p = getnode ();
		p->key = xstrdup (xdir);
		p->type = UPDATE;
		p->data = ml;
		p->delproc = masterlist_delproc;
		(void) addnode (mulist, p);
	    }

	    /* first do ulist, then cilist */
	    p = getnode ();
	    p->key = xstrdup (finfo->file);
	    p->type = UPDATE;
	    p->delproc = update_delproc;
	    li = xmalloc (sizeof (struct logfile_info));
	    li->type = status;

	    if (check_valid_edit)
            {
                char *editors = NULL;

		editor = NULL;
                editors = fileattr_get0 (finfo->file, "_editors");
                if (editors != NULL)
                {
                    char *caller = getcaller ();
                    char *p = NULL;
                    char *p0 = NULL;

                    p = editors;
                    p0 = p;
                    while (*p != '\0')
                    {
                        p = strchr (p, '>');
                        if (p == NULL)
                        {
                            break;
                        }
                        *p = '\0';
                        if (strcmp (caller, p0) == 0)
                        {
                            break;
                        }
                        p = strchr (p + 1, ',');
                        if (p == NULL)
                        {
                            break;
                        }
                        ++p;
                        p0 = p;
                    }

                    if (strcmp (caller, p0) == 0)
                    {
                        editor = caller;
                    }

                    free (editors);
                }
            }

            if (check_valid_edit && editor == NULL)
            {
                error (0, 0, "Valid edit does not exist for %s",
                       finfo->fullname);
                freevers_ts (&vers);
                return 1;
            }

	    li->tag = xstrdup (vers->tag);
	    /* If the file was re-added, we want the revision in the commitlog
	       to be NONE, not the previous dead revision. */
	    li->rev_old = status == T_ADDED ? NULL : xstrdup (vers->vn_rcs);
	    li->rev_new = NULL;
	    p->data = li;
	    (void) addnode (ulist, p);

	    p = getnode ();
	    p->key = xstrdup (finfo->file);
	    p->type = UPDATE;
	    p->delproc = ci_delproc;
	    ci = xmalloc (sizeof (struct commit_info));
	    ci->status = status;
	    if (vers->tag)
		if (isdigit ((unsigned char) *vers->tag))
		    ci->rev = xstrdup (vers->tag);
		else
		    ci->rev = RCS_whatbranch (finfo->rcs, vers->tag);
	    else
		ci->rev = NULL;
	    ci->tag = xstrdup (vers->tag);
	    ci->options = xstrdup (vers->options);
	    p->data = ci;
	    (void) addnode (cilist, p);

#ifdef PRESERVE_PERMISSIONS_SUPPORT
	    if (preserve_perms)
	    {
		/* Add this file to hardlist, indexed on its inode.  When
		   we are done, we can find out what files are hardlinked
		   to a given file by looking up its inode in hardlist. */
		char *fullpath;
		Node *linkp;
		struct hardlink_info *hlinfo;

		/* Get the full pathname of the current file. */
		fullpath = Xasprintf ("%s/%s", working_dir, finfo->fullname);

		/* To permit following links in subdirectories, files
                   are keyed on finfo->fullname, not on finfo->name. */
		linkp = lookup_file_by_inode (fullpath);

		/* If linkp is NULL, the file doesn't exist... maybe
		   we're doing a remove operation? */
		if (linkp != NULL)
		{
		    /* Create a new hardlink_info node, which will record
		       the current file's status and the links listed in its
		       `hardlinks' delta field.  We will append this
		       hardlink_info node to the appropriate hardlist entry. */
		    hlinfo = xmalloc (sizeof (struct hardlink_info));
		    hlinfo->status = status;
		    linkp->data = hlinfo;
		}
	    }
#endif

	    break;
        }

	case T_UNKNOWN:
	    error (0, 0, "nothing known about `%s'", finfo->fullname);
	    goto out;
	case T_UPTODATE:
	    break;
	default:
	    error (0, 0, "CVS internal error: unknown status %d", status);
	    break;
    }

    retval = 0;

 out:

    freevers_ts (&vers);
    return retval;
}



/*
 * By default, return the code that tells do_recursion to examine all
 * directories
 */
/* ARGSUSED */
static Dtype
check_direntproc (void *callerdat, const char *dir, const char *repos,
                  const char *update_dir, List *entries)
{
    if (!isdir (dir))
	return R_SKIP_ALL;

    if (!quiet)
	error (0, 0, "Examining %s", update_dir);

    return R_PROCESS;
}



/*
 * Walklist proc to generate an arg list from the line in commitinfo
 */
static int
precommit_list_to_args_proc (p, closure)
    Node *p;
    void *closure;
{
    struct format_cmdline_walklist_closure *c = closure;
    struct logfile_info *li;
    char *arg = NULL;
    const char *f;
    char *d;
    size_t doff;

    if (p->data == NULL) return 1;

    f = c->format;
    d = *c->d;
    /* foreach requested attribute */
    while (*f)
    {
   	switch (*f++)
	{
	    case 's':
		li = p->data;
		if (li->type == T_ADDED
			|| li->type == T_MODIFIED
			|| li->type == T_REMOVED)
		{
		    arg = p->key;
		}
		break;
	    default:
		error (1, 0,
		       "Unknown format character or not a list attribute: %c",
		       f[-1]);
		/* NOTREACHED */
		break;
	}
	/* copy the attribute into an argument */
	if (c->quotes)
	{
	    arg = cmdlineescape (c->quotes, arg);
	}
	else
	{
	    arg = cmdlinequote ('"', arg);
	}
	doff = d - *c->buf;
	expand_string (c->buf, c->length, doff + strlen (arg));
	d = *c->buf + doff;
	strncpy (d, arg, strlen (arg));
	d += strlen (arg);
	free (arg);

	/* and always put the extra space on.  we'll have to back up a char
	 * when we're done, but that seems most efficient
	 */
	doff = d - *c->buf;
	expand_string (c->buf, c->length, doff + 1);
	d = *c->buf + doff;
	*d++ = ' ';
    }
    /* correct our original pointer into the buff */
    *c->d = d;
    return 0;
}



/*
 * Callback proc for pre-commit checking
 */
static int
precommit_proc (const char *repository, const char *filter, void *closure)
{
    char *newfilter = NULL;
    char *cmdline;
    const char *srepos = Short_Repository (repository);
    List *ulist = closure;

#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    if (!strchr (filter, '%'))
    {
	error (0, 0,
               "warning: commitinfo line contains no format strings:\n"
               "    \"%s\"\n"
               "Appending defaults (\" %%r/%%p %%s\"), but please be aware that this usage is\n"
               "deprecated.", filter);
	newfilter = Xasprintf ("%s %%r/%%p %%s", filter);
	filter = newfilter;
    }
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */

    /*
     * Cast any NULL arguments as appropriate pointers as this is an
     * stdarg function and we need to be certain the caller gets what
     * is expected.
     */
    cmdline = format_cmdline (
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
			      false, srepos,
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
			      filter,
			      "c", "s", cvs_cmd_name,
#ifdef SERVER_SUPPORT
			      "R", "s", referrer ? referrer->original : "NONE",
#endif /* SERVER_SUPPORT */
			      "p", "s", srepos,
			      "r", "s", current_parsed_root->directory,
			      "s", ",", ulist, precommit_list_to_args_proc,
			      (void *) NULL,
			      (char *) NULL);

    if (newfilter) free (newfilter);

    if (!cmdline || !strlen (cmdline))
    {
	if (cmdline) free (cmdline);
	error (0, 0, "precommit proc resolved to the empty string!");
	return 1;
    }

    run_setup (cmdline);
    free (cmdline);

    return run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL|RUN_REALLY|
	(server_active ? 0 : RUN_UNSETXID));
}



/*
 * Run the pre-commit checks for the dir
 */
/* ARGSUSED */
static int
check_filesdoneproc (void *callerdat, int err, const char *repos,
                     const char *update_dir, List *entries)
{
    int n;
    Node *p;
    List *saved_ulist;

    /* find the update list for this dir */
    p = findnode (mulist, update_dir);
    if (p != NULL)
	saved_ulist = ((struct master_lists *) p->data)->ulist;
    else
	saved_ulist = NULL;

    /* skip the checks if there's nothing to do */
    if (saved_ulist == NULL || saved_ulist->list->next == saved_ulist->list)
	return err;

    /* run any pre-commit checks */
    n = Parse_Info (CVSROOTADM_COMMITINFO, repos, precommit_proc, PIOPT_ALL,
                    saved_ulist);
    if (n > 0)
    {
	error (0, 0, "Pre-commit check failed");
	err += n;
    }

    return err;
}



/*
 * Do the work of committing a file
 */
static int maxrev;
static char *sbranch;

/* ARGSUSED */
static int
commit_fileproc (void *callerdat, struct file_info *finfo)
{
    Node *p;
    int err = 0;
    List *ulist, *cilist;
    struct commit_info *ci;

    /* Keep track of whether write_dirtag is a branch tag.
       Note that if it is a branch tag in some files and a nonbranch tag
       in others, treat it as a nonbranch tag.  It is possible that case
       should elicit a warning or an error.  */
    if (write_dirtag != NULL
	&& finfo->rcs != NULL)
    {
	char *rev = RCS_getversion (finfo->rcs, write_dirtag, NULL, 1, NULL);
	if (rev != NULL
	    && !RCS_nodeisbranch (finfo->rcs, write_dirtag))
	    write_dirnonbranch = 1;
	if (rev != NULL)
	    free (rev);
    }

    if (finfo->update_dir[0] == '\0')
	p = findnode (mulist, ".");
    else
	p = findnode (mulist, finfo->update_dir);

    /*
     * if p is null, there were file type command line args which were
     * all up-to-date so nothing really needs to be done
     */
    if (p == NULL)
	return 0;
    ulist = ((struct master_lists *) p->data)->ulist;
    cilist = ((struct master_lists *) p->data)->cilist;

    /*
     * At this point, we should have the commit message unless we were called
     * with files as args from the command line.  In that latter case, we
     * need to get the commit message ourselves
     */
    if (!got_message)
    {
	got_message = 1;
	if (!server_active && use_editor)
	    do_editor (finfo->update_dir, &saved_message,
		       finfo->repository, ulist);
	do_verify (&saved_message, finfo->repository, ulist);
    }

    p = findnode (cilist, finfo->file);
    if (p == NULL)
	return 0;

    ci = p->data;

/* cvsacl patch */
#ifdef SERVER_SUPPORT
    if (use_cvs_acl /* && server_active */)
    {
	int whichperm;
	if (ci->status == T_MODIFIED)
	    whichperm = 3;
	else if (ci->status == T_ADDED)
	    whichperm = 6;
	else if (ci->status == T_REMOVED)	
	    whichperm = 7;

	if (!access_allowed (finfo->file, finfo->repository, ci->tag, whichperm,
			     NULL, NULL, 1))
	{
	    if (stop_at_first_permission_denied)
		error (1, 0, "permission denied for %s",
		       Short_Repository (finfo->repository));
	    else
		error (0, 0, "permission denied for %s/%s",
		       Short_Repository (finfo->repository), finfo->file);
			
		return (0);
	}
    }
#endif

    if (ci->status == T_MODIFIED)
    {
	if (finfo->rcs == NULL)
	    error (1, 0, "internal error: no parsed RCS file");
	if (lock_RCS (finfo->file, finfo->rcs, ci->rev,
		      finfo->repository) != 0)
	{
	    unlockrcs (finfo->rcs);
	    err = 1;
	    goto out;
	}
    }
    else if (ci->status == T_ADDED)
    {
	if (checkaddfile (finfo->file, finfo->repository, ci->tag, ci->options,
			  &finfo->rcs) != 0)
	{
	    if (finfo->rcs != NULL)
		fixaddfile (finfo->rcs->path);
	    err = 1;
	    goto out;
	}

	/* adding files with a tag, now means adding them on a branch.
	   Since the branch test was done in check_fileproc for
	   modified files, we need to stub it in again here. */

	if (ci->tag

	    /* If numeric, it is on the trunk; check_fileproc enforced
	       this.  */
	    && !isdigit ((unsigned char) ci->tag[0]))
	{
	    if (finfo->rcs == NULL)
		error (1, 0, "internal error: no parsed RCS file");
	    if (ci->rev)
		free (ci->rev);
	    ci->rev = RCS_whatbranch (finfo->rcs, ci->tag);
	    err = Checkin ('A', finfo, ci->rev,
			   ci->tag, ci->options, saved_message);
	    if (err != 0)
	    {
		unlockrcs (finfo->rcs);
		fixbranch (finfo->rcs, sbranch);
	    }

	    (void) time (&last_register_time);

	    ci->status = T_UPTODATE;
	}
    }

    /*
     * Add the file for real
     */
    if (ci->status == T_ADDED)
    {
	char *xrev = NULL;

	if (ci->rev == NULL)
	{
	    /* find the max major rev number in this directory */
	    maxrev = 0;
	    (void) walklist (finfo->entries, findmaxrev, NULL);
	    if (finfo->rcs->head)
	    {
		/* resurrecting: include dead revision */
		int thisrev = atoi (finfo->rcs->head);
		if (thisrev > maxrev)
		    maxrev = thisrev;
	    }
	    if (maxrev == 0)
		maxrev = 1;
	    xrev = Xasprintf ("%d", maxrev);
	}

	/* XXX - an added file with symbolic -r should add tag as well */
	err = finaladd (finfo, ci->rev ? ci->rev : xrev, ci->tag, ci->options);
	if (xrev)
	    free (xrev);
    }
    else if (ci->status == T_MODIFIED)
    {
	err = Checkin ('M', finfo, ci->rev, ci->tag,
		       ci->options, saved_message);

	(void) time (&last_register_time);

	if (err != 0)
	{
	    unlockrcs (finfo->rcs);
	    fixbranch (finfo->rcs, sbranch);
	}
    }
    else if (ci->status == T_REMOVED)
    {
	err = remove_file (finfo, ci->tag, saved_message);
#ifdef SERVER_SUPPORT
	if (server_active)
	{
	    server_scratch_entry_only ();
	    server_updated (finfo,
			    NULL,

			    /* Doesn't matter, it won't get checked.  */
			    SERVER_UPDATED,

			    (mode_t) -1,
			    NULL,
			    NULL);
	}
#endif
    }

    /* Clearly this is right for T_MODIFIED.  I haven't thought so much
       about T_ADDED or T_REMOVED.  */
    notify_do ('C', finfo->file, finfo->update_dir, getcaller (), NULL, NULL,
	       finfo->repository);

out:
    if (err != 0)
    {
	/* on failure, remove the file from ulist */
	p = findnode (ulist, finfo->file);
	if (p)
	    delnode (p);
    }
    else
    {
	/* On success, retrieve the new version number of the file and
           copy it into the log information (see logmsg.c
           (logfile_write) for more details).  We should only update
           the version number for files that have been added or
           modified but not removed since classify_file_internal
           will return the version number of a file even after it has
           been removed from the archive, which is not the behavior we
           want for our commitlog messages; we want the old version
           number and then "NONE." */

	if (ci->status != T_REMOVED)
	{
	    p = findnode (ulist, finfo->file);
	    if (p)
	    {
		Vers_TS *vers;
		struct logfile_info *li;

		(void) classify_file_internal (finfo, &vers);
		li = p->data;
		li->rev_new = xstrdup (vers->vn_rcs);
		freevers_ts (&vers);
	    }
	}
    }
    if (SIG_inCrSect ())
	SIG_endCrSect ();

    return err;
}



/*
 * Log the commit and clean up the update list
 */
/* ARGSUSED */
static int
commit_filesdoneproc (void *callerdat, int err, const char *repository,
                      const char *update_dir, List *entries)
{
    Node *p;
    List *ulist;

    assert (repository);

    p = findnode (mulist, update_dir);
    if (p == NULL)
	return err;

    ulist = ((struct master_lists *) p->data)->ulist;

    got_message = 0;

    /* Build the administrative files if necessary.  */
    {
	const char *p;

	if (strncmp (current_parsed_root->directory, repository,
		     strlen (current_parsed_root->directory)) != 0)
	    error (0, 0,
		 "internal error: repository (%s) doesn't begin with root (%s)",
		   repository, current_parsed_root->directory);
	p = repository + strlen (current_parsed_root->directory);
	if (*p == '/')
	    ++p;
	if (strcmp ("CVSROOT", p) == 0
	    /* Check for subdirectories because people may want to create
	       subdirectories and list files therein in checkoutlist.  */
	    || strncmp ("CVSROOT/", p, strlen ("CVSROOT/")) == 0
	    )
	{
	    /* "Database" might a little bit grandiose and/or vague,
	       but "checked-out copies of administrative files, unless
	       in the case of modules and you are using ndbm in which
	       case modules.{pag,dir,db}" is verbose and excessively
	       focused on how the database is implemented.  */

	    /* mkmodules requires the absolute name of the CVSROOT directory.
	       Remove anything after the `CVSROOT' component -- this is
	       necessary when committing in a subdirectory of CVSROOT.  */
	    char *admin_dir = xstrdup (repository);
	    int cvsrootlen = strlen ("CVSROOT");
	    assert (admin_dir[p - repository + cvsrootlen] == '\0'
		    || admin_dir[p - repository + cvsrootlen] == '/');
	    admin_dir[p - repository + cvsrootlen] = '\0';

	    if (!really_quiet)
	    {
		cvs_output (program_name, 0);
		cvs_output (" ", 1);
		cvs_output (cvs_cmd_name, 0);
		cvs_output (": Rebuilding administrative file database\n", 0);
	    }
	    mkmodules (admin_dir);
	    free (admin_dir);
	    WriteTemplate (".", 1, repository);
	}
    }

    /* FIXME: This used to be above the block above.  The advantage of being
     * here is that it is not called until after all possible writes from this
     * process are complete.  The disadvantage is that a fatal error during
     * update of CVSROOT can prevent the loginfo script from being called.
     *
     * A more general solution I have been considering is calling a generic
     * "postwrite" hook from the remove write lock routine.
     */
    Update_Logfile (repository, saved_message, NULL, ulist);

    return err;
}



/*
 * Get the log message for a dir
 */
/* ARGSUSED */
static Dtype
commit_direntproc (void *callerdat, const char *dir, const char *repos,
                   const char *update_dir, List *entries)
{
    Node *p;
    List *ulist;
    char *real_repos;

    if (!isdir (dir))
	return R_SKIP_ALL;

    /* find the update list for this dir */
    p = findnode (mulist, update_dir);
    if (p != NULL)
	ulist = ((struct master_lists *) p->data)->ulist;
    else
	ulist = NULL;

    /* skip the files as an optimization */
    if (ulist == NULL || ulist->list->next == ulist->list)
	return R_SKIP_FILES;

    /* get commit message */
    got_message = 1;
    real_repos = Name_Repository (dir, update_dir);
    if (!server_active && use_editor)
	do_editor (update_dir, &saved_message, real_repos, ulist);
    do_verify (&saved_message, real_repos, ulist);
    free (real_repos);
    return R_PROCESS;
}



/*
 * Process the post-commit proc if necessary
 */
/* ARGSUSED */
static int
commit_dirleaveproc (void *callerdat, const char *dir, int err,
                     const char *update_dir, List *entries)
{
    /* update the per-directory tag info */
    /* FIXME?  Why?  The "commit examples" node of cvs.texinfo briefly
       mentions commit -r being sticky, but apparently in the context of
       this being a confusing feature!  */
    if (err == 0 && write_dirtag != NULL)
    {
	char *repos = Name_Repository (NULL, update_dir);
	WriteTag (NULL, write_dirtag, NULL, write_dirnonbranch,
		  update_dir, repos);
	free (repos);
    }

    return err;
}



/*
 * find the maximum major rev number in an entries file
 */
static int
findmaxrev (Node *p, void *closure)
{
    int thisrev;
    Entnode *entdata = p->data;

    if (entdata->type != ENT_FILE)
	return 0;
    thisrev = atoi (entdata->version);
    if (thisrev > maxrev)
	maxrev = thisrev;
    return 0;
}

/*
 * Actually remove a file by moving it to the attic
 * XXX - if removing a ,v file that is a relative symbolic link to
 * another ,v file, we probably should add a ".." component to the
 * link to keep it relative after we move it into the attic.

   Return value is 0 on success, or >0 on error (in which case we have
   printed an error message).  */
static int
remove_file (struct file_info *finfo, char *tag, char *message)
{
    int retcode;

    int branch;
    int lockflag;
    char *corev;
    char *rev;
    char *prev_rev;
    char *old_path;

    corev = NULL;
    rev = NULL;
    prev_rev = NULL;

    retcode = 0;

    if (finfo->rcs == NULL)
	error (1, 0, "internal error: no parsed RCS file");

    branch = 0;
    if (tag && !(branch = RCS_nodeisbranch (finfo->rcs, tag)))
    {
	/* a symbolic tag is specified; just remove the tag from the file */
	if ((retcode = RCS_deltag (finfo->rcs, tag)) != 0)
	{
	    if (!quiet)
		error (0, retcode == -1 ? errno : 0,
		       "failed to remove tag `%s' from `%s'", tag,
		       finfo->fullname);
	    return 1;
	}
	RCS_rewrite (finfo->rcs, NULL, NULL);
	Scratch_Entry (finfo->entries, finfo->file);
	return 0;
    }

    /* we are removing the file from either the head or a branch */
    /* commit a new, dead revision. */

    rev = NULL;
    lockflag = 1;
    if (branch)
    {
	char *branchname;

	rev = RCS_whatbranch (finfo->rcs, tag);
	if (rev == NULL)
	{
	    error (0, 0, "cannot find branch \"%s\".", tag);
	    return 1;
	}

	branchname = RCS_getbranch (finfo->rcs, rev, 1);
	if (branchname == NULL)
	{
	    /* no revision exists on this branch.  use the previous
	       revision but do not lock. */
	    corev = RCS_gettag (finfo->rcs, tag, 1, NULL);
	    prev_rev = xstrdup (corev);
	    lockflag = 0;
	} else
	{
	    corev = xstrdup (rev);
	    prev_rev = xstrdup (branchname);
	    free (branchname);
	}

    } else  /* Not a branch */
    {
        /* Get current head revision of file. */
	prev_rev = RCS_head (finfo->rcs);
    }

    /* if removing without a tag or a branch, then make sure the default
       branch is the trunk. */
    if (!tag && !branch)
    {
        if (RCS_setbranch (finfo->rcs, NULL) != 0)
	{
	    error (0, 0, "cannot change branch to default for %s",
		   finfo->fullname);
	    return 1;
	}
	RCS_rewrite (finfo->rcs, NULL, NULL);
    }

    /* check something out.  Generally this is the head.  If we have a
       particular rev, then name it.  */
    retcode = RCS_checkout (finfo->rcs, finfo->file, rev ? corev : NULL,
			    NULL, NULL, RUN_TTY, NULL, NULL);
    if (retcode != 0)
    {
	error (0, 0,
	       "failed to check out `%s'", finfo->fullname);
	return 1;
    }

    /* Except when we are creating a branch, lock the revision so that
       we can check in the new revision.  */
    if (lockflag)
    {
	if (RCS_lock (finfo->rcs, rev ? corev : NULL, 1) == 0)
	    RCS_rewrite (finfo->rcs, NULL, NULL);
    }

    if (corev != NULL)
	free (corev);

    retcode = RCS_checkin (finfo->rcs, NULL, finfo->file, message,
			   rev, 0, RCS_FLAGS_DEAD | RCS_FLAGS_QUIET);
    if (retcode	!= 0)
    {
	if (!quiet)
	    error (0, retcode == -1 ? errno : 0,
		   "failed to commit dead revision for `%s'", finfo->fullname);
	return 1;
    }
    /* At this point, the file has been committed as removed.  We should
       probably tell the history file about it  */
    history_write ('R', NULL, finfo->rcs->head, finfo->file, finfo->repository);

    if (rev != NULL)
	free (rev);

    old_path = xstrdup (finfo->rcs->path);
    if (!branch)
	RCS_setattic (finfo->rcs, 1);

    /* Print message that file was removed. */
    if (!really_quiet)
    {
	cvs_output (old_path, 0);
	cvs_output ("  <--  ", 0);
	if (finfo->update_dir && strlen (finfo->update_dir))
	{
	    cvs_output (finfo->update_dir, 0);
	    cvs_output ("/", 1);
	}
	cvs_output (finfo->file, 0);
	cvs_output ("\nnew revision: delete; previous revision: ", 0);
	cvs_output (prev_rev, 0);
	cvs_output ("\n", 0);
    }

    free (prev_rev);

    free (old_path);

    Scratch_Entry (finfo->entries, finfo->file);
    return 0;
}



/*
 * Do the actual checkin for added files
 */
static int
finaladd (struct file_info *finfo, char *rev, char *tag, char *options)
{
    int ret;

    ret = Checkin ('A', finfo, rev, tag, options, saved_message);
    if (ret == 0)
    {
	char *tmp = Xasprintf ("%s/%s%s", CVSADM, finfo->file, CVSEXT_LOG);
	if (unlink_file (tmp) < 0
	    && !existence_error (errno))
	    error (0, errno, "cannot remove %s", tmp);
	free (tmp);
    }
    else if (finfo->rcs != NULL)
	fixaddfile (finfo->rcs->path);

    (void) time (&last_register_time);

    return ret;
}



/*
 * Unlock an rcs file
 */
static void
unlockrcs (RCSNode *rcs)
{
    int retcode;

    if ((retcode = RCS_unlock (rcs, NULL, 1)) != 0)
	error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
	       "could not unlock %s", rcs->path);
    else
	RCS_rewrite (rcs, NULL, NULL);
}



/*
 * remove a partially added file.  if we can parse it, leave it alone.
 *
 * FIXME: Every caller that calls this function can access finfo->rcs (the
 * parsed RCSNode data), so we should be able to detect that the file needs
 * to be removed without reparsing the file as we do below.
 */
static void
fixaddfile (const char *rcs)
{
    RCSNode *rcsfile;
    int save_really_quiet;

    save_really_quiet = really_quiet;
    really_quiet = 1;
    if ((rcsfile = RCS_parsercsfile (rcs)) == NULL)
    {
	if (unlink_file (rcs) < 0)
	    error (0, errno, "cannot remove %s", rcs);
    }
    else
	freercsnode (&rcsfile);
    really_quiet = save_really_quiet;
}



/*
 * put the branch back on an rcs file
 */
static void
fixbranch (RCSNode *rcs, char *branch)
{
    int retcode;

    if (branch != NULL)
    {
	if ((retcode = RCS_setbranch (rcs, branch)) != 0)
	    error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
		   "cannot restore branch to %s for %s", branch, rcs->path);
	RCS_rewrite (rcs, NULL, NULL);
    }
}



/*
 * do the initial part of a file add for the named file.  if adding
 * with a tag, put the file in the Attic and point the symbolic tag
 * at the committed revision.
 *
 * INPUTS
 *   file	The name of the file in the workspace.
 *   repository	The repository directory to expect to find FILE,v in.
 *   tag	The name or rev num of the branch being added to, if any.
 *   options	Any RCS keyword expansion options specified by the user.
 *   rcsnode	A pointer to the pre-parsed RCSNode for this file, if the file
 *		exists in the repository.  If this is NULL, assume the file
 *		does not yet exist.
 *
 * RETURNS
 *   0 on success.
 *   1 on errors, after printing any appropriate error messages.
 *
 * ERRORS
 *   This function will return an error when any of the following functions do:
 *     add_rcs_file
 *     RCS_setattic
 *     lock_RCS
 *     RCS_checkin
 *     RCS_parse (called to verify the newly created archive file)
 *     RCS_settag
 */

static int
checkaddfile (const char *file, const char *repository, const char *tag,
              const char *options, RCSNode **rcsnode)
{
    RCSNode *rcs;
    char *fname;
    int newfile = 0;		/* Set to 1 if we created a new RCS archive. */
    int retval = 1;
    int adding_on_branch;

    assert (rcsnode != NULL);

    /* Callers expect to be able to use either "" or NULL to mean the
       default keyword expansion.  */
    if (options != NULL && options[0] == '\0')
	options = NULL;
    if (options != NULL)
	assert (options[0] == '-' && options[1] == 'k');

    /* If numeric, it is on the trunk; check_fileproc enforced
       this.  */
    adding_on_branch = tag != NULL && !isdigit ((unsigned char) tag[0]);

    if (*rcsnode == NULL)
    {
	char *rcsname;
	char *desc = NULL;
	size_t descalloc = 0;
	size_t desclen = 0;
	const char *opt;

	if (adding_on_branch)
	{
	    mode_t omask;
	    rcsname = xmalloc (strlen (repository)
			       + sizeof (CVSATTIC)
			       + strlen (file)
			       + sizeof (RCSEXT)
			       + 3);
	    (void) sprintf (rcsname, "%s/%s", repository, CVSATTIC);
	    omask = umask (cvsumask);
	    if (CVS_MKDIR (rcsname, 0777) != 0 && errno != EEXIST)
		error (1, errno, "cannot make directory `%s'", rcsname);
	    (void) umask (omask);
	    (void) sprintf (rcsname,
			    "%s/%s/%s%s",
			    repository,
			    CVSATTIC,
			    file,
			    RCSEXT);
	}
	else
	    rcsname = Xasprintf ("%s/%s%s", repository, file, RCSEXT);

	/* this is the first time we have ever seen this file; create
	   an RCS file.  */
	fname = Xasprintf ("%s/%s%s", CVSADM, file, CVSEXT_LOG);
	/* If the file does not exist, no big deal.  In particular, the
	   server does not (yet at least) create CVSEXT_LOG files.  */
	if (isfile (fname))
	    /* FIXME: Should be including update_dir in the appropriate
	       place here.  */
	    get_file (fname, fname, "r", &desc, &descalloc, &desclen);
	free (fname);

	/* From reading the RCS 5.7 source, "rcs -i" adds a newline to the
	   end of the log message if the message is nonempty.
	   Do it.  RCS also deletes certain whitespace, in cleanlogmsg,
	   which we don't try to do here.  */
	if (desclen > 0)
	{
	    expand_string (&desc, &descalloc, desclen + 1);
	    desc[desclen++] = '\012';
	}

	/* Set RCS keyword expansion options.  */
	if (options != NULL)
	    opt = options + 2;
	else
	    opt = NULL;

	if (add_rcs_file (NULL, rcsname, file, NULL, opt,
			  NULL, NULL, 0, NULL,
			  desc, desclen, NULL, 0) != 0)
	{
	    if (rcsname != NULL)
	        free (rcsname);
	    goto out;
	}
	rcs = RCS_parsercsfile (rcsname);
	newfile = 1;
	if (rcsname != NULL)
	    free (rcsname);
	if (desc != NULL)
	    free (desc);
	*rcsnode = rcs;
    }
    else
    {
	/* file has existed in the past.  Prepare to resurrect. */
	char *rev;
	char *oldexpand;

	rcs = *rcsnode;

	oldexpand = RCS_getexpand (rcs);
	if ((oldexpand != NULL
	     && options != NULL
	     && strcmp (options + 2, oldexpand) != 0)
	    || (oldexpand == NULL && options != NULL))
	{
	    /* We tell the user about this, because it means that the
	       old revisions will no longer retrieve the way that they
	       used to.  */
	    error (0, 0, "changing keyword expansion mode to %s", options);
	    RCS_setexpand (rcs, options + 2);
	}

	if (!adding_on_branch)
	{
	    /* We are adding on the trunk, so move the file out of the
	       Attic.  */
	    if (!(rcs->flags & INATTIC))
	    {
		error (0, 0, "warning: expected %s to be in Attic",
		       rcs->path);
	    }

	    /* Begin a critical section around the code that spans the
	       first commit on the trunk of a file that's already been
	       committed on a branch.  */
	    SIG_beginCrSect ();

	    if (RCS_setattic (rcs, 0))
	    {
		goto out;
	    }
	}

	rev = RCS_getversion (rcs, tag, NULL, 1, NULL);
	/* and lock it */
	if (lock_RCS (file, rcs, rev, repository))
	{
	    error (0, 0, "cannot lock revision %s in `%s'.",
		   rev ? rev : tag ? tag : "HEAD", rcs->path);
	    if (rev != NULL)
		free (rev);
	    goto out;
	}

	if (rev != NULL)
	    free (rev);
    }

    /* when adding a file for the first time, and using a tag, we need
       to create a dead revision on the trunk.  */
    if (adding_on_branch)
    {
	if (newfile)
	{
	    char *tmp;
	    FILE *fp;
	    int retcode;

	    /* move the new file out of the way. */
	    fname = Xasprintf ("%s/%s%s", CVSADM, CVSPREFIX, file);
	    rename_file (file, fname);

	    /* Create empty FILE.  Can't use copy_file with a DEVNULL
	       argument -- copy_file now ignores device files. */
	    fp = fopen (file, "w");
	    if (fp == NULL)
		error (1, errno, "cannot open %s for writing", file);
	    if (fclose (fp) < 0)
		error (0, errno, "cannot close %s", file);

	    tmp = Xasprintf ("file %s was initially added on branch %s.",
			     file, tag);
	    /* commit a dead revision. */
	    retcode = RCS_checkin (rcs, NULL, NULL, tmp, NULL, 0,
				   RCS_FLAGS_DEAD | RCS_FLAGS_QUIET);
	    free (tmp);
	    if (retcode != 0)
	    {
		error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
		       "could not create initial dead revision %s", rcs->path);
		free (fname);
		goto out;
	    }

	    /* put the new file back where it was */
	    rename_file (fname, file);
	    free (fname);

	    /* double-check that the file was written correctly */
	    freercsnode (&rcs);
	    rcs = RCS_parse (file, repository);
	    if (rcs == NULL)
	    {
		error (0, 0, "could not read %s", rcs->path);
		goto out;
	    }
	    *rcsnode = rcs;

	    /* and lock it once again. */
	    if (lock_RCS (file, rcs, NULL, repository))
	    {
		error (0, 0, "cannot lock initial revision in `%s'.",
		       rcs->path);
		goto out;
	    }
	}

	/* when adding with a tag, we need to stub a branch, if it
	   doesn't already exist.  */
	if (!RCS_nodeisbranch (rcs, tag))
	{
	    /* branch does not exist.  Stub it.  */
	    char *head;
	    char *magicrev;
	    int retcode;
	    time_t headtime = -1;
	    char *revnum, *tmp;
	    FILE *fp;
	    time_t t = -1;
	    struct tm *ct;

	    fixbranch (rcs, sbranch);

	    head = RCS_getversion (rcs, NULL, NULL, 0, NULL);
	    if (!head)
		error (1, 0, "No head revision in archive file `%s'.",
		       rcs->print_path);
	    magicrev = RCS_magicrev (rcs, head);

	    /* If this is not a new branch, then we will want a dead
	       version created before this one. */
	    if (!newfile)
		headtime = RCS_getrevtime (rcs, head, 0, 0);

	    retcode = RCS_settag (rcs, tag, magicrev);
	    RCS_rewrite (rcs, NULL, NULL);

	    free (head);
	    free (magicrev);

	    if (retcode != 0)
	    {
		error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
		       "could not stub branch %s for %s", tag, rcs->path);
		goto out;
	    }
	    /* We need to add a dead version here to avoid -rtag -Dtime
	       checkout problems between when the head version was
	       created and now. */
	    if (!newfile && headtime != -1)
	    {
		/* move the new file out of the way. */
		fname = Xasprintf ("%s/%s%s", CVSADM, CVSPREFIX, file);
		rename_file (file, fname);

		/* Create empty FILE.  Can't use copy_file with a DEVNULL
		   argument -- copy_file now ignores device files. */
		fp = fopen (file, "w");
		if (fp == NULL)
		    error (1, errno, "cannot open %s for writing", file);
		if (fclose (fp) < 0)
		    error (0, errno, "cannot close %s", file);

		/* As we will be hacking the delta date, put the time
		   this was added into the log message. */
		t = time (NULL);
		ct = gmtime (&t);
		tmp = Xasprintf ("file %s was added on branch %s on %d-%02d-%02d %02d:%02d:%02d +0000",
				 file, tag,
				 ct->tm_year + (ct->tm_year < 100 ? 0 : 1900),
				 ct->tm_mon + 1, ct->tm_mday,
				 ct->tm_hour, ct->tm_min, ct->tm_sec);
			 
		/* commit a dead revision. */
		revnum = RCS_whatbranch (rcs, tag);
		retcode = RCS_checkin (rcs, NULL, NULL, tmp, revnum, headtime,
				       RCS_FLAGS_DEAD |
				       RCS_FLAGS_QUIET |
				       RCS_FLAGS_USETIME);
		free (revnum);
		free (tmp);

		if (retcode != 0)
		{
		    error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
			   "could not created dead stub %s for %s", tag,
			   rcs->path);
		    goto out;
		}

		/* put the new file back where it was */
		rename_file (fname, file);
		free (fname);

		/* double-check that the file was written correctly */
		freercsnode (&rcs);
		rcs = RCS_parse (file, repository);
		if (rcs == NULL)
		{
		    error (0, 0, "could not read %s", rcs->path);
		    goto out;
		}
		*rcsnode = rcs;
	    }
	}
	else
	{
	    /* lock the branch. (stubbed branches need not be locked.)  */
	    if (lock_RCS (file, rcs, NULL, repository))
	    {
		error (0, 0, "cannot lock head revision in `%s'.", rcs->path);
		goto out;
	    }
	}

	if (*rcsnode != rcs)
	{
	    freercsnode (rcsnode);
	    *rcsnode = rcs;
	}
    }

    fileattr_newfile (file);

    /* At this point, we used to set the file mode of the RCS file
       based on the mode of the file in the working directory.  If we
       are creating the RCS file for the first time, add_rcs_file does
       this already.  If we are re-adding the file, then perhaps it is
       consistent to preserve the old file mode, just as we preserve
       the old keyword expansion mode.

       If we decide that we should change the modes, then we can't do
       it here anyhow.  At this point, the RCS file may be owned by
       somebody else, so a chmod will fail.  We need to instead do the
       chmod after rewriting it.

       FIXME: In general, I think the file mode (and the keyword
       expansion mode) should be associated with a particular revision
       of the file, so that it is possible to have different revisions
       of a file have different modes.  */

    retval = 0;

 out:
    if (retval != 0 && SIG_inCrSect ())
	SIG_endCrSect ();
    return retval;
}



/*
 * Attempt to place a lock on the RCS file; returns 0 if it could and 1 if it
 * couldn't.  If the RCS file currently has a branch as the head, we must
 * move the head back to the trunk before locking the file, and be sure to
 * put the branch back as the head if there are any errors.
 */
static int
lock_RCS (const char *user, RCSNode *rcs, const char *rev,
          const char *repository)
{
    char *branch = NULL;
    int err = 0;

    /*
     * For a specified, numeric revision of the form "1" or "1.1", (or when
     * no revision is specified ""), definitely move the branch to the trunk
     * before locking the RCS file.
     *
     * The assumption is that if there is more than one revision on the trunk,
     * the head points to the trunk, not a branch... and as such, it's not
     * necessary to move the head in this case.
     */
    if (rev == NULL
	|| (rev && isdigit ((unsigned char) *rev) && numdots (rev) < 2))
    {
	branch = xstrdup (rcs->branch);
	if (branch != NULL)
	{
	    if (RCS_setbranch (rcs, NULL) != 0)
	    {
		error (0, 0, "cannot change branch to default for %s",
		       rcs->path);
		if (branch)
		    free (branch);
		return 1;
	    }
	}
	err = RCS_lock (rcs, NULL, 1);
    }
    else
    {
	RCS_lock (rcs, rev, 1);
    }

    /* We used to call RCS_rewrite here, and that might seem
       appropriate in order to write out the locked revision
       information.  However, such a call would actually serve no
       purpose.  CVS locks will prevent any interference from other
       CVS processes.  The comment above rcs_internal_lockfile
       explains that it is already unsafe to use RCS and CVS
       simultaneously.  It follows that writing out the locked
       revision information here would add no additional security.

       If we ever do care about it, the proper fix is to create the
       RCS lock file before calling this function, and maintain it
       until the checkin is complete.

       The call to RCS_lock is still required at present, since in
       some cases RCS_checkin will determine which revision to check
       in by looking for a lock.  FIXME: This is rather roundabout,
       and a more straightforward approach would probably be easier to
       understand.  */

    if (err == 0)
    {
	if (sbranch != NULL)
	    free (sbranch);
	sbranch = branch;
	return 0;
    }

    /* try to restore the branch if we can on error */
    if (branch != NULL)
	fixbranch (rcs, branch);

    if (branch)
	free (branch);
    return 1;
}



/*
 * free an UPDATE node's data
 */
void
update_delproc (Node *p)
{
    struct logfile_info *li = p->data;

    if (li->tag)
	free (li->tag);
    if (li->rev_old)
	free (li->rev_old);
    if (li->rev_new)
	free (li->rev_new);
    free (li);
}

/*
 * Free the commit_info structure in p.
 */
static void
ci_delproc (Node *p)
{
    struct commit_info *ci = p->data;

    if (ci->rev)
	free (ci->rev);
    if (ci->tag)
	free (ci->tag);
    if (ci->options)
	free (ci->options);
    free (ci);
}

/*
 * Free the commit_info structure in p.
 */
static void
masterlist_delproc (Node *p)
{
    struct master_lists *ml = p->data;

    dellist (&ml->ulist);
    dellist (&ml->cilist);
    free (ml);
}
