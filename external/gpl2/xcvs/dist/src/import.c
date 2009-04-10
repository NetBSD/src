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
 * "import" checks in the vendor release located in the current directory into
 * the CVS source repository.  The CVS vendor branch support is utilized.
 * 
 * At least three arguments are expected to follow the options:
 *	repository	Where the source belongs relative to the CVSROOT
 *	VendorTag	Vendor's major tag
 *	VendorReleTag	Tag for this particular release
 *
 * Additional arguments specify more Vendor Release Tags.
 */

#include "cvs.h"
#include "lstat.h"
#include "save-cwd.h"

static char *get_comment (const char *user);
static int add_rev (char *message, RCSNode *rcs, char *vfile,
			  char *vers);
static int add_tags (RCSNode *rcs, char *vfile, char *vtag, int targc,
		     char *targv[]);
static int import_descend (char *message, char *vtag, int targc, char *targv[]);
static int import_descend_dir (char *message, char *dir, char *vtag,
			       int targc, char *targv[]);
static int process_import_file (char *message, char *vfile, char *vtag,
				int targc, char *targv[]);
static int update_rcs_file (char *message, char *vfile, char *vtag, int targc,
			    char *targv[], int inattic);
#ifdef PRESERVE_PERMISSIONS_SUPPORT
static int preserve_initial_permissions (FILE *fprcs, const char *userfile,
					 mode_t file_type, struct stat *sbp);
#endif
static int expand_and_copy_contents (FILE *fprcs, mode_t file_type,
				     const char *user, FILE *fpuser);
static void add_log (int ch, char *fname);

static int repos_len;
static char *vhead;
static char *vbranch;
static FILE *logfp;
static char *repository;
static int conflicts;
static int use_file_modtime;
static char *keyword_opt = NULL;
static bool killnew;

