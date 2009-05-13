/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * Copyright (c) 2001, Tony Hoyle
 * Copyright (c) 2004, Derek R. Price & Ximbiot <http://ximbiot.com>
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 *
 * Query CVS/Entries from server
 */

#include "cvs.h"
#include <stdbool.h>

static int ls_proc (int argc, char **argv, char *xwhere, char *mwhere,
                    char *mfile, int shorten, int local, char *mname,
                    char *msg);

static const char *const ls_usage[] =
{
    "Usage: %s %s [-e | -l] [-RP] [-r rev] [-D date] [path...]\n",
    "\t-d\tShow dead revisions (with tag when specified).\n",
    "\t-e\tDisplay in CVS/Entries format.\n",
    "\t-l\tDisplay all details.\n",
    "\t-P\tPrune empty directories.\n",
    "\t-R\tList recursively.\n",
    "\t-r rev\tShow files with revision or tag.\n",
    "\t-D date\tShow files from date.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

static bool entries_format;
static bool long_format;
static char *show_tag;
static char *show_date;
static bool set_tag;
static char *created_dir;
static bool tag_validated;
static bool recurse;
static bool ls_prune_dirs;
static char *regexp_match;
static bool is_rls;
static bool show_dead_revs;



int
ls (int argc, char **argv)
{
    int c;
    int err = 0;

    is_rls = strcmp (cvs_cmd_name, "rls") == 0;

    if (argc == -1)
	usage (ls_usage);

    entries_format = false;
    long_format = false;
    show_tag = NULL;
    show_date = NULL;
    tag_validated = false;
    recurse = false;
    ls_prune_dirs = false;
    show_dead_revs = false;

    getoptreset ();

    while ((c = getopt (argc, argv,
#ifdef SERVER_SUPPORT
           server_active ? "qdelr:D:PR" :
#endif /* SERVER_SUPPORT */
           "delr:D:RP"
           )) != -1)
    {
	switch (c)
	{
#ifdef SERVER_SUPPORT
	    case 'q':
		if (server_active)
		{
		    error (0, 0,
"`%s ls -q' is deprecated.  Please use the global `-q' option instead.",
                           program_name);
		    quiet = true;
		}
		else
		    usage (ls_usage);
		break;
#endif /* SERVER_SUPPORT */
	    case 'd':
		show_dead_revs = true;
		break;
	    case 'e':
		entries_format = true;
		break;
	    case 'l':
		long_format = true;
		break;
	    case 'r':
		parse_tagdate (&show_tag, &show_date, optarg);
		break;
	    case 'D':
		if (show_date) free (show_date);
		show_date = Make_Date (optarg);
		break;
	    case 'P':
		ls_prune_dirs = true;
		break;
	    case 'R':
		recurse = true;
		break;
	    case '?':
	    default:
		usage (ls_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    if (entries_format && long_format)
    {
        error (0, 0, "`-e' & `-l' are mutually exclusive.");
        usage (ls_usage);
    }

    wrap_setup ();

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	/* We're the local client.  Fire up the remote server.	*/
	start_server ();

	ign_setup ();

	if (is_rls ? !(supported_request ("rlist") || supported_request ("ls"))
                   : !supported_request ("list"))
	    error (1, 0, "server does not support %s", cvs_cmd_name);

	if (quiet && !supported_request ("global-list-quiet"))
	    send_arg ("-q");
	if (entries_format)
	    send_arg ("-e");
	if (long_format)
	    send_arg ("-l");
	if (ls_prune_dirs)
	    send_arg ("-P");
	if (recurse)
	    send_arg ("-R");
	if (show_dead_revs)
	    send_arg ("-d");
	if (show_tag)
	    option_with_arg ("-r", show_tag);
	if (show_date)
	    client_senddate (show_date);

	send_arg ("--");

	if (is_rls)
	{
	    int i;
	    for (i = 0; i < argc; i++)
		send_arg (argv[i]);
            if (supported_request ("rlist"))
		send_to_server ("rlist\012", 0);
	    else
		/* For backwards compatibility with CVSNT...  */
		send_to_server ("ls\012", 0);
	}
	else
	{
	    /* Setting this means, I think, that any empty directories created
	     * by the server will be deleted by the client.  Since any dirs
	     * created at all by ls should remain empty, this should cause any
	     * dirs created by the server for the ls command to be deleted.
	     */
	    client_prune_dirs = 1;

	    /* I explicitly decide not to send contents here.  We *could* let
	     * the user pull status information with this command, but why
	     * don't they just use update or status?
	     */
	    send_files (argc, argv, !recurse, 0, SEND_NO_CONTENTS);
	    send_file_names (argc, argv, SEND_EXPAND_WILD);
	    send_to_server ("list\012", 0);
	}

	err = get_responses_and_close ();
	return err;
    }
#endif

    if (is_rls)
    {
	DBM *db;
	int i;
	db = open_module ();
	if (argc)
	{
	    for (i = 0; i < argc; i++)
	    {
		char *mod = xstrdup (argv[i]);
		char *p;

		for (p=strchr (mod,'\\'); p; p=strchr (p,'\\'))
		    *p='/';

		p = strrchr (mod,'/');
		if (p && (strchr (p,'?') || strchr (p,'*')))
		{
		    *p='\0';
		    regexp_match = p+1;
		}
		else
		    regexp_match = NULL;

		/* Frontends like to do 'ls -q /', so we support it explicitly.
                 */
		if (!strcmp (mod,"/"))
		{
		    *mod='\0';
		}

		err += do_module (db, mod, MISC, "Listing",
				  ls_proc, NULL, 0, 0, 0, 0, NULL);

		free (mod);
	    }
	}
	else
	{
	    /* should be ".", but do_recursion() 
	       fails this: assert ( strstr ( repository, "/./" ) == NULL ); */
	    char *topmod = xstrdup ("");
	    err += do_module (db, topmod, MISC, "Listing",
			      ls_proc, NULL, 0, 0, 0, 0, NULL);
	    free (topmod);
	}
	close_module (db);
    }
    else
	ls_proc (argc + 1, argv - 1, NULL, NULL, NULL, 0, 0, NULL, NULL);

    return err;
}



struct long_format_data
{
    char *header;
    char *time;
    char *footer;
};

static int
ls_print (Node *p, void *closure)
{
    if (long_format)
    {
	struct long_format_data *data = p->data;
	cvs_output_tagged ("text", data->header);
	if (data->time)
	    cvs_output_tagged ("date", data->time);
	if (data->footer)
	    cvs_output_tagged ("text", data->footer);
	cvs_output_tagged ("newline", NULL);
    }
    else
	cvs_output (p->data, 0);
    return 0;
}



static int
ls_print_dir (Node *p, void *closure)
{
    static bool printed = false;

    if (recurse && !(ls_prune_dirs && list_isempty (p->data)))
    {
        /* Keep track of whether we've printed.  If we have, then put a blank
         * line before directory headers, to separate the header from the
         * listing of the previous directory.
         */
        if (printed)
            cvs_output ("\n", 1);
        else
            printed = true;

        if (!strcmp (p->key, ""))
            cvs_output (".", 1);
        else
	    cvs_output (p->key, 0);
        cvs_output (":\n", 2);
    }
    walklist (p->data, ls_print, NULL);
    return 0;
}



/*
 * Delproc for a node containing a struct long_format_data as data.
 */
static void
long_format_data_delproc (Node *n)
{
	struct long_format_data *data = (struct long_format_data *)n->data;
	if (data->header) free (data->header);
	if (data->time) free (data->time);
	if (data->footer) free (data->footer);
	free (data);
}



/* A delproc for a List Node containing a List *.  */
static void
ls_delproc (Node *p)
{
    dellist ((List **)&p->data);
}



/*
 * Add a file to our list of data to print for a directory.
 */
/* ARGSUSED */
static int
ls_fileproc (void *callerdat, struct file_info *finfo)
{
    Vers_TS *vers;
    char *regex_err;
    Node *p, *n;
    bool isdead;
    const char *filename;

    if (regexp_match)
    {
#ifdef FILENAMES_CASE_INSENSITIVE
	  re_set_syntax (REG_ICASE|RE_SYNTAX_EGREP);
#else
	  re_set_syntax (RE_SYNTAX_EGREP);
#endif
	  if ((regex_err = re_comp (regexp_match)) != NULL)
	  {
	      error (1, 0, "bad regular expression passed to 'ls': %s",
                     regex_err);
	  }
	  if (re_exec (finfo->file) == 0)
	      return 0;				/* no match */
    }

    vers = Version_TS (finfo, NULL, show_tag, show_date, 1, 0);
    /* Skip dead revisions unless specifically requested to do otherwise.
     * We also bother to check for long_format so we can print the state.
     */
    if (vers->vn_rcs && (!show_dead_revs || long_format))
	isdead = RCS_isdead (finfo->rcs, vers->vn_rcs);
    else
	isdead = false;
    if (!vers->vn_rcs || (!show_dead_revs && isdead))
    {
        freevers_ts (&vers);
	return 0;
    }

    p = findnode (callerdat, finfo->update_dir);
    if (!p)
    {
	/* This only occurs when a complete path to a file is specified on the
	 * command line.  Put the file in the root list.
	 */
	filename = finfo->fullname;

	/* Add update_dir node.  */
	p = findnode (callerdat, ".");
	if (!p)
	{
	    p = getnode ();
	    p->key = xstrdup (".");
	    p->data = getlist ();
	    p->delproc = ls_delproc;
	    addnode (callerdat, p);
	}
    }
    else
	filename = finfo->file;

    n = getnode();
    if (entries_format)
    {
	char *outdate = entries_time (RCS_getrevtime (finfo->rcs, vers->vn_rcs,
                                                      0, 0));
	n->data = Xasprintf ("/%s/%s/%s/%s/%s%s\n",
                             filename, vers->vn_rcs,
                             outdate, vers->options,
                             show_tag ? "T" : "", show_tag ? show_tag : "");
	free (outdate);
    }
    else if (long_format)
    {
	struct long_format_data *out =
		xmalloc (sizeof (struct long_format_data));
	out->header = Xasprintf ("%-5.5s",
                                 vers->options[0] != '\0' ? vers->options
                                                          : "----");
	/* FIXME: Do we want to mimc the real `ls' command's date format?  */
	out->time = gmformat_time_t (RCS_getrevtime (finfo->rcs, vers->vn_rcs,
                                                     0, 0));
	out->footer = Xasprintf (" %-9.9s%s %s%s", vers->vn_rcs,
                                 strlen (vers->vn_rcs) > 9 ? "+" : " ",
                                 show_dead_revs ? (isdead ? "dead " : "     ")
                                                : "",
                                 filename);
	n->data = out;
	n->delproc = long_format_data_delproc;
    }
    else
	n->data = Xasprintf ("%s\n", filename);

    addnode (p->data, n);

    freevers_ts (&vers);
    return 0;
}



/*
 * Add this directory to the list of data to be printed for a directory and
 * decide whether to tell the recursion processor whether to continue
 * recursing or not.
 */
static Dtype
ls_direntproc (void *callerdat, const char *dir, const char *repos,
               const char *update_dir, List *entries)
{
    Dtype retval;
    Node *p;

    /* Due to the way we called start_recursion() from ls_proc() with a single
     * argument at a time, we can assume that if we don't yet have a parent
     * directory in DIRS then this directory should be processed.
     */

    if (strcmp (dir, "."))
    {
        /* Search for our parent directory.  */
	char *parent;
        parent = xmalloc (strlen (update_dir) - strlen (dir) + 1);
        strncpy (parent, update_dir, strlen (update_dir) - strlen (dir));
        parent[strlen (update_dir) - strlen (dir)] = '\0';
        strip_trailing_slashes (parent);
        p = findnode (callerdat, parent);
    }
    else
        p = NULL;

    if (p)
    {
	/* Push this dir onto our parent directory's listing.  */
	Node *n = getnode();

	if (entries_format)
	    n->data = Xasprintf ("D/%s////\n", dir);
	else if (long_format)
	{
	    struct long_format_data *out =
		    xmalloc (sizeof (struct long_format_data));
	    out->header = xstrdup ("d--- ");
	    out->time = gmformat_time_t (unix_time_stamp (repos));
	    out->footer = Xasprintf ("%12s%s%s", "",
                                     show_dead_revs ? "     " : "", dir);
	    n->data = out;
	    n->delproc = long_format_data_delproc;
	}
	else
	    n->data = Xasprintf ("%s\n", dir);

	addnode (p->data, n);
    }

    if (!p || recurse)
    {
	/* Create a new list for this directory.  */
	p = getnode ();
	p->key = xstrdup (strcmp (update_dir, ".") ? update_dir : "");
	p->data = getlist ();
        p->delproc = ls_delproc;
	addnode (callerdat, p);

	/* Create a local directory and mark it as needing deletion.  This is
         * the behavior the recursion processor relies upon, a la update &
         * checkout.
         */
	if (!isdir (dir))
        {
	    int nonbranch;
	    if (show_tag == NULL && show_date == NULL)
	    {
		ParseTag (&show_tag, &show_date, &nonbranch);
		set_tag = true;
	    }

	    if (!created_dir)
		created_dir = xstrdup (update_dir);

	    make_directory (dir);
	    Create_Admin (dir, update_dir, repos, show_tag, show_date,
			  nonbranch, 0, 0);
	    Subdir_Register (entries, NULL, dir);
	}

	/* Tell do_recursion to keep going.  */
	retval = R_PROCESS;
    }
    else
        retval = R_SKIP_ALL;

    return retval;
}



/* Clean up tags, dates, and dirs if we created this directory.
 */
static int
ls_dirleaveproc (void *callerdat, const char *dir, int err,
                 const char *update_dir, List *entries)
{
	if (created_dir && !strcmp (created_dir, update_dir))
	{
		if (set_tag)
		{
		    if (show_tag) free (show_tag);
		    if (show_date) free (show_date);
		    show_tag = show_date = NULL;
		    set_tag = false;
		}

		(void)CVS_CHDIR ("..");
		if (unlink_file_dir (dir))
		    error (0, errno, "Failed to remove directory `%s'",
			   created_dir);
		Subdir_Deregister (entries, NULL, dir);

		free (created_dir);
		created_dir = NULL;
	}
	return err;
}



static int
ls_proc (int argc, char **argv, char *xwhere, char *mwhere, char *mfile,
         int shorten, int local, char *mname, char *msg)
{
    char *repository;
    int err = 0;
    int which;
    char *where;
    int i;

    if (is_rls)
    {
	char *myargv[2];

	if (!quiet)
	    error (0, 0, "Listing module: `%s'",
	           strcmp (mname, "") ? mname : ".");

	repository = xmalloc (strlen (current_parsed_root->directory)
			      + strlen (argv[0])
			      + (mfile == NULL ? 0 : strlen (mfile) + 1)
			      + 2);
	(void)sprintf (repository, "%s/%s", current_parsed_root->directory,
		       argv[0]);
	where = xmalloc (strlen (argv[0])
			 + (mfile == NULL ? 0 : strlen (mfile) + 1)
			 + 1);
	(void)strcpy (where, argv[0]);

	/* If mfile isn't null, we need to set up to do only part of the
	 * module.
	 */
	if (mfile != NULL)
	{
	    char *cp;
	    char *path;

	    /* If the portion of the module is a path, put the dir part on
	     * repos.
	     */
	    if ((cp = strrchr (mfile, '/')) != NULL)
	    {
		*cp = '\0';
		(void)strcat (repository, "/");
		(void)strcat (repository, mfile);
		(void)strcat (where, "/");
		(void)strcat (where, mfile);
		mfile = cp + 1;
	    }

	    /* take care of the rest */
	    path = Xasprintf ("%s/%s", repository, mfile);
	    if (isdir (path))
	    {
		/* directory means repository gets the dir tacked on */
		(void)strcpy (repository, path);
		(void)strcat (where, "/");
		(void)strcat (where, mfile);
	    }
	    else
	    {
		myargv[1] = mfile;
		argc = 2;
		argv = myargv;
	    }
	    free (path);
	}

	/* cd to the starting repository */
	if (CVS_CHDIR (repository) < 0)
	{
	    error (0, errno, "cannot chdir to %s", repository);
	    free (repository);
	    free (where);
	    return 1;
	}

	which = W_REPOS;
    }
    else /* !is_rls */
    {
        repository = NULL;
        where = NULL;
        which = W_LOCAL | W_REPOS;
    }

    if (show_tag || show_date || show_dead_revs)
	which |= W_ATTIC;

    if (show_tag != NULL && !tag_validated)
    {
	tag_check_valid (show_tag, argc - 1, argv + 1, local, 0, repository,
			 false);
	tag_validated = true;
    }

    /* Loop on argc so that we are guaranteed that any directory passed to
     * ls_direntproc should be processed if its parent is not yet in DIRS.
     */
    if (argc == 1)
    {
	List *dirs = getlist ();
	err = start_recursion (ls_fileproc, NULL, ls_direntproc,
			       ls_dirleaveproc, dirs, 0, NULL, local, which, 0,
			       CVS_LOCK_READ, where, 1, repository);
	walklist (dirs, ls_print_dir, NULL);
	dellist (&dirs);
    }
    else
    {
	for (i = 1; i < argc; i++)
	{
	    List *dirs = getlist ();
	    err = start_recursion (ls_fileproc, NULL, ls_direntproc,
				   NULL, dirs, 1, argv + i, local, which, 0,
				   CVS_LOCK_READ, where, 1, repository);
	    walklist (dirs, ls_print_dir, NULL);
	    dellist (&dirs);
	}
    }

    if (!(which & W_LOCAL)) free (repository);
    if (where) free (where);

    return err;
}
