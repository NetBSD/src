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
 * Create Version
 * 
 * "checkout" creates a "version" of an RCS repository.  This version is owned
 * totally by the user and is actually an independent copy, to be dealt with
 * as seen fit.  Once "checkout" has been called in a given directory, it
 * never needs to be called again.  The user can keep up-to-date by calling
 * "update" when he feels like it; this will supply him with a merge of his
 * own modifications and the changes made in the RCS original.  See "update"
 * for details.
 * 
 * "checkout" can be given a list of directories or files to be updated and in
 * the case of a directory, will recursivley create any sub-directories that
 * exist in the repository.
 * 
 * When the user is satisfied with his own modifications, the present version
 * can be committed by "commit"; this keeps the present version in tact,
 * usually.
 * 
 * The call is cvs checkout [options] <module-name>...
 * 
 * "checkout" creates a directory ./CVS, in which it keeps its administration,
 * in two files, Repository and Entries. The first contains the name of the
 * repository.  The second contains one line for each registered file,
 * consisting of the version number it derives from, its time stamp at
 * derivation time and its name.  Both files are normal files and can be
 * edited by the user, if necessary (when the repository is moved, e.g.)
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: checkout.c,v 1.4 2016/05/17 14:00:09 christos Exp $");

#include "cvs.h"

static char *findslash (char *start, char *p);
static int checkout_proc (int argc, char **argv, char *where,
		          char *mwhere, char *mfile, int shorten,
		          int local_specified, char *omodule,
		          char *msg);