static const char *const import_usage[] =
{
    "Usage: %s %s [-dX] [-k subst] [-I ign] [-m msg] [-b branch]\n",
    "    [-W spec] repository vendor-tag release-tags...\n",
    "\t-d\tUse the file's modification time as the time of import.\n",
    "\t-X\tWhen importing new files, mark their trunk revisions as dead.\n",
    "\t-k sub\tSet default RCS keyword substitution mode.\n",
    "\t-I ign\tMore files to ignore (! to reset).\n",
    "\t-b bra\tVendor branch id.\n",
    "\t-m msg\tLog message.\n",
    "\t-W spec\tWrappers specification line.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

int
import (int argc, char **argv)
{
    char *message = NULL;
    char *tmpfile;
    char *cp;
    int i, c, msglen, err;
    List *ulist;
    Node *p;
    struct logfile_info *li;

    if (argc == -1)
	usage (import_usage);

    /* Force -X behaviour or not based on the CVS repository
       CVSROOT/config setting.  */
#ifdef CLIENT_SUPPORT
    killnew = !current_parsed_root->isremote
	      && config->ImportNewFilesToVendorBranchOnly;
#else /* !CLIENT_SUPPORT */
    killnew = config->ImportNewFilesToVendorBranchOnly;
#endif /* CLIENT_SUPPORT */


    ign_setup ();
    wrap_setup ();

    vbranch = xstrdup (CVSBRANCH);
    getoptreset ();
    while ((c = getopt (argc, argv, "+Qqdb:m:I:k:W:X")) != -1)
    {
	switch (c)
	{
	    case 'Q':
	    case 'q':
		/* The CVS 1.5 client sends these options (in addition to
		   Global_option requests), so we must ignore them.  */
		if (!server_active)
		    error (1, 0,
			   "-q or -Q must be specified before \"%s\"",
			   cvs_cmd_name);
		break;
	    case 'd':
		if (server_active)
		{
		    /* CVS 1.10 and older clients will send this, but it
		       doesn't do any good.  So tell the user we can't
		       cope, rather than silently losing.  */
		    error (0, 0,
			   "warning: not setting the time of import from the file");
		    error (0, 0, "due to client limitations");
		}
		use_file_modtime = 1;
		break;
	    case 'b':
		free (vbranch);
		vbranch = xstrdup (optarg);
		break;
	    case 'm':
#ifdef FORCE_USE_EDITOR
		use_editor = 1;
#else
		use_editor = 0;
#endif
		if (message) free (message);
		message = xstrdup (optarg);
		break;
	    case 'I':
		ign_add (optarg, 0);
		break;
            case 'k':
		/* RCS_check_kflag returns strings of the form -kxx.  We
		   only use it for validation, so we can free the value
		   as soon as it is returned. */
		free (RCS_check_kflag (optarg));
		keyword_opt = optarg;
		break;
	    case 'W':
		wrap_add (optarg, 0);
		break;
	    case 'X':
		killnew = true;
		break;
	    case '?':
	    default:
		usage (import_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc < 3)
	usage (import_usage);

    /* This is for handling the Checkin-time request.  It might seem a
       bit odd to enable the use_file_modtime code even in the case
       where Checkin-time was not sent for a particular file.  The
       effect is that we use the time of upload, rather than the time
       when we call RCS_checkin.  Since those times are both during
       CVS's run, that seems OK, and it is easier to implement than
       putting the "was Checkin-time sent" flag in CVS/Entries or some
       such place.  */

    if (server_active)
	use_file_modtime = 1;

    /* Don't allow "CVS" as any directory in module path.
     *
     * Could abstract this to valid_module_path, but I don't think we'll need
     * to call it from anywhere else.
     */
    if ((cp = strstr (argv[0], "CVS")) &&   /* path contains "CVS" AND ... */
        ((cp == argv[0]) || ISSLASH (*(cp-1))) && /* /^CVS/ OR m#/CVS# AND ... */
        ((*(cp+3) == '\0') || ISSLASH (*(cp+3))) /* /CVS$/ OR m#CVS/# */
       )
    {
        error (0, 0,
               "The word `CVS' is reserved by CVS and may not be used");
        error (1, 0, "as a directory in a path or as a file name.");
    }

    for (i = 1; i < argc; i++)		/* check the tags for validity */
    {
	int j;

	RCS_check_tag (argv[i]);
	for (j = 1; j < i; j++)
	    if (strcmp (argv[j], argv[i]) == 0)
		error (1, 0, "tag `%s' was specified more than once", argv[i]);
    }

    if (ISABSOLUTE (argv[0]) || pathname_levels (argv[0]) > 0)
	/* It is somewhere between a security hole and "unexpected" to
	   let the client start mucking around outside the cvsroot
	   (wouldn't get the right CVSROOT configuration, &c).  */
	error (1, 0, "directory %s not relative within the repository",
	       argv[0]);

    if (current_parsed_root == NULL)
    {
	error (0, 0, "missing CVSROOT environment variable\n");
	error (1, 0, "Set it or specify the '-d' option to %s.",
	       program_name);
    }
    repository = Xasprintf ("%s/%s", current_parsed_root->directory, argv[0]);
    repos_len = strlen (current_parsed_root->directory);

    /*
     * Consistency checks on the specified vendor branch.  It must be
     * composed of only numbers and dots ('.').  Also, for now we only
     * support branching to a single level, so the specified vendor branch
     * must only have two dots in it (like "1.1.1").
     */
    {
	regex_t pat;
	int ret = regcomp (&pat, "^[1-9][0-9]*\\.[1-9][0-9]*\\.[1-9][0-9]*$",
			   REG_EXTENDED);
	assert (!ret);
	if (regexec (&pat, vbranch, 0, NULL, 0))
	{
	    error (1, 0,
"Only numeric branch specifications with two dots are\n"
"supported by import, not `%s'.  For example: `1.1.1'.",
		   vbranch);
	}
	regfree (&pat);
    }

    /* Set vhead to the branch's parent.  */
    vhead = xstrdup (vbranch);
    cp = strrchr (vhead, '.');
    *cp = '\0';

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	/* For rationale behind calling start_server before do_editor, see
	   commit.c  */
	start_server ();
    }
#endif

    if (!server_active && use_editor)
    {
	do_editor (NULL, &message,
		   current_parsed_root->isremote ? NULL : repository,
		   NULL);
    }
    msglen = message == NULL ? 0 : strlen (message);
    if (msglen == 0 || message[msglen - 1] != '\n')
    {
	char *nm = xmalloc (msglen + 2);
	*nm = '\0';
	if (message != NULL)
	{
	    (void) strcpy (nm, message);
	    free (message);
	}
	(void) strcat (nm + msglen, "\n");
	message = nm;
    }

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	int err;

	if (vbranch[0] != '\0')
	    option_with_arg ("-b", vbranch);
	option_with_arg ("-m", message ? message : "");
	if (keyword_opt != NULL)
	    option_with_arg ("-k", keyword_opt);
	if (killnew)
	    send_arg ("-X");
	/* The only ignore processing which takes place on the server side
	   is the CVSROOT/cvsignore file.  But if the user specified -I !,
	   the documented behavior is to not process said file.  */
	if (ign_inhibit_server)
	{
	    send_arg ("-I");
	    send_arg ("!");
	}
	wrap_send ();

	{
	    int i;
	    for (i = 0; i < argc; ++i)
		send_arg (argv[i]);
	}

	logfp = stdin;
	client_import_setup (repository);
	err = import_descend (message, argv[1], argc - 2, argv + 2);
	client_import_done ();
	if (message)
	    free (message);
	free (repository);
	free (vbranch);
	free (vhead);
	send_to_server ("import\012", 0);
	err += get_responses_and_close ();
	return err;
    }
#endif

    if (!safe_location (NULL))
    {
	error (1, 0, "attempt to import the repository");
    }

    ulist = getlist ();
    p = getnode ();
    p->type = UPDATE;
    p->delproc = update_delproc;
    p->key = xstrdup ("- Imported sources");
    li = xmalloc (sizeof (struct logfile_info));
    li->type = T_TITLE;
    li->tag = xstrdup (vbranch);
    li->rev_old = li->rev_new = NULL;
    p->data = li;
    (void) addnode (ulist, p);
    do_verify (&message, repository, ulist);

    /*
     * Make all newly created directories writable.  Should really use a more
     * sophisticated security mechanism here.
     */
    (void) umask (cvsumask);
    make_directories (repository);

    /* Create the logfile that will be logged upon completion */
    if ((logfp = cvs_temp_file (&tmpfile)) == NULL)
	error (1, errno, "cannot create temporary file `%s'", tmpfile);
    /* On systems where we can unlink an open file, do so, so it will go
       away no matter how we exit.  FIXME-maybe: Should be checking for
       errors but I'm not sure which error(s) we get if we are on a system
       where one can't unlink open files.  */
    (void) CVS_UNLINK (tmpfile);
    (void) fprintf (logfp, "\nVendor Tag:\t%s\n", argv[1]);
    (void) fprintf (logfp, "Release Tags:\t");
    for (i = 2; i < argc; i++)
	(void) fprintf (logfp, "%s\n\t\t", argv[i]);
    (void) fprintf (logfp, "\n");

    /* Just Do It.  */
    err = import_descend (message, argv[1], argc - 2, argv + 2);
    if (conflicts || killnew)
    {
	if (!really_quiet)
	{
	    char buf[20];

	    cvs_output_tagged ("+importmergecmd", NULL);
	    cvs_output_tagged ("newline", NULL);
	    if (conflicts)
	        sprintf (buf, "%d", conflicts);
	    else
	        strcpy (buf, "No");
	    cvs_output_tagged ("conflicts", buf);
	    cvs_output_tagged ("text", " conflicts created by this import.");
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("text",
			       "Use the following command to help the merge:");
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("text", "\t");
	    cvs_output_tagged ("text", program_name);
	    if (CVSroot_cmdline != NULL)
	    {
		cvs_output_tagged ("text", " -d ");
		cvs_output_tagged ("text", CVSroot_cmdline);
	    }
	    cvs_output_tagged ("text", " checkout -j");
	    cvs_output_tagged ("mergetag1", "<prev_rel_tag>");
	    cvs_output_tagged ("text", " -j");
	    cvs_output_tagged ("mergetag2", argv[2]);
	    cvs_output_tagged ("text", " ");
	    cvs_output_tagged ("repository", argv[0]);
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("-importmergecmd", NULL);
	}

	/* FIXME: I'm not sure whether we need to put this information
           into the loginfo.  If we do, then note that it does not
           report any required -d option.  There is no particularly
           clean way to tell the server about the -d option used by
           the client.  */
	if (conflicts)
	    (void) fprintf (logfp, "\n%d", conflicts);
	else
	    (void) fprintf (logfp, "\nNo");
	(void) fprintf (logfp, " conflicts created by this import.\n");
	(void) fprintf (logfp,
			"Use the following command to help the merge:\n\n");
	(void) fprintf (logfp, "\t%s checkout ", program_name);
	(void) fprintf (logfp, "-j%s:yesterday -j%s %s\n\n",
			argv[1], argv[1], argv[0]);
    }
    else
    {
	if (!really_quiet)
	    cvs_output ("\nNo conflicts created by this import\n\n", 0);
	(void) fprintf (logfp, "\nNo conflicts created by this import\n\n");
    }

    /*
     * Write out the logfile and clean up.
     */
    Update_Logfile (repository, message, logfp, ulist);
    dellist (&ulist);
    if (fclose (logfp) < 0)
	error (0, errno, "error closing %s", tmpfile);

    /* Make sure the temporary file goes away, even on systems that don't let
       you delete a file that's in use.  */
    if (CVS_UNLINK (tmpfile) < 0 && !existence_error (errno))
	error (0, errno, "cannot remove %s", tmpfile);
    free (tmpfile);

    if (message)
	free (message);
    free (repository);
    free (vbranch);
    free (vhead);

    return err;
}

/* Process all the files in ".", then descend into other directories.
   Returns 0 for success, or >0 on error (in which case a message
   will have been printed).  */
static int
import_descend (char *message, char *vtag, int targc, char **targv)
{
    DIR *dirp;
    struct dirent *dp;
    int err = 0;
    List *dirlist = NULL;

    /* first, load up any per-directory ignore lists */
    ign_add_file (CVSDOTIGNORE, 1);
    wrap_add_file (CVSDOTWRAPPER, 1);

    if (!current_parsed_root->isremote)
	lock_dir_for_write (repository);

    if ((dirp = CVS_OPENDIR (".")) == NULL)
    {
	error (0, errno, "cannot open directory");
	err++;
    }
    else
    {
	errno = 0;
	while ((dp = CVS_READDIR (dirp)) != NULL)
	{
	    if (strcmp (dp->d_name, ".") == 0 || strcmp (dp->d_name, "..") == 0)
		goto one_more_time_boys;

	    /* CVS directories are created in the temp directory by
	       server.c because it doesn't special-case import.  So
	       don't print a message about them, regardless of -I!.  */
	    if (server_active && strcmp (dp->d_name, CVSADM) == 0)
		goto one_more_time_boys;

	    if (ign_name (dp->d_name))
	    {
		add_log ('I', dp->d_name);
		goto one_more_time_boys;
	    }

	    if (
#ifdef DT_DIR
		(dp->d_type == DT_DIR
		 || (dp->d_type == DT_UNKNOWN && isdir (dp->d_name)))
#else
		isdir (dp->d_name)
#endif
		&& !wrap_name_has (dp->d_name, WRAP_TOCVS)
		)
	    {
		Node *n;

		if (dirlist == NULL)
		    dirlist = getlist ();

		n = getnode ();
		n->key = xstrdup (dp->d_name);
		addnode (dirlist, n);
	    }
	    else if (
#ifdef DT_DIR
		     dp->d_type == DT_LNK
		     || (dp->d_type == DT_UNKNOWN && islink (dp->d_name))
#else
		     islink (dp->d_name)
#endif
		     )
	    {
		add_log ('L', dp->d_name);
		err++;
	    }
	    else
	    {
#ifdef CLIENT_SUPPORT
		if (current_parsed_root->isremote)
		    err += client_process_import_file (message, dp->d_name,
                                                       vtag, targc, targv,
                                                       repository,
                                                       keyword_opt != NULL &&
						       keyword_opt[0] == 'b',
						       use_file_modtime);
		else
#endif
		    err += process_import_file (message, dp->d_name,
						vtag, targc, targv);
	    }
	one_more_time_boys:
	    errno = 0;
	}
	if (errno != 0)
	{
	    error (0, errno, "cannot read directory");
	    ++err;
	}
	(void) CVS_CLOSEDIR (dirp);
    }

    if (!current_parsed_root->isremote)
	Simple_Lock_Cleanup ();

    if (dirlist != NULL)
    {
	Node *head, *p;

	head = dirlist->list;
	for (p = head->next; p != head; p = p->next)
	{
	    err += import_descend_dir (message, p->key, vtag, targc, targv);
	}

	dellist (&dirlist);
    }

    return err;
}

/*
 * Process the argument import file.
 */
static int
process_import_file (char *message, char *vfile, char *vtag, int targc,
		     char **targv)
{
    char *rcs;
    int inattic = 0;

    rcs = Xasprintf ("%s/%s%s", repository, vfile, RCSEXT);
    if (!isfile (rcs))
    {
	char *attic_name;

	attic_name = xmalloc (strlen (repository) + strlen (vfile) +
			      sizeof (CVSATTIC) + sizeof (RCSEXT) + 10);
	(void) sprintf (attic_name, "%s/%s/%s%s", repository, CVSATTIC,
			vfile, RCSEXT);
	if (!isfile (attic_name))
	{
	    int retval;
	    char *free_opt = NULL;
	    char *our_opt = keyword_opt;

	    /* If marking newly-imported files as dead, they must be
	       created in the attic!  */
	    if (!killnew)
	        free (attic_name);
	    else 
	    {
		free (rcs);
		rcs = attic_name;

		/* Attempt to make the Attic directory, in case it
		   does not exist.  */
		(void) sprintf (rcs, "%s/%s", repository, CVSATTIC);
		if (CVS_MKDIR (rcs, 0777 ) != 0 && errno != EEXIST)
		    error (1, errno, "cannot make directory `%s'", rcs);

		/* Note that the above clobbered the path name, so we
		   recreate it here.  */
		(void) sprintf (rcs, "%s/%s/%s%s", repository, CVSATTIC,
				vfile, RCSEXT);
	    }

	    /*
	     * A new import source file; it doesn't exist as a ,v within the
	     * repository nor in the Attic -- create it anew.
	     */
	    add_log ('N', vfile);

#ifdef SERVER_SUPPORT
	    /* The most reliable information on whether the file is binary
	       is what the client told us.  That is because if the client had
	       the wrong idea about binaryness, it corrupted the file, so
	       we might as well believe the client.  */
	    if (server_active)
	    {
		Node *node;
		List *entries;

		/* Reading all the entries for each file is fairly silly, and
		   probably slow.  But I am too lazy at the moment to do
		   anything else.  */
		entries = Entries_Open (0, NULL);
		node = findnode_fn (entries, vfile);
		if (node != NULL)
		{
		    Entnode *entdata = node->data;

		    if (entdata->type == ENT_FILE)
		    {
			assert (entdata->options[0] == '-'
				&& entdata->options[1] == 'k');
			our_opt = xstrdup (entdata->options + 2);
			free_opt = our_opt;
		    }
		}
		Entries_Close (entries);
	    }
#endif

	    retval = add_rcs_file (message, rcs, vfile, vhead, our_opt,
				   vbranch, vtag, targc, targv,
				   NULL, 0, logfp, killnew);
	    if (free_opt != NULL)
		free (free_opt);
	    free (rcs);
	    return retval;
	}
	free (attic_name);
	inattic = 1;
    }

    free (rcs);
    /*
     * an rcs file exists. have to do things the official, slow, way.
     */
    return update_rcs_file (message, vfile, vtag, targc, targv, inattic);
}

/*
 * The RCS file exists; update it by adding the new import file to the
 * (possibly already existing) vendor branch.
 */
static int
update_rcs_file (char *message, char *vfile, char *vtag, int targc,
		 char **targv, int inattic)
{
    Vers_TS *vers;
    int letter;
    char *tocvsPath;
    char *expand;
    struct file_info finfo;

    memset (&finfo, 0, sizeof finfo);
    finfo.file = vfile;
    /* Not used, so don't worry about it.  */
    finfo.update_dir = NULL;
    finfo.fullname = finfo.file;
    finfo.repository = repository;
    finfo.entries = NULL;
    finfo.rcs = NULL;
    vers = Version_TS (&finfo, NULL, vbranch, NULL, 1, 0);
    if (vers->vn_rcs != NULL
	&& !RCS_isdead (vers->srcfile, vers->vn_rcs))
    {
	int different;

	/*
	 * The rcs file does have a revision on the vendor branch. Compare
	 * this revision with the import file; if they match exactly, there
	 * is no need to install the new import file as a new revision to the
	 * branch.  Just tag the revision with the new import tags.
	 * 
	 * This is to try to cut down the number of "C" conflict messages for
	 * locally modified import source files.
	 */
	tocvsPath = wrap_tocvs_process_file (vfile);
	/* FIXME: Why don't we pass tocvsPath to RCS_cmp_file if it is
           not NULL?  */
	expand = (vers->srcfile->expand != NULL
		  && vers->srcfile->expand[0] == 'b') ? "-kb" : "-ko";
	different = RCS_cmp_file (vers->srcfile, vers->vn_rcs, NULL,
	                          NULL, expand, vfile);
	if (tocvsPath)
	    if (unlink_file_dir (tocvsPath) < 0)
		error (0, errno, "cannot remove %s", tocvsPath);

	if (!different)
	{
	    int retval = 0;

	    /*
	     * The two files are identical.  Just update the tags, print the
	     * "U", signifying that the file has changed, but needs no
	     * attention, and we're done.
	     */
	    if (add_tags (vers->srcfile, vfile, vtag, targc, targv))
		retval = 1;
	    add_log ('U', vfile);
	    freevers_ts (&vers);
	    return retval;
	}
    }

    /* We may have failed to parse the RCS file; check just in case */
    if (vers->srcfile == NULL ||
	add_rev (message, vers->srcfile, vfile, vers->vn_rcs) ||
	add_tags (vers->srcfile, vfile, vtag, targc, targv))
    {
	freevers_ts (&vers);
	return 1;
    }

    if (vers->srcfile->branch == NULL || inattic ||
	strcmp (vers->srcfile->branch, vbranch) != 0)
    {
	conflicts++;
	letter = 'C';
    }
    else
	letter = 'U';
    add_log (letter, vfile);

    freevers_ts (&vers);
    return 0;
}

/*
 * Add the revision to the vendor branch
 */
static int
add_rev (char *message, RCSNode *rcs, char *vfile, char *vers)
{
    int locked, status, ierrno;
    char *tocvsPath;

    if (noexec)
	return 0;

    locked = 0;
    if (vers != NULL)
    {
	/* Before RCS_lock existed, we were directing stdout, as well as
	   stderr, from the RCS command, to DEVNULL.  I wouldn't guess that
	   was necessary, but I don't know for sure.  */
	/* Earlier versions of this function printed a `fork failed' error
	   when RCS_lock returned an error code.  That's not appropriate
	   now that RCS_lock is librarified, but should the error text be
	   preserved? */
	if (RCS_lock (rcs, vbranch, 1) != 0)
	    return 1;
	locked = 1;
	RCS_rewrite (rcs, NULL, NULL);
    }
    tocvsPath = wrap_tocvs_process_file (vfile);

    status = RCS_checkin (rcs, NULL, tocvsPath == NULL ? vfile : tocvsPath,
			  message, vbranch, 0,
			  (RCS_FLAGS_QUIET | RCS_FLAGS_KEEPFILE
			   | (use_file_modtime ? RCS_FLAGS_MODTIME : 0)));
    ierrno = errno;

    if ((tocvsPath != NULL) && (unlink_file_dir (tocvsPath) < 0))
	error (0, errno, "cannot remove %s", tocvsPath);

    if (status)
    {
	if (!noexec)
	{
	    fperrmsg (logfp, 0, status == -1 ? ierrno : 0,
		      "ERROR: Check-in of %s failed", rcs->path);
	    error (0, status == -1 ? ierrno : 0,
		   "ERROR: Check-in of %s failed", rcs->path);
	}
	if (locked)
	{
	    (void) RCS_unlock (rcs, vbranch, 0);
	    RCS_rewrite (rcs, NULL, NULL);
	}
	return 1;
    }
    return 0;
}

/*
 * Add the vendor branch tag and all the specified import release tags to the
 * RCS file.  The vendor branch tag goes on the branch root (1.1.1) while the
 * vendor release tags go on the newly added leaf of the branch (1.1.1.1,
 * 1.1.1.2, ...).
 */
static int
add_tags (RCSNode *rcs, char *vfile, char *vtag, int targc, char **targv)
{
    int i, ierrno;
    Vers_TS *vers;
    int retcode = 0;
    struct file_info finfo;

    if (noexec)
	return 0;

    if ((retcode = RCS_settag (rcs, vtag, vbranch)) != 0)
    {
	ierrno = errno;
	fperrmsg (logfp, 0, retcode == -1 ? ierrno : 0,
		  "ERROR: Failed to set tag %s in %s", vtag, rcs->path);
	error (0, retcode == -1 ? ierrno : 0,
	       "ERROR: Failed to set tag %s in %s", vtag, rcs->path);
	return 1;
    }
    RCS_rewrite (rcs, NULL, NULL);

    memset (&finfo, 0, sizeof finfo);
    finfo.file = vfile;
    /* Not used, so don't worry about it.  */
    finfo.update_dir = NULL;
    finfo.fullname = finfo.file;
    finfo.repository = repository;
    finfo.entries = NULL;
    finfo.rcs = NULL;
    vers = Version_TS (&finfo, NULL, vtag, NULL, 1, 0);
    for (i = 0; i < targc; i++)
    {
	if ((retcode = RCS_settag (rcs, targv[i], vers->vn_rcs)) == 0)
	    RCS_rewrite (rcs, NULL, NULL);
	else
	{
	    ierrno = errno;
	    fperrmsg (logfp, 0, retcode == -1 ? ierrno : 0,
		      "WARNING: Couldn't add tag %s to %s", targv[i],
		      rcs->path);
	    error (0, retcode == -1 ? ierrno : 0,
		   "WARNING: Couldn't add tag %s to %s", targv[i],
		   rcs->path);
	}
    }
    freevers_ts (&vers);
    return 0;
}

/*
 * Stolen from rcs/src/rcsfnms.c, and adapted/extended.
 */
struct compair
{
    char *suffix, *comlead;
};

static const struct compair comtable[] =
{

/*
 * comtable pairs each filename suffix with a comment leader. The comment
 * leader is placed before each line generated by the $Log keyword. This
 * table is used to guess the proper comment leader from the working file's
 * suffix during initial ci (see InitAdmin()). Comment leaders are needed for
 * languages without multiline comments; for others they are optional.
 *
 * I believe that the comment leader is unused if you are using RCS 5.7, which
 * decides what leader to use based on the text surrounding the $Log keyword
 * rather than a specified comment leader.
 */
    {"a", "-- "},			/* Ada		 */
    {"ada", "-- "},
    {"adb", "-- "},
    {"asm", ";; "},			/* assembler (MS-DOS) */
    {"ads", "-- "},			/* Ada		 */
    {"bas", "' "},    			/* Visual Basic code */
    {"bat", ":: "},			/* batch (MS-DOS) */
    {"body", "-- "},			/* Ada		 */
    {"c", " * "},			/* C		 */
    {"c++", "// "},			/* C++ in all its infinite guises */
    {"cc", "// "},
    {"cpp", "// "},
    {"cxx", "// "},
    {"m", "// "},			/* Objective-C */
    {"cl", ";;; "},			/* Common Lisp	 */
    {"cmd", ":: "},			/* command (OS/2) */
    {"cmf", "c "},			/* CM Fortran	 */
    {"cs", " * "},			/* C*		 */
    {"csh", "# "},			/* shell	 */
    {"dlg", " * "},   			/* MS Windows dialog file */
    {"e", "# "},			/* efl		 */
    {"epsf", "% "},			/* encapsulated postscript */
    {"epsi", "% "},			/* encapsulated postscript */
    {"el", "; "},			/* Emacs Lisp	 */
    {"f", "c "},			/* Fortran	 */
    {"for", "c "},
    {"frm", "' "},    			/* Visual Basic form */
    {"h", " * "},			/* C-header	 */
    {"hh", "// "},			/* C++ header	 */
    {"hpp", "// "},
    {"hxx", "// "},
    {"in", "# "},			/* for Makefile.in */
    {"l", " * "},			/* lex (conflict between lex and
					 * franzlisp) */
    {"mac", ";; "},			/* macro (DEC-10, MS-DOS, PDP-11,
					 * VMS, etc) */
    {"mak", "# "},    			/* makefile, e.g. Visual C++ */
    {"me", ".\\\" "},			/* me-macros	t/nroff	 */
    {"ml", "; "},			/* mocklisp	 */
    {"mm", ".\\\" "},			/* mm-macros	t/nroff	 */
    {"ms", ".\\\" "},			/* ms-macros	t/nroff	 */
    {"man", ".\\\" "},			/* man-macros	t/nroff	 */
    {"1", ".\\\" "},			/* feeble attempt at man pages... */
    {"2", ".\\\" "},
    {"3", ".\\\" "},
    {"4", ".\\\" "},
    {"5", ".\\\" "},
    {"6", ".\\\" "},
    {"7", ".\\\" "},
    {"8", ".\\\" "},
    {"9", ".\\\" "},
    {"p", " * "},			/* pascal	 */
    {"pas", " * "},
    {"pl", "# "},			/* perl	(conflict with Prolog) */
    {"ps", "% "},			/* postscript	 */
    {"psw", "% "},			/* postscript wrap */
    {"pswm", "% "},			/* postscript wrap */
    {"r", "# "},			/* ratfor	 */
    {"rc", " * "},			/* Microsoft Windows resource file */
    {"red", "% "},			/* psl/rlisp	 */
#ifdef sparc
    {"s", "! "},			/* assembler	 */
#endif
#ifdef mc68000
    {"s", "| "},			/* assembler	 */
#endif
#ifdef pdp11
    {"s", "/ "},			/* assembler	 */
#endif
#ifdef vax
    {"s", "# "},			/* assembler	 */
#endif
#ifdef __ksr__
    {"s", "# "},			/* assembler	 */
    {"S", "# "},			/* Macro assembler */
#endif
    {"sh", "# "},			/* shell	 */
    {"sl", "% "},			/* psl		 */
    {"spec", "-- "},			/* Ada		 */
    {"tex", "% "},			/* tex		 */
    {"y", " * "},			/* yacc		 */
    {"ye", " * "},			/* yacc-efl	 */
    {"yr", " * "},			/* yacc-ratfor	 */
    {"", "# "},				/* default for empty suffix	 */
    {NULL, "# "}			/* default for unknown suffix;	 */
/* must always be last		 */
};



static char *
get_comment (const char *user)
{
    char *cp, *suffix;
    char *suffix_path;
    int i;
    char *retval;

    suffix_path = xmalloc (strlen (user) + 5);
    cp = strrchr (user, '.');
    if (cp != NULL)
    {
	cp++;

	/*
	 * Convert to lower-case, since we are not concerned about the
	 * case-ness of the suffix.
	 */
	(void) strcpy (suffix_path, cp);
	for (cp = suffix_path; *cp; cp++)
	    if (isupper ((unsigned char) *cp))
		*cp = tolower (*cp);
	suffix = suffix_path;
    }
    else
	suffix = "";			/* will use the default */
    for (i = 0;; i++)
    {
	if (comtable[i].suffix == NULL)
	{
	    /* Default.  Note we'll always hit this case before we
	       ever return NULL.  */
	    retval = comtable[i].comlead;
	    break;
	}
	if (strcmp (suffix, comtable[i].suffix) == 0)
	{
	    retval = comtable[i].comlead;
	    break;
	}
    }
    free (suffix_path);
    return retval;
}

/* Create a new RCS file from scratch.
 *
 * This probably should be moved to rcs.c now that it is called from
 * places outside import.c.
 *
 * INPUTS
 *   message    Log message for the addition.  Not used if add_vhead == NULL.
 *   rcs        Filename of the RCS file to create.  Note that if 'do_killnew'
 *		is set, this file should be in the Attic directory, and the
 *		Attic directory must already exist.
 *   user       Filename of the file to serve as the contents of the initial
 *              revision.  Even if add_vhead is NULL, we use this to determine
 *              the modes to give the new RCS file.
 *   add_vhead  Revision number of head that we are adding.  Normally 1.1 but
 *              could be another revision as long as ADD_VBRANCH is a branch
 *              from it.  If NULL, then just add an empty file without any
 *              revisions (similar to the one created by "rcs -i").
 *   key_opt    Keyword expansion mode, e.g., "b" for binary.  NULL means the
 *              default behavior.
 *   add_vbranch
 *              Vendor branch to import to, or NULL if none.  If non-NULL, then
 *              vtag should also be non-NULL.
 *   vtag
 *   targc      Number of elements in TARGV.
 *   targv      The list of tags to attached to this imported revision.
 *   desctext   If non-NULL, description for the file.  If NULL, the
 *              description will be empty.
 *   desclen    The number of bytes in desctext.
 *   add_logfp  Write errors to here as well as via error (), or NULL if we
 *              should use only error ().
 *   do_killnew	Mark newly-imported files as being dead on the trunk, i.e.,
 *		as being imported only to the vendor branch.
 *
 * RETURNS
 *   Return value is 0 for success, or nonzero for failure (in which
 *   case an error message will have already been printed).
 */
int
add_rcs_file (const char *message, const char *rcs, const char *user,
              const char *add_vhead, const char *key_opt,
              const char *add_vbranch, const char *vtag, int targc,
              char **targv, const char *desctext, size_t desclen,
              FILE *add_logfp, bool do_killnew)
{
    FILE *fprcs, *fpuser;
    struct stat sb;
    struct tm *ftm;
    time_t now;
    char altdate1[MAXDATELEN];
    char *author;
    int i, ierrno, err = 0;
    mode_t mode;
    char *tocvsPath;
    const char *userfile;
    char *free_opt = NULL;
    mode_t file_type;
    char *dead_revision = NULL;

    if (noexec)
	return 0;

    if (do_killnew)
    {
	char *last_place;
	int last_number;

	/* If we are marking the newly imported file as dead, we must
	   have a head revision.  */
	if (add_vhead == NULL)
	    error (1, 0, "killing new file attempted when no head revision is being added");

	/* One extra byte for NUL, plus one for carry generated by adding
	   one to the last number in the add_vhead revision.  */
	dead_revision = xmalloc (strlen (add_vhead) + 2);
	strcpy (dead_revision, add_vhead);

	/* Find the loacation of the last number, which we will increment
	   and overwrite.  Note that this handles single numbers (w/o
	   dots), which is probably unnecessary.  */
	if ((last_place = strrchr (dead_revision, '.')) != NULL)
	    last_place++;
	else
	    last_place = dead_revision;
	last_number = atoi (last_place);
	if (++last_number <= 0)
	  error (1, 0, "invalid revision number %s", add_vhead);
	sprintf (last_place, "%d", last_number);
    }

    /* Note that as the code stands now, the -k option overrides any
       settings in wrappers (whether CVSROOT/cvswrappers, -W, or
       whatever).  Some have suggested this should be the other way
       around.  As far as I know the documentation doesn't say one way
       or the other.  Before making a change of this sort, should think
       about what is best, document it (in cvs.texinfo and NEWS), &c.  */

    if (key_opt == NULL)
    {
	if (wrap_name_has (user, WRAP_RCSOPTION))
	{
	    key_opt = free_opt = wrap_rcsoption (user, 0);
	}
    }

    tocvsPath = wrap_tocvs_process_file (user);
    userfile = (tocvsPath == NULL ? user : tocvsPath);

    /* Opening in text mode is probably never the right thing for the
       server (because the protocol encodes text files in a fashion
       which does not depend on what the client or server OS is, as
       documented in cvsclient.texi), but as long as the server just
       runs on unix it is a moot point.  */

    /* If PreservePermissions is set, then make sure that the file
       is a plain file before trying to open it.  Longstanding (although
       often unpopular) CVS behavior has been to follow symlinks, so we
       maintain that behavior if PreservePermissions is not on.

       NOTE: this error message used to be `cannot fstat', but is now
       `cannot lstat'.  I don't see a way around this, since we must
       stat the file before opening it. -twp */

    if (lstat (userfile, &sb) < 0)
    {
	/* not fatal, continue import */
	if (add_logfp != NULL)
	    fperrmsg (add_logfp, 0, errno,
			  "ERROR: cannot lstat file %s", userfile);
	error (0, errno, "cannot lstat file %s", userfile);
	goto read_error;
    }
    file_type = sb.st_mode & S_IFMT;

    fpuser = NULL;
    if (
#ifdef PRESERVE_PERMISSIONS_SUPPORT
	!config->preserve_perms ||
#endif /* PRESERVE_PERMISSIONS_SUPPORT */
	file_type == S_IFREG)
    {
	fpuser = CVS_FOPEN (userfile,
			    ((key_opt != NULL && strcmp (key_opt, "b") == 0)
			     ? "rb"
			     : "r")
	    );
	if (fpuser == NULL)
	{
	    /* not fatal, continue import */
	    if (add_logfp != NULL)
		fperrmsg (add_logfp, 0, errno,
			  "ERROR: cannot read file %s", userfile);
	    error (0, errno, "ERROR: cannot read file %s", userfile);
	    goto read_error;
	}
    }

    fprcs = CVS_FOPEN (rcs, "w+b");
    if (fprcs == NULL)
    {
	ierrno = errno;
	goto write_error_noclose;
    }

    /*
     * putadmin()
     */
    if (add_vhead != NULL)
    {
	if (fprintf (fprcs, "head     %s;\012",
	             do_killnew ? dead_revision : add_vhead) < 0)
	    goto write_error;
    }
    else
    {
	if (fprintf (fprcs, "head     ;\012") < 0)
	    goto write_error;
    }

    /* This sets the default branch.  If using the 'do_killnew' functionality,
       where imports don't show up until merged, no default branch should
       be set.  */
    if (add_vbranch != NULL && ! do_killnew)
    {
	if (fprintf (fprcs, "branch   %s;\012", add_vbranch) < 0)
	    goto write_error;
    }
    if (fprintf (fprcs, "access   ;\012") < 0 ||
	fprintf (fprcs, "symbols  ") < 0)
    {
	goto write_error;
    }

    for (i = targc - 1; i >= 0; i--)
    {
	/* RCS writes the symbols backwards */
	assert (add_vbranch != NULL);
	if (fprintf (fprcs, "%s:%s.1 ", targv[i], add_vbranch) < 0)
	    goto write_error;
    }

    if (add_vbranch != NULL)
    {
	if (fprintf (fprcs, "%s:%s", vtag, add_vbranch) < 0)
	    goto write_error;
    }
    if (fprintf (fprcs, ";\012") < 0)
	goto write_error;

    if (fprintf (fprcs, "locks    ; strict;\012") < 0 ||
	/* XXX - make sure @@ processing works in the RCS file */
	fprintf (fprcs, "comment  @%s@;\012", get_comment (user)) < 0)
    {
	goto write_error;
    }

    if (key_opt != NULL && strcmp (key_opt, "kv") != 0)
    {
	if (fprintf (fprcs, "expand   @%s@;\012", key_opt) < 0)
	{
	    goto write_error;
	}
    }

    if (fprintf (fprcs, "\012") < 0)
      goto write_error;

    /* Write the revision(s), with the date and author and so on
       (that is "delta" rather than "deltatext" from rcsfile(5)).  */

    if (use_file_modtime)
	now = sb.st_mtime;
    else
	(void) time (&now);
    ftm = gmtime (&now);
    (void) sprintf (altdate1, DATEFORM,
		    ftm->tm_year + (ftm->tm_year < 100 ? 0 : 1900),
		    ftm->tm_mon + 1, ftm->tm_mday, ftm->tm_hour,
		    ftm->tm_min, ftm->tm_sec);
    author = getcaller ();

    if (do_killnew)
    {
	if (fprintf (fprcs, "\012%s\012", dead_revision) < 0 ||
	fprintf (fprcs, "date     %s;  author %s;  state %s;\012",
		 altdate1, author, RCSDEAD) < 0)
	goto write_error;

	if (fprintf (fprcs, "branches;\012") < 0)
	    goto write_error;
	if (fprintf (fprcs, "next    %s;\012", add_vhead) < 0)
	    goto write_error;

	if (fprintf (fprcs, "commitid        %s;\012", global_session_id) < 0)
	    goto write_error;

#ifdef PRESERVE_PERMISSIONS_SUPPORT
	/* Store initial permissions if necessary. */
	if (config->preserve_perms)
	{
	    if (preserve_initial_permissions (fprcs, userfile,
					      file_type, sbp))
		goto write_error;
	}
#endif
    }

    if (add_vhead != NULL)
    {
	if (fprintf (fprcs, "\012%s\012", add_vhead) < 0 ||
	fprintf (fprcs, "date     %s;  author %s;  state Exp;\012",
		 altdate1, author) < 0)
	goto write_error;

	if (fprintf (fprcs, "branches") < 0)
	    goto write_error;
	if (add_vbranch != NULL)
	{
	    if (fprintf (fprcs, " %s.1", add_vbranch) < 0)
		goto write_error;
	}
	if (fprintf (fprcs, ";\012") < 0)
	    goto write_error;

	if (fprintf (fprcs, "next     ;\012") < 0)
	    goto write_error;

	if (fprintf (fprcs, "commitid        %s;\012", global_session_id) < 0)
	    goto write_error;

#ifdef PRESERVE_PERMISSIONS_SUPPORT
	/* Store initial permissions if necessary. */
	if (config->preserve_perms)
	{
	    if (preserve_initial_permissions (fprcs, userfile,
					      file_type, sbp))
		goto write_error;
	}
#endif

	if (add_vbranch != NULL)
	{
	    if (fprintf (fprcs, "\012%s.1\012", add_vbranch) < 0 ||
		fprintf (fprcs, "date     %s;  author %s;  state Exp;\012",
			 altdate1, author) < 0 ||
		fprintf (fprcs, "branches ;\012") < 0 ||
		fprintf (fprcs, "next     ;\012") < 0 ||
	        fprintf (fprcs, "commitid        %s;\012", global_session_id) < 0)
		goto write_error;

#ifdef PRESERVE_PERMISSIONS_SUPPORT
	    /* Store initial permissions if necessary. */
	    if (config->preserve_perms)
	    {
		if (preserve_initial_permissions (fprcs, userfile,
						  file_type, sbp))
		    goto write_error;
	    }
#endif

	    if (fprintf (fprcs, "\012") < 0)
		goto write_error;
	}
    }

    /* Now write the description (possibly empty).  */
    if (fprintf (fprcs, "\012desc\012") < 0 ||
	fprintf (fprcs, "@") < 0)
	goto write_error;
    if (desctext != NULL)
    {
	/* The use of off_t not size_t for the second argument is very
	   strange, since we are dealing with something which definitely
	   fits in memory.  */
	if (expand_at_signs (desctext, (off_t) desclen, fprcs) < 0)
	    goto write_error;
    }
    if (fprintf (fprcs, "@\012\012\012") < 0)
	goto write_error;

    /* Now write the log messages and contents for the revision(s) (that
       is, "deltatext" rather than "delta" from rcsfile(5)).  */

    if (do_killnew)
    {
	if (fprintf (fprcs, "\012%s\012", dead_revision) < 0 ||
	    fprintf (fprcs, "log\012@") < 0)
	    goto write_error;
	if (fprintf (fprcs, "Revision %s was added on the vendor branch.\012",
		     add_vhead) < 0)
	    goto write_error;
	if (fprintf (fprcs, "@\012") < 0 ||
	    fprintf (fprcs, "text\012@") < 0)
	{
	    goto write_error;
	}

	/* Now copy over the contents of the file, expanding at signs.  */
	if (expand_and_copy_contents (fprcs, file_type, user, fpuser))
	    goto write_error;

	if (fprintf (fprcs, "@\012\012") < 0)
	    goto write_error;
    }

    if (add_vhead != NULL)
    {
	if (fprintf (fprcs, "\012%s\012", add_vhead) < 0 ||
	    fprintf (fprcs, "log\012@") < 0)
	    goto write_error;
	if (add_vbranch != NULL)
	{
	    /* We are going to put the log message in the revision on the
	       branch.  So putting it here too seems kind of redundant, I
	       guess (and that is what CVS has always done, anyway).  */
	    if (fprintf (fprcs, "Initial revision\012") < 0)
		goto write_error;
	}
	else
	{
	    if (expand_at_signs (message, (off_t) strlen (message), fprcs) < 0)
		goto write_error;
	}
	if (fprintf (fprcs, "@\012") < 0 ||
	    fprintf (fprcs, "text\012@") < 0)
	{
	    goto write_error;
	}

	/* Now copy over the contents of the file, expanding at signs.
	 * If config->preserve_perms is set, do this only for regular files.
	 */
	if (!do_killnew)
	{
            /* Now copy over the contents of the file, expanding at signs,
	       if not done as part of do_killnew handling above.  */
	    if (expand_and_copy_contents (fprcs, file_type, user, fpuser))
	        goto write_error;
	}

	if (fprintf (fprcs, "@\012\012") < 0)
	    goto write_error;

	if (add_vbranch != NULL)
	{
	    if (fprintf (fprcs, "\012%s.1\012", add_vbranch) < 0 ||
		fprintf (fprcs, "log\012@") < 0 ||
		expand_at_signs (message,
				 (off_t) strlen (message), fprcs) < 0 ||
		fprintf (fprcs, "@\012text\012") < 0 ||
		fprintf (fprcs, "@@\012") < 0)
		goto write_error;
	}
    }

    if (fclose (fprcs) == EOF)
    {
	ierrno = errno;
	goto write_error_noclose;
    }
    /* Close fpuser only if we opened it to begin with. */
    if (fpuser != NULL)
    {
	if (fclose (fpuser) < 0)
	    error (0, errno, "cannot close %s", user);
    }

    /*
     * Fix the modes on the RCS files.  The user modes of the original
     * user file are propagated to the group and other modes as allowed
     * by the repository umask, except that all write permissions are
     * turned off.
     */
    mode = (sb.st_mode |
	    (sb.st_mode & S_IRWXU) >> 3 |
	    (sb.st_mode & S_IRWXU) >> 6) &
	   ~cvsumask &
	   ~(S_IWRITE | S_IWGRP | S_IWOTH);
    if (chmod (rcs, mode) < 0)
    {
	ierrno = errno;
	if (add_logfp != NULL)
	    fperrmsg (add_logfp, 0, ierrno,
		      "WARNING: cannot change mode of file %s", rcs);
	error (0, ierrno, "WARNING: cannot change mode of file %s", rcs);
	err++;
    }
    if (tocvsPath)
	if (unlink_file_dir (tocvsPath) < 0)
		error (0, errno, "cannot remove %s", tocvsPath);
    if (free_opt != NULL)
	free (free_opt);
    return err;

write_error:
    ierrno = errno;
    if (fclose (fprcs) < 0)
	error (0, errno, "cannot close %s", rcs);
write_error_noclose:
    if (fclose (fpuser) < 0)
	error (0, errno, "cannot close %s", user);
    if (add_logfp != NULL)
	fperrmsg (add_logfp, 0, ierrno, "ERROR: cannot write file %s", rcs);
    error (0, ierrno, "ERROR: cannot write file %s", rcs);
    if (ierrno == ENOSPC)
    {
	if (CVS_UNLINK (rcs) < 0)
	    error (0, errno, "cannot remove %s", rcs);
	if (add_logfp != NULL)
	    fperrmsg (add_logfp, 0, 0, "ERROR: out of space - aborting");
	error (1, 0, "ERROR: out of space - aborting");
    }
read_error:
    if (tocvsPath)
	if (unlink_file_dir (tocvsPath) < 0)
	    error (0, errno, "cannot remove %s", tocvsPath);

    if (free_opt != NULL)
	free (free_opt);

    return err + 1;
}

#ifdef PRESERVE_PERMISSIONS_SUPPORT
/* Write file permissions and symlink information for a file being
 * added into its RCS file.
 *
 * INPUTS
 *   fprcs	FILE pointer for the (newly-created) RCS file.  Permisisons
 *		and symlink information should be written here.
 *   userfile	Filename of the file being added.  (Used to read symbolic
 *		link contents, for symlinks.)
 *   file_type	File type of userfile, extracted from sbp->st_mode.
 *   sbp	'stat' information for userfile.
 *
 * RETURNS
 *   Return value is 0 for success, or nonzero for failure (in which case
 *   no error message has yet been printed).
 */
static int
preserve_initial_permissions (fprcs, userfile, file_type, sbp)
    FILE *fprcs;
    const char *userfile;
    mode_t file_type;
    struct stat *sbp;
{
    if (file_type == S_IFLNK)
    {
	char *link = Xreadlink (userfile, sbp->st_size);
	if (fprintf (fprcs, "symlink\t@") < 0 ||
	    expand_at_signs (link, strlen (link), fprcs) < 0 ||
	    fprintf (fprcs, "@;\012") < 0)
	    goto write_error;
	free (link);
    }
    else
    {
	if (fprintf (fprcs, "owner\t%u;\012", sbp->st_uid) < 0)
	    goto write_error;
	if (fprintf (fprcs, "group\t%u;\012", sbp->st_gid) < 0)
	    goto write_error;
	if (fprintf (fprcs, "permissions\t%o;\012",
		     sbp->st_mode & 07777) < 0)
	    goto write_error;
	switch (file_type)
	{
	    case S_IFREG: break;
	    case S_IFCHR:
	    case S_IFBLK:
#ifdef HAVE_STRUCT_STAT_ST_RDEV
		if (fprintf (fprcs, "special\t%s %lu;\012",
			     (file_type == S_IFCHR
			      ? "character"
			      : "block"),
			     (unsigned long) sbp->st_rdev) < 0)
		    goto write_error;
#else
		error (0, 0,
"can't import %s: unable to import device files on this system",
userfile);
#endif
		break;
	    default:
		error (0, 0,
		       "can't import %s: unknown kind of special file",
		       userfile);
	}
    }
    return 0;

write_error:
    return 1;
}
#endif /* PRESERVE_PERMISSIONS_SUPPORT */

/* Copy file contents into an RCS file, expanding at signs.
 *
 * If config->preserve_perms is set, nothing is copied if the source is not
 * a regular file.
 *
 * INPUTS
 *   fprcs	FILE pointer for the (newly-created) RCS file.  The expanded
 *		contents should be written here.
 *   file_type	File type of the data source.  No data is copied if
 *		preserve_permissions is set and the source is not a
 *		regular file.
 *   user	Filename of the data source (used to print error messages).
 *   fpuser	FILE pointer for the data source, whose data is being
 *		copied into the RCS file.
 *
 * RETURNS
 *   Return value is 0 for success, or nonzero for failure (in which case
 *   no error message has yet been printed).
 */
static int
expand_and_copy_contents (fprcs, file_type, user, fpuser)
    FILE *fprcs, *fpuser;
    mode_t file_type;
    const char *user;
{
    if (
#ifdef PRESERVE_PERMISSIONS_SUPPORT
	!config->preserve_perms ||
#endif /* PRESERVE_PERMISSIONS_SUPPORT */
	file_type == S_IFREG)
    {
	char buf[8192];
	unsigned int len;

	while (1)
	{
	    len = fread (buf, 1, sizeof buf, fpuser);
	    if (len == 0)
	    {
		if (ferror (fpuser))
		    error (1, errno, "cannot read file %s for copying",
			   user);
		break;
	    }
	    if (expand_at_signs (buf, len, fprcs) < 0)
		goto write_error;
	}
    }
    return 0;

write_error:
    return 1;
}

/*
 * Write SIZE bytes at BUF to FP, expanding @ signs into double @
 * signs.  If an error occurs, return a negative value and set errno
 * to indicate the error.  If not, return a nonnegative value.
 */
int
expand_at_signs (const char *buf, size_t size, FILE *fp)
{
    register const char *cp, *next;

    cp = buf;
    while ((next = memchr (cp, '@', size)) != NULL)
    {
	size_t len = ++next - cp;
	if (fwrite (cp, 1, len, fp) != len)
	    return EOF;
	if (putc ('@', fp) == EOF)
	    return EOF;
	cp = next;
	size -= len;
    }

    if (fwrite (cp, 1, size, fp) != size)
	return EOF;

    return 1;
}

/*
 * Write an update message to (potentially) the screen and the log file.
 */
static void
add_log (int ch, char *fname)
{
    if (!really_quiet)			/* write to terminal */
    {
	char buf[2];
	buf[0] = ch;
	buf[1] = ' ';
	cvs_output (buf, 2);
	if (repos_len)
	{
	    cvs_output (repository + repos_len + 1, 0);
	    cvs_output ("/", 1);
	}
	else if (repository[0] != '\0')
	{
	    cvs_output (repository, 0);
	    cvs_output ("/", 1);
	}
	cvs_output (fname, 0);
	cvs_output ("\n", 1);
    }

    if (repos_len)			/* write to logfile */
	(void) fprintf (logfp, "%c %s/%s\n", ch,
			repository + repos_len + 1, fname);
    else if (repository[0])
	(void) fprintf (logfp, "%c %s/%s\n", ch, repository, fname);
    else
	(void) fprintf (logfp, "%c %s\n", ch, fname);
}

/*
 * This is the recursive function that walks the argument directory looking
 * for sub-directories that have CVS administration files in them and updates
 * them recursively.
 * 
 * Note that we do not follow symbolic links here, which is a feature!
 */
static int
import_descend_dir (char *message, char *dir, char *vtag, int targc,
		    char **targv)
{
    struct saved_cwd cwd;
    char *cp;
    int ierrno, err;
    char *rcs = NULL;

    if (islink (dir))
	return 0;
    if (save_cwd (&cwd))
    {
	fperrmsg (logfp, 0, errno, "Failed to save current directory.");
	return 1;
    }

    /* Concatenate DIR to the end of REPOSITORY.  */
    if (repository[0] == '\0')
    {
	char *new = xstrdup (dir);
	free (repository);
	repository = new;
    }
    else
    {
	char *new = Xasprintf ("%s/%s", repository, dir);
	free (repository);
	repository = new;
    }

    if (!quiet && !current_parsed_root->isremote)
	error (0, 0, "Importing %s", repository);

    if (CVS_CHDIR (dir) < 0)
    {
	ierrno = errno;
	fperrmsg (logfp, 0, ierrno, "ERROR: cannot chdir to %s", repository);
	error (0, ierrno, "ERROR: cannot chdir to %s", repository);
	err = 1;
	goto out;
    }
    if (!current_parsed_root->isremote && !isdir (repository))
    {
	rcs = Xasprintf ("%s%s", repository, RCSEXT);
	if (isfile (repository) || isfile (rcs))
	{
	    fperrmsg (logfp, 0, 0,
		      "ERROR: %s is a file, should be a directory!",
		      repository);
	    error (0, 0, "ERROR: %s is a file, should be a directory!",
		   repository);
	    err = 1;
	    goto out;
	}
	if (noexec == 0 && CVS_MKDIR (repository, 0777) < 0)
	{
	    ierrno = errno;
	    fperrmsg (logfp, 0, ierrno,
		      "ERROR: cannot mkdir %s -- not added", repository);
	    error (0, ierrno,
		   "ERROR: cannot mkdir %s -- not added", repository);
	    err = 1;
	    goto out;
	}
    }
    err = import_descend (message, vtag, targc, targv);
  out:
    if (rcs != NULL)
	free (rcs);
    if ((cp = strrchr (repository, '/')) != NULL)
	*cp = '\0';
    else
	repository[0] = '\0';
    if (restore_cwd (&cwd))
	error (1, errno, "Failed to restore current directory, `%s'.",
	       cwd.name);
    free_cwd (&cwd);
    return err;
}