static const char *const checkout_usage[] =
{
    "Usage:\n  %s %s [-ANPRcflnps] [-r rev] [-D date] [-d dir]\n",
    "    [-j rev1] [-j rev2] [-k kopt] modules...\n",
    "\t-A\tReset any sticky tags/date/kopts.\n",
    "\t-N\tDon't shorten module paths if -d specified.\n",
    "\t-P\tPrune empty directories.\n",
    "\t-R\tProcess directories recursively.\n",
    "\t-c\t\"cat\" the module database.\n",
    "\t-f\tForce a head revision match if tag/date not found.\n",
    "\t-l\tLocal directory only, not recursive\n",
    "\t-n\tDo not run module program (if any).\n",
    "\t-p\tCheck out files to standard output (avoids stickiness).\n",
    "\t-s\tLike -c, but include module status.\n",
    "\t-r rev\tCheck out revision or tag. (implies -P) (is sticky)\n",
    "\t-D date\tCheck out revisions as of date. (implies -P) (is sticky)\n",
    "\t-d dir\tCheck out into dir instead of module name.\n",
    "\t-k kopt\tUse RCS kopt -k option on checkout. (is sticky)\n",
    "\t-j rev\tMerge in changes made between current revision and rev.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

static const char *const export_usage[] =
{
    "Usage: %s %s [-NRfln] [-r tag] [-D date] [-d dir] [-k kopt] module...\n",
    "\t-N\tDon't shorten module paths if -d specified.\n",
    "\t-f\tForce a head revision match if tag/date not found.\n",
    "\t-l\tLocal directory only, not recursive\n",
    "\t-R\tProcess directories recursively (default).\n",
    "\t-n\tDo not run module program (if any).\n",
    "\t-r tag\tExport tagged revisions.\n",
    "\t-D date\tExport revisions as of date.\n",
    "\t-d dir\tExport into dir instead of module name.\n",
    "\t-k kopt\tUse RCS kopt -k option on checkout.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

static int checkout_prune_dirs;
static int force_tag_match;
static int pipeout;
static int aflag;
static char *options;
static char *tag;
static bool tag_validated;
static char *date;
static char *join_rev1, *join_date1;
static char *join_rev2, *join_date2;
static bool join_tags_validated;
static char *preload_update_dir;
static char *history_name;
static enum mtype m_type;

int
checkout (int argc, char **argv)
{
    int i;
    int c;
    DBM *db;
    int cat = 0, err = 0, status = 0;
    int run_module_prog = 1;
    int local = 0;
    int shorten = -1;
    char *where = NULL;
    const char *valid_options;
    const char *const *valid_usage;
    char *join_orig1, *join_orig2;

    /* initialize static options */
    force_tag_match = 1;
    if (options)
    {
	free (options);
	options = NULL;
    }
    tag = date = join_rev1 = join_date1 = join_rev2 = join_date2 =
	  join_orig1 = join_orig2 = preload_update_dir = NULL;
    history_name = NULL;
    tag_validated = join_tags_validated = false;


    /*
     * A smaller subset of options are allowed for the export command, which
     * is essentially like checkout, except that it hard-codes certain
     * options to be default (like -kv) and takes care to remove the CVS
     * directory when it has done its duty
     */
    if (strcmp (cvs_cmd_name, "export") == 0)
    {
        m_type = EXPORT;
	valid_options = "+Nnk:d:flRQqr:D:";
	valid_usage = export_usage;
    }
    else
    {
        m_type = CHECKOUT;
	valid_options = "+ANnk:d:flRpQqcsr:D:j:P";
	valid_usage = checkout_usage;
    }

    if (argc == -1)
	usage (valid_usage);

    ign_setup ();
    wrap_setup ();

    getoptreset ();
    while ((c = getopt (argc, argv, valid_options)) != -1)
    {
	switch (c)
	{
	    case 'A':
		aflag = 1;
		break;
	    case 'N':
		shorten = 0;
		break;
	    case 'k':
		if (options)
		    free (options);
		options = RCS_check_kflag (optarg);
		break;
	    case 'n':
		run_module_prog = 0;
		break;
	    case 'Q':
	    case 'q':
		/* The CVS 1.5 client sends these options (in addition to
		   Global_option requests), so we must ignore them.  */
		if (!server_active)
		    error (1, 0,
			   "-q or -Q must be specified before \"%s\"",
			   cvs_cmd_name);
		break;
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case 'P':
		checkout_prune_dirs = 1;
		break;
	    case 'p':
		pipeout = 1;
		run_module_prog = 0;	/* don't run module prog when piping */
		noexec = nolock = 1;	/* so no locks will be created */
		break;
	    case 'c':
		cat = 1;
		break;
	    case 'd':
		where = optarg;
		if (shorten == -1)
		    shorten = 1;
		break;
	    case 's':
		cat = status = 1;
		break;
	    case 'f':
		force_tag_match = 0;
		break;
	    case 'r':
		parse_tagdate (&tag, &date, optarg);
		checkout_prune_dirs = 1;
		break;
	    case 'D':
		if (date) free (date);
		date = Make_Date (optarg);
		checkout_prune_dirs = 1;
		break;
	    case 'j':
		if (join_rev2 || join_date2)
		    error (1, 0, "only two -j options can be specified");
		if (join_rev1 || join_date1)
		{
		    if (join_orig2) free (join_orig2);
		    join_orig2 = xstrdup (optarg);
		    parse_tagdate (&join_rev2, &join_date2, optarg);
		}
		else
		{
		    if (join_orig1) free (join_orig1);
		    join_orig1 = xstrdup (optarg);
		    parse_tagdate (&join_rev1, &join_date1, optarg);
		}
		break;
	    case '?':
	    default:
		usage (valid_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    if (shorten == -1)
	shorten = 0;

    if (cat && argc != 0)
	error (1, 0, "-c and -s must not get any arguments");

    if (!cat && argc == 0)
	error (1, 0, "must specify at least one module or directory");

    if (where && pipeout)
	error (1, 0, "-d and -p are mutually exclusive");

    if (m_type == EXPORT)
    {
	if (!tag && !date)
	    error (1, 0, "must specify a tag or date");

	if (tag && isdigit (tag[0]))
	    error (1, 0, "tag `%s' must be a symbolic tag", tag);
    }

#ifdef SERVER_SUPPORT
    if (server_active && where != NULL)
    {
	server_pathname_check (where);
    }
#endif

    if (!cat && !pipeout && !safe_location (where))
    {
        error (1, 0, "Cannot check out files into the repository itself");
    }

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	int expand_modules;

	start_server ();

	ign_setup ();
	
	expand_modules = (!cat && !pipeout
			  && supported_request ("expand-modules"));
	
	if (expand_modules)
	{
	    /* This is done here because we need to read responses
               from the server before we send the command checkout or
               export files. */

	    client_expand_modules (argc, argv, local);
	}

	if (!run_module_prog)
	    send_arg ("-n");
	if (local)
	    send_arg ("-l");
	if (pipeout)
	    send_arg ("-p");
	if (!force_tag_match)
	    send_arg ("-f");
	if (aflag)
	    send_arg ("-A");
	if (!shorten)
	    send_arg ("-N");
	if (checkout_prune_dirs && m_type == CHECKOUT)
	    send_arg ("-P");
	client_prune_dirs = checkout_prune_dirs;
	if (cat && !status)
	    send_arg ("-c");
	if (where != NULL)
	    option_with_arg ("-d", where);
	if (status)
	    send_arg ("-s");
	if (options != NULL && options[0] != '\0')
	    send_arg (options);
	option_with_arg ("-r", tag);
	if (date)
	    client_senddate (date);
	if (join_orig1)
	    option_with_arg ("-j", join_orig1);
	if (join_orig2)
	    option_with_arg ("-j", join_orig2);
	send_arg ("--");

	if (expand_modules)
	{
	    client_send_expansions (local, where, 1);
	}
	else
	{
	    int i;
	    for (i = 0; i < argc; ++i)
		send_arg (argv[i]);
	    client_nonexpanded_setup ();
	}

	send_to_server (m_type == EXPORT ? "export\012" : "co\012", 0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    if (cat)
    {
	cat_module (status);
	if (options)
	{
	    free (options);
	    options = NULL;
	}
	return 0;
    }
    db = open_module ();


    /* If we've specified something like "cvs co foo/bar baz/quux"
       don't try to shorten names.  There are a few cases in which we
       could shorten (e.g. "cvs co foo/bar foo/baz"), but we don't
       handle those yet.  Better to have an extra directory created
       than the thing checked out under the wrong directory name. */

    if (argc > 1)
	shorten = 0;


    /* If we will be calling history_write, work out the name to pass
       it.  */
    if (!pipeout)
    {
	if (!date)
	    history_name = tag;
	else if (!tag)
	    history_name = date;
	else
	    history_name = Xasprintf ("%s:%s", tag, date);
    }


    for (i = 0; i < argc; i++)
	err += do_module (db, argv[i], m_type, "Updating", checkout_proc,
			  where, shorten, local, run_module_prog, !pipeout,
			  NULL);
    close_module (db);
    if (options)
    {
	free (options);
	options = NULL;
    }
    if (history_name != tag && history_name != date && history_name != NULL)
	free (history_name);
    return err;
}



/* FIXME: This is and emptydir_name are in checkout.c for historical
   reasons, probably want to move them.  */

/* int
 * safe_location ( char *where )
 *
 * Return true if where is a safe destination for a checkout.
 *
 * INPUTS
 *  where	The requested destination directory.
 *
 * GLOBALS
 *  current_parsed_root->directory
 *  current_parsed_root->isremote
 *  		Used to locate our CVSROOT.
 *
 * RETURNS
 *  true	If we are running in client mode or if where is not located
 *  		within the CVSROOT.
 *  false	Otherwise.
 *
 * ERRORS
 *  Exits with a fatal error message when various events occur, such as not
 *  being able to resolve a path or failing ot chdir to a path.
 */
int
safe_location (char *where)
{
    char *current;
    char *hardpath;
    size_t hardpath_len;
    int retval;

    TRACE (TRACE_FUNCTION, "safe_location( where=%s )",
           where ? where : "(null)");

    /* Don't compare remote CVSROOTs to our destination directory. */
    if (current_parsed_root->isremote) return 1;

    /* set current - even if where is set we'll need to cd back... */
    current = xgetcwd ();
    if (current == NULL)
	error (1, errno, "could not get working directory");

    hardpath = xcanonicalize_file_name (current_parsed_root->directory);

    /* if where is set, set current to as much of where as exists,
     * or fail.
     */
    if (where != NULL)
    {
	char *where_this_pass = xstrdup (where);
	while (1)
	{
	    if (CVS_CHDIR (where_this_pass) != -1)
	    {
		/* where */
		free (where_this_pass);
		where_this_pass = xgetcwd ();
		if (where_this_pass == NULL)
		    error (1, errno, "could not get working directory");

		if (CVS_CHDIR (current) == -1)
		    error (1, errno,
			   "could not restore directory to `%s'", current);

		free (current);
		current = where_this_pass;
		break;
	    }
	    else if (errno == ENOENT)
	    {
		/* where_this_pass - last_component (where_this_pass) */
		char *parent;

		/* It's okay to cast out the const below since we know we
		 * allocated where_this_pass and have control of it.
		 */
		if ((parent = (char *)last_component (where_this_pass))
		        != where_this_pass)
		{
		    /* strip the last_component */
		    parent[-1] = '\0';
		    /* continue */
		}
		else
		{
		    /* ERRNO == ENOENT
		     *   && last_component (where_this_pass) == where_this_pass
		     * means we've tried all the parent diretories and not one
		     * exists, so there is no need to test any portion of where
		     * - it is all being created.
		     */
		    free (where_this_pass);
		    break;
		}
	    }
	    else
		/* we don't know how to handle other errors, so fail */
		error (1, errno, "\
could not change directory to requested checkout directory `%s'",
		       where_this_pass);
	} /* while (1) */
    } /* where != NULL */

    hardpath_len = strlen (hardpath);
    if (strlen (current) >= hardpath_len
	&& strncmp (current, hardpath, hardpath_len) == 0)
    {
	if (/* Current is a subdirectory of hardpath.  */
	    current[hardpath_len] == '/'

	    /* Current is hardpath itself.  */
	    || current[hardpath_len] == '\0')
	    retval = 0;
	else
	    /* It isn't a problem.  For example, current is
	       "/foo/cvsroot-bar" and hardpath is "/foo/cvsroot".  */
	    retval = 1;
    }
    else
	retval = 1;
    free (current);
    free (hardpath);
    return retval;
}



struct dir_to_build
{
    /* What to put in CVS/Repository.  */
    char *repository;
    /* The path to the directory.  */
    char *dirpath;

    struct dir_to_build *next;
};



static int build_dirs_and_chdir (struct dir_to_build *list,
					int sticky);

static void
build_one_dir (char *repository, char *dirpath, int sticky)
{
    FILE *fp;

    if (isfile (CVSADM))
    {
	if (m_type == EXPORT)
	    error (1, 0, "cannot export into a working directory");
    }
    else if (m_type == CHECKOUT)
    {
	/* I suspect that this check could be omitted.  */
	if (!isdir (repository))
	    error (1, 0, "there is no repository %s", repository);

	if (Create_Admin (".", dirpath, repository,
			  sticky ? tag : NULL,
			  sticky ? date : NULL,

			  /* FIXME?  This is a guess.  If it is important
			     for nonbranch to be set correctly here I
			     think we need to write it one way now and
			     then rewrite it later via WriteTag, once
			     we've had a chance to call RCS_nodeisbranch
			     on each file.  */
			  0, 1, 1))
	    return;

	if (!noexec)
	{
	    fp = xfopen (CVSADM_ENTSTAT, "w+");
	    if (fclose (fp) == EOF)
		error (1, errno, "cannot close %s", CVSADM_ENTSTAT);
#ifdef SERVER_SUPPORT
	    if (server_active)
		server_set_entstat (dirpath, repository);
#endif
	}
    }
}



/*
 * process_module calls us back here so we do the actual checkout stuff
 */
/* ARGSUSED */
static int
checkout_proc (int argc, char **argv, char *where_orig, char *mwhere,
	       char *mfile, int shorten, int local_specified, char *omodule,
	       char *msg)
{
    char *myargv[2];
    int err = 0;
    int which;
    char *cp;
    char *repository;
    char *oldupdate = NULL;
    char *where;

    TRACE (TRACE_FUNCTION, "checkout_proc (%s, %s, %s, %d, %d, %s, %s)\n",
	   where_orig ? where_orig : "(null)",
	   mwhere ? mwhere : "(null)",
	   mfile ? mfile : "(null)",
	   shorten, local_specified,
	   omodule ? omodule : "(null)",
	   msg ? msg : "(null)"
	  );

    /*
     * OK, so we're doing the checkout! Our args are as follows: 
     *  argc,argv contain either dir or dir followed by a list of files 
     *  where contains where to put it (if supplied by checkout) 
     *  mwhere contains the module name or -d from module file 
     *  mfile says do only that part of the module
     *  shorten = 1 says shorten as much as possible 
     *  omodule is the original arg to do_module()
     */

    /* Set up the repository (maybe) for the bottom directory.
       Allocate more space than we need so we don't need to keep
       reallocating this string. */
    repository = xmalloc (strlen (current_parsed_root->directory)
			  + strlen (argv[0])
			  + (mfile == NULL ? 0 : strlen (mfile))
			  + 10);
    (void) sprintf (repository, "%s/%s",
                    current_parsed_root->directory, argv[0]);
    Sanitize_Repository_Name (repository);


    /* save the original value of preload_update_dir */
    if (preload_update_dir != NULL)
	oldupdate = xstrdup (preload_update_dir);


    /* Allocate space and set up the where variable.  We allocate more
       space than necessary here so that we don't have to keep
       reallocaing it later on. */
    
    where = xmalloc (strlen (argv[0])
		     + (mfile == NULL ? 0 : strlen (mfile))
		     + (mwhere == NULL ? 0 : strlen (mwhere))
		     + (where_orig == NULL ? 0 : strlen (where_orig))
		     + 10);

    /* Yes, this could be written in a less verbose way, but in this
       form it is quite easy to read.
    
       FIXME?  The following code that sets should probably be moved
       to do_module in modules.c, since there is similar code in
       patch.c and rtag.c. */
    
    if (shorten)
    {
	if (where_orig != NULL)
	{
	    /* If the user has specified a directory with `-d' on the
	       command line, use it preferentially, even over the `-d'
	       flag in the modules file. */
    
	    (void) strcpy (where, where_orig);
	}
	else if (mwhere != NULL)
	{
	    /* Second preference is the value of mwhere, which is from
	       the `-d' flag in the modules file. */

	    (void) strcpy (where, mwhere);
	}
	else
	{
	    /* Third preference is the directory specified in argv[0]
	       which is this module'e directory in the repository. */
	    
	    (void) strcpy (where, argv[0]);
	}
    }
    else
    {
	/* Use the same preferences here, bug don't shorten -- that
           is, tack on where_orig if it exists. */

	*where = '\0';

	if (where_orig != NULL)
	{
	    (void) strcat (where, where_orig);
	    (void) strcat (where, "/");
	}

	/* If the -d flag in the modules file specified an absolute
           directory, let the user override it with the command-line
           -d option. */

	if (mwhere && !ISABSOLUTE (mwhere))
	    (void) strcat (where, mwhere);
	else
	    (void) strcat (where, argv[0]);
    }
    strip_trailing_slashes (where); /* necessary? */


    /* At this point, the user may have asked for a single file or
       directory from within a module.  In that case, we should modify
       where, repository, and argv as appropriate. */

    if (mfile != NULL)
    {
	/* The mfile variable can have one or more path elements.  If
	   it has multiple elements, we want to tack those onto both
	   repository and where.  The last element may refer to either
	   a file or directory.  Here's what to do:

	   it refers to a directory
	     -> simply tack it on to where and repository
	   it refers to a file
	     -> munge argv to contain `basename mfile` */

	char *cp;
	char *path;


	/* Paranoia check. */

	if (mfile[strlen (mfile) - 1] == '/')
	{
	    error (0, 0, "checkout_proc: trailing slash on mfile (%s)!",
		   mfile);
	}


	/* Does mfile have multiple path elements? */

	cp = strrchr (mfile, '/');
	if (cp != NULL)
	{
	    *cp = '\0';
	    (void) strcat (repository, "/");
	    (void) strcat (repository, mfile);
	    (void) strcat (where, "/");
	    (void) strcat (where, mfile);
	    mfile = cp + 1;
	}
	

	/* Now mfile is a single path element. */

	path = Xasprintf ("%s/%s", repository, mfile);
	if (isdir (path))
	{
	    /* It's a directory, so tack it on to repository and
               where, as we did above. */

	    (void) strcat (repository, "/");
	    (void) strcat (repository, mfile);
	    (void) strcat (where, "/");
	    (void) strcat (where, mfile);
	}
	else
	{
	    /* It's a file, which means we have to screw around with
               argv. */
	    myargv[0] = argv[0];
	    myargv[1] = mfile;
	    argc = 2;
	    argv = myargv;
	}
	free (path);
    }

    if (preload_update_dir != NULL)
    {
	preload_update_dir =
	    xrealloc (preload_update_dir,
		      strlen (preload_update_dir) + strlen (where) + 5);
	strcat (preload_update_dir, "/");
	strcat (preload_update_dir, where);
    }
    else
	preload_update_dir = xstrdup (where);

    /*
     * At this point, where is the directory we want to build, repository is
     * the repository for the lowest level of the path.
     *
     * We need to tell build_dirs not only the path we want it to
     * build, but also the repositories we want it to populate the
     * path with.  To accomplish this, we walk the path backwards, one
     * pathname component at a time, constucting a linked list of
     * struct dir_to_build.
     */

    /*
     * If we are sending everything to stdout, we can skip a whole bunch of
     * work from here
     */
    if (!pipeout)
    {
	struct dir_to_build *head;
	char *reposcopy;

	if (strncmp (repository, current_parsed_root->directory,
		     strlen (current_parsed_root->directory)) != 0)
	    error (1, 0, "\
internal error: %s doesn't start with %s in checkout_proc",
		   repository, current_parsed_root->directory);

	/* We always create at least one directory, which corresponds to
	   the entire strings for WHERE and REPOSITORY.  */
	head = xmalloc (sizeof (struct dir_to_build));
	/* Special marker to indicate that we don't want build_dirs_and_chdir
	   to create the CVSADM directory for us.  */
	head->repository = NULL;
	head->dirpath = xstrdup (where);
	head->next = NULL;

	/* Make a copy of the repository name to play with. */
	reposcopy = xstrdup (repository);

	/* FIXME: this should be written in terms of last_component
	   instead of hardcoding '/'.  This presumably affects OS/2,
	   NT, &c, if the user specifies '\'.  Likewise for the call
	   to findslash.  */
	cp = where + strlen (where);
	while (cp > where)
	{
	    struct dir_to_build *new;

	    cp = findslash (where, cp - 1);
	    if (cp == NULL)
		break;		/* we're done */

	    new = xmalloc (sizeof (struct dir_to_build));
	    new->dirpath = xmalloc (strlen (where));

	    /* If the user specified an absolute path for where, the
               last path element we create should be the top-level
               directory. */

	    if (cp > where)
	    {
		strncpy (new->dirpath, where, cp - where);
		new->dirpath[cp - where] = '\0';
	    }
	    else
	    {
		/* where should always be at least one character long. */
		assert (where[0] != '\0');
		strcpy (new->dirpath, "/");
	    }
	    new->next = head;
	    head = new;

	    /* Now figure out what repository directory to generate.
               The most complete case would be something like this:

	       The modules file contains
	         foo -d bar/baz quux

	       The command issued was:
	         cvs co -d what/ever -N foo
	       
	       The results in the CVS/Repository files should be:
	         .     -> (don't touch CVS/Repository)
			  (I think this case might be buggy currently)
		 what  -> (don't touch CVS/Repository)
		 ever  -> .          (same as "cd what/ever; cvs co -N foo")
		 bar   -> Emptydir   (generated dir -- not in repos)
		 baz   -> quux       (finally!) */

	    if (strcmp (reposcopy, current_parsed_root->directory) == 0)
	    {
		/* We can't walk up past CVSROOT.  Instead, the
                   repository should be Emptydir. */
		new->repository = emptydir_name ();
	    }
	    else
	    {
		/* It's a directory in the repository! */
		    
		char *rp;
		    
		/* We'll always be below CVSROOT, but check for
		   paranoia's sake. */
		rp = strrchr (reposcopy, '/');
		if (rp == NULL)
		    error (1, 0,
			   "internal error: %s doesn't contain a slash",
			   reposcopy);
			   
		*rp = '\0';
		    
		if (strcmp (reposcopy, current_parsed_root->directory) == 0)
		{
		    /* Special case -- the repository name needs
		       to be "/path/to/repos/." (the trailing dot
		       is important).  We might be able to get rid
		       of this after the we check out the other
		       code that handles repository names. */
		    new-> repository = Xasprintf ("%s/.", reposcopy);
		}
		else
		    new->repository = xstrdup (reposcopy);
	    }
	}

	/* clean up */
	free (reposcopy);

	/* The top-level CVSADM directory should always be
	   current_parsed_root->directory.  Create it, but only if WHERE is
	   relative.  If WHERE is absolute, our current directory
	   may not have a thing to do with where the sources are
	   being checked out.  If it does, build_dirs_and_chdir
	   will take care of creating adm files here. */
	/* FIXME: checking where_is_absolute is a horrid kludge;
	   I suspect we probably can just skip the call to
	   build_one_dir whenever the -d command option was specified
	   to checkout.  */

	if (!ISABSOLUTE (where) && config->top_level_admin
	    && m_type == CHECKOUT)
	{
	    /* It may be argued that we shouldn't set any sticky
	       bits for the top-level repository.  FIXME?  */
	    build_one_dir (current_parsed_root->directory, ".", argc <= 1);

#ifdef SERVER_SUPPORT
	    /* We _always_ want to have a top-level admin
	       directory.  If we're running in client/server mode,
	       send a "Clear-static-directory" command to make
	       sure it is created on the client side.  (See 5.10
	       in cvsclient.dvi to convince yourself that this is
	       OK.)  If this is a duplicate command being sent, it
	       will be ignored on the client side.  */

	    if (server_active)
		server_clear_entstat (".", current_parsed_root->directory);
#endif
	}


	/* Build dirs on the path if necessary and leave us in the
	   bottom directory (where if where was specified) doesn't
	   contain a CVS subdir yet, but all the others contain
	   CVS and Entries.Static files */

	if (build_dirs_and_chdir (head, argc <= 1) != 0)
	{
	    error (0, 0, "ignoring module %s", omodule);
	    err = 1;
	    goto out;
	}

	/* set up the repository (or make sure the old one matches) */
	if (!isfile (CVSADM))
	{
	    FILE *fp;

	    if (!noexec && argc > 1)
	    {
		/* I'm not sure whether this check is redundant.  */
		if (!isdir (repository))
		    error (1, 0, "there is no repository %s", repository);

		Create_Admin (".", preload_update_dir, repository,
			      NULL, NULL, 0, 0, m_type == CHECKOUT);
		fp = xfopen (CVSADM_ENTSTAT, "w+");
		if (fclose (fp) == EOF)
		    error (1, errno, "cannot close %s", CVSADM_ENTSTAT);
#ifdef SERVER_SUPPORT
		if (server_active)
		    server_set_entstat (where, repository);
#endif
	    }
	    else
	    {
		/* I'm not sure whether this check is redundant.  */
		if (!isdir (repository))
		    error (1, 0, "there is no repository %s", repository);

		Create_Admin (".", preload_update_dir, repository, tag, date,

			      /* FIXME?  This is a guess.  If it is important
				 for nonbranch to be set correctly here I
				 think we need to write it one way now and
				 then rewrite it later via WriteTag, once
				 we've had a chance to call RCS_nodeisbranch
				 on each file.  */
			      0, 0, m_type == CHECKOUT);
	    }
	}
	else
	{
	    char *repos;

	    if (m_type == EXPORT)
		error (1, 0, "cannot export into working directory");

	    /* get the contents of the previously existing repository */
	    repos = Name_Repository (NULL, preload_update_dir);
	    if (fncmp (repository, repos) != 0)
	    {
		char *prepos = xstrdup (primary_root_inverse_translate (repos));
		char *prepository =
		    xstrdup (primary_root_inverse_translate (repository));
		error (0, 0, "existing repository %s does not match %s",
		       prepos, prepository);
		error (0, 0, "ignoring module %s", omodule);
		free (repos);
		free (prepos);
		free (prepository);
		err = 1;
		goto out;
	    }
	    free (repos);
	}
    }

    /*
     * If we are going to be updating to stdout, we need to cd to the
     * repository directory so the recursion processor can use the current
     * directory as the place to find repository information
     */
    if (pipeout)
    {
	if (CVS_CHDIR (repository) < 0)
	{
	    error (0, errno, "cannot chdir to %s", repository);
	    err = 1;
	    goto out;
	}
	which = W_REPOS;
	if (tag && !tag_validated)
	{
	    tag_check_valid (tag, argc - 1, argv + 1, 0, aflag,
			     repository, false);
	    tag_validated = true;
	}
    }
    else
    {
	which = W_LOCAL | W_REPOS;
	if (tag && !tag_validated)
	{
	    tag_check_valid (tag, argc - 1, argv + 1, 0, aflag,
			     repository, false);
	    tag_validated = true;
	}
    }

    if (tag || date || join_rev1 || join_date2)
	which |= W_ATTIC;

    if (!join_tags_validated)
    {
        if (join_rev1)
	    tag_check_valid (join_rev1, argc - 1, argv + 1, 0, aflag,
			     repository, false);
	if (join_rev2)
	    tag_check_valid (join_rev2, argc - 1, argv + 1, 0, aflag,
			     repository, false);
	join_tags_validated = true;
    }

    /*
     * if we are going to be recursive (building dirs), go ahead and call the
     * update recursion processor.  We will be recursive unless either local
     * only was specified, or we were passed arguments
     */
    if (!(local_specified || argc > 1))
    {
	if (!pipeout)
	    history_write (m_type == CHECKOUT ? 'O' : 'E', preload_update_dir,
			   history_name, where, repository);
	err += do_update (0, NULL, options, tag, date,
			  force_tag_match, false /* !local */ ,
			  true /* update -d */ , aflag, checkout_prune_dirs,
			  pipeout, which, join_rev1, join_date1,
			  join_rev2, join_date2,
			  preload_update_dir, m_type == CHECKOUT,
			  repository);
	goto out;
    }

    if (!pipeout)
    {
	int i;
	List *entries;

	/* we are only doing files, so register them */
	entries = Entries_Open (0, NULL);
	for (i = 1; i < argc; i++)
	{
	    char *line;
	    Vers_TS *vers;
	    struct file_info finfo;

	    memset (&finfo, 0, sizeof finfo);
	    finfo.file = argv[i];
	    /* Shouldn't be used, so set to arbitrary value.  */
	    finfo.update_dir = NULL;
	    finfo.fullname = argv[i];
	    finfo.repository = repository;
	    finfo.entries = entries;
	    /* The rcs slot is needed to get the options from the RCS
               file */
	    finfo.rcs = RCS_parse (finfo.file, repository);

	    vers = Version_TS (&finfo, options, tag, date,
			       force_tag_match, 0);
	    if (vers->ts_user == NULL)
	    {
		line = Xasprintf ("Initial %s", finfo.file);
		Register (entries, finfo.file,
			  vers->vn_rcs ? vers->vn_rcs : "0",
			  line, vers->options, vers->tag,
			  vers->date, NULL);
		free (line);
	    }
	    freevers_ts (&vers);
	    freercsnode (&finfo.rcs);
	}

	Entries_Close (entries);
    }

    /* Don't log "export", just regular "checkouts" */
    if (m_type == CHECKOUT && !pipeout)
	history_write ('O', preload_update_dir, history_name, where,
		       repository);

    /* go ahead and call update now that everything is set */
    err += do_update (argc - 1, argv + 1, options, tag, date,
		      force_tag_match, local_specified, true /* update -d */,
		      aflag, checkout_prune_dirs, pipeout, which, join_rev1,
		      join_date1, join_rev2, join_date2, preload_update_dir,
		      m_type == CHECKOUT, repository);
out:
    free (preload_update_dir);
    preload_update_dir = oldupdate;
    free (where);
    free (repository);
    return err;
}



static char *
findslash (char *start, char *p)
{
    for (;;)
    {
	if (*p == '/') return p;
	if (p == start) break;
	--p;
    }
    return NULL;
}



/* Return a newly malloc'd string containing a pathname for CVSNULLREPOS,
   and make sure that it exists.  If there is an error creating the
   directory, give a fatal error.  Otherwise, the directory is guaranteed
   to exist when we return.  */
char *
emptydir_name (void)
{
    char *repository;

    repository = Xasprintf ("%s/%s/%s", current_parsed_root->directory,
			    CVSROOTADM, CVSNULLREPOS);
    if (!isfile (repository))
    {
	mode_t omask;
	omask = umask (cvsumask);
	if (CVS_MKDIR (repository, 0777) < 0)
	    error (1, errno, "cannot create %s", repository);
	(void) umask (omask);
    }
    return repository;
}



/* Build all the dirs along the path to DIRS with CVS subdirs with appropriate
 * repositories.  If DIRS->repository is NULL or the directory already exists,
 * do not create a CVSADM directory for that subdirectory; just CVS_CHDIR into
 * it.  Frees all storage used by DIRS.
 *
 * ASSUMPTIONS
 *   1. Parent directories will be listed in DIRS before their children.
 *   2. At most a single directory will need to be changed at one time.  In
 *      other words, if we are in /a/b/c, and our final destination is
 *      /a/b/c/d/e/f, then we will build d, then d/e, then d/e/f.
 *
 * INPUTS
 *   dirs	Simple list composed of dir_to_build structures, listing
 *		information about directories to build.
 *   sticky	Passed to build_one_dir to tell it whether there are any sticky
 *		tags or dates to be concerned with.
 *
 * RETURNS
 *   1 on error, 0 otherwise.
 *
 * ERRORS
 *  The only nonfatal error this function may return is if the CHDIR fails.
 */
static int
build_dirs_and_chdir (struct dir_to_build *dirs, int sticky)
{
    int retval = 0;
    struct dir_to_build *nextdir;

    while (dirs != NULL)
    {
	const char *dir = last_component (dirs->dirpath);
	int made_dir = 0;

	made_dir = !mkdir_if_needed (dir);
	if (made_dir) Subdir_Register (NULL, NULL, dir);

	if (CVS_CHDIR (dir) < 0)
	{
	    error (0, errno, "cannot chdir to %s", dir);
	    retval = 1;
	    goto out;
	}
	if (dirs->repository != NULL)
	{
	    if (made_dir)
		build_one_dir (dirs->repository, dirs->dirpath, sticky);
	    free (dirs->repository);
	}
	nextdir = dirs->next;
	free (dirs->dirpath);
	free (dirs);
	dirs = nextdir;
    }

 out:
    while (dirs != NULL)
    {
	if (dirs->repository != NULL)
	    free (dirs->repository);
	nextdir = dirs->next;
	free (dirs->dirpath);
	free (dirs);
	dirs = nextdir;
    }
    return retval;
}
