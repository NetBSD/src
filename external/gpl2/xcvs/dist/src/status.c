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
 * Status Information
 */

#include "cvs.h"

static Dtype status_dirproc (void *callerdat, const char *dir,
                             const char *repos, const char *update_dir,
                             List *entries);
static int status_fileproc (void *callerdat, struct file_info *finfo);
static int tag_list_proc (Node * p, void *closure);

static int local = 0;
static int long_format = 0;
static RCSNode *xrcsnode;

static const char *const status_usage[] =
{
    "Usage: %s %s [-vlR] [files...]\n",
    "\t-v\tVerbose format; includes tag information for the file\n",
    "\t-l\tProcess this directory only (not recursive).\n",
    "\t-R\tProcess directories recursively.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

int
cvsstatus (int argc, char **argv)
{
    int c;
    int err = 0;

    if (argc == -1)
	usage (status_usage);

    getoptreset ();
    while ((c = getopt (argc, argv, "+vlR")) != -1)
    {
	switch (c)
	{
	    case 'v':
		long_format = 1;
		break;
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case '?':
	    default:
		usage (status_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    wrap_setup ();

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	start_server ();

	ign_setup ();

	if (long_format)
	    send_arg("-v");
	if (local)
	    send_arg("-l");
	send_arg ("--");

	/* For a while, we tried setting SEND_NO_CONTENTS here so this
	   could be a fast operation.  That prevents the
	   server from updating our timestamp if the timestamp is
	   changed but the file is unmodified.  Worse, it is user-visible
	   (shows "locally modified" instead of "up to date" if
	   timestamp is changed but file is not).  And there is no good
	   workaround (you might not want to run "cvs update"; "cvs -n
	   update" doesn't update CVS/Entries; "cvs diff --brief" or
	   something perhaps could be made to work but somehow that
	   seems nonintuitive to me even if so).  Given that timestamps
	   seem to have the potential to get munged for any number of
	   reasons, it seems better to not rely too much on them.  */

	send_files (argc, argv, local, 0, 0);

	send_file_names (argc, argv, SEND_EXPAND_WILD);

	send_to_server ("status\012", 0);
	err = get_responses_and_close ();

	return err;
    }
#endif

    /* start the recursion processor */
    err = start_recursion (status_fileproc, NULL, status_dirproc,
			   NULL, NULL, argc, argv, local, W_LOCAL,
			   0, CVS_LOCK_READ, NULL, 1, NULL);

    return (err);
}

/*
 * display the status of a file
 */
/* ARGSUSED */
static int
status_fileproc (void *callerdat, struct file_info *finfo)
{
    Ctype status;
    char *sstat;
    Vers_TS *vers;
    Node *node;

    status = Classify_File (finfo, NULL, NULL, NULL, 1, 0, &vers, 0);

/* cvsacl patch */
#ifdef SERVER_SUPPORT
    if (use_cvs_acl /* && server_active */)
    {
	if (!access_allowed (finfo->file, finfo->repository, vers->tag, 5,
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

    sstat = "Classify Error";
    switch (status)
    {
	case T_UNKNOWN:
	    sstat = "Unknown";
	    break;
	case T_CHECKOUT:
	    sstat = "Needs Checkout";
	    break;
	case T_PATCH:
	    sstat = "Needs Patch";
	    break;
	case T_CONFLICT:
	    /* FIXME - This message could be clearer.  It comes up
	     * when a file exists or has been added in the local sandbox
	     * and a file of the same name has been committed indepenently to
	     * the repository from a different sandbox, as well as when a
	     * timestamp hasn't changed since a merge resulted in conflicts.
	     * It also comes up whether an update has been attempted or not, so
	     * technically, I think the double-add case is not actually a
	     * conflict yet.
	     */
	    sstat = "Unresolved Conflict";
	    break;
	case T_ADDED:
	    sstat = "Locally Added";
	    break;
	case T_REMOVED:
	    sstat = "Locally Removed";
	    break;
	case T_MODIFIED:
	    if (file_has_markers (finfo))
		sstat = "File had conflicts on merge";
	    else
		/* Note that we do not re Register() the file when we spot
		 * a resolved conflict like update_fileproc() does on the
		 * premise that status should not alter the sandbox.
		 */
		sstat = "Locally Modified";
	    break;
	case T_REMOVE_ENTRY:
	    sstat = "Entry Invalid";
	    break;
	case T_UPTODATE:
	    sstat = "Up-to-date";
	    break;
	case T_NEEDS_MERGE:
	    sstat = "Needs Merge";
	    break;
	case T_TITLE:
	    /* I don't think this case can occur here.  Just print
	       "Classify Error".  */
	    break;
    }

    cvs_output ("\
===================================================================\n", 0);
    if (vers->ts_user == NULL)
    {
	cvs_output ("File: no file ", 0);
	cvs_output (finfo->file, 0);
	cvs_output ("\t\tStatus: ", 0);
	cvs_output (sstat, 0);
	cvs_output ("\n\n", 0);
    }
    else
    {
	char *buf;
	buf = Xasprintf ("File: %-17s\tStatus: %s\n\n", finfo->file, sstat);
	cvs_output (buf, 0);
	free (buf);
    }

    if (vers->vn_user == NULL)
    {
	cvs_output ("   Working revision:\tNo entry for ", 0);
	cvs_output (finfo->file, 0);
	cvs_output ("\n", 0);
    }
    else if (vers->vn_user[0] == '0' && vers->vn_user[1] == '\0')
	cvs_output ("   Working revision:\tNew file!\n", 0);
    else
    {
	cvs_output ("   Working revision:\t", 0);
	cvs_output (vers->vn_user, 0);

	/* Only add the UTC timezone if there is a time to use. */
	if (!server_active && strlen (vers->ts_rcs) > 0)
	{
	    /* Convert from the asctime() format to ISO 8601 */
	    char *buf;

	    cvs_output ("\t", 0);

	    /* Allow conversion from CVS/Entries asctime() to ISO 8601 */
	    buf = Xasprintf ("%s UTC", vers->ts_rcs);
	    cvs_output_tagged ("date", buf);
	    free (buf);
	}
	cvs_output ("\n", 0);
    }

    if (vers->vn_rcs == NULL)
	cvs_output ("   Repository revision:\tNo revision control file\n", 0);
    else
    {
	cvs_output ("   Repository revision:\t", 0);
	cvs_output (vers->vn_rcs, 0);
	cvs_output ("\t", 0);
	cvs_output (vers->srcfile->print_path, 0);
	cvs_output ("\n", 0);

	node = findnode(vers->srcfile->versions,vers->vn_rcs);
	if (node)
	{
	    RCSVers *v;
	    v=(RCSVers*)node->data;
	    node = findnode(v->other_delta,"commitid");
	    cvs_output("   Commit Identifier:\t", 0);
	    if(node && node->data)
	        cvs_output(node->data, 0);
	    else
	        cvs_output("(none)",0);
	    cvs_output("\n",0);
	}
    }

    if (vers->entdata)
    {
	Entnode *edata;

	edata = vers->entdata;
	if (edata->tag)
	{
	    if (vers->vn_rcs == NULL)
	    {
		cvs_output ("   Sticky Tag:\t\t", 0);
		cvs_output (edata->tag, 0);
		cvs_output (" - MISSING from RCS file!\n", 0);
	    }
	    else
	    {
		if (isdigit ((unsigned char) edata->tag[0]))
		{
		    cvs_output ("   Sticky Tag:\t\t", 0);
		    cvs_output (edata->tag, 0);
		    cvs_output ("\n", 0);
		}
		else
		{
		    char *branch = NULL;

		    if (RCS_nodeisbranch (finfo->rcs, edata->tag))
			branch = RCS_whatbranch(finfo->rcs, edata->tag);

		    cvs_output ("   Sticky Tag:\t\t", 0);
		    cvs_output (edata->tag, 0);
		    cvs_output (" (", 0);
		    cvs_output (branch ? "branch" : "revision", 0);
		    cvs_output (": ", 0);
		    cvs_output (branch ? branch : vers->vn_rcs, 0);
		    cvs_output (")\n", 0);

		    if (branch)
			free (branch);
		}
	    }
	}
	else if (!really_quiet)
	    cvs_output ("   Sticky Tag:\t\t(none)\n", 0);

	if (edata->date)
	{
	    cvs_output ("   Sticky Date:\t\t", 0);
	    cvs_output (edata->date, 0);
	    cvs_output ("\n", 0);
	}
	else if (!really_quiet)
	    cvs_output ("   Sticky Date:\t\t(none)\n", 0);

	if (edata->options && edata->options[0])
	{
	    cvs_output ("   Sticky Options:\t", 0);
	    cvs_output (edata->options, 0);
	    cvs_output ("\n", 0);
	}
	else if (!really_quiet)
	    cvs_output ("   Sticky Options:\t(none)\n", 0);
    }

    if (long_format && vers->srcfile)
    {
	List *symbols = RCS_symbols(vers->srcfile);

	cvs_output ("\n   Existing Tags:\n", 0);
	if (symbols)
	{
	    xrcsnode = finfo->rcs;
	    (void) walklist (symbols, tag_list_proc, NULL);
	}
	else
	    cvs_output ("\tNo Tags Exist\n", 0);
    }

    cvs_output ("\n", 0);
    freevers_ts (&vers);
    return (0);
}



/*
 * Print a warm fuzzy message
 */
/* ARGSUSED */
static Dtype
status_dirproc (void *callerdat, const char *dir, const char *repos,
                const char *update_dir, List *entries)
{
    if (!quiet)
	error (0, 0, "Examining %s", update_dir);
    return (R_PROCESS);
}



/*
 * Print out a tag and its type
 */
static int
tag_list_proc (Node *p, void *closure)
{
    char *branch = NULL;
    char *buf;

    if (RCS_nodeisbranch (xrcsnode, p->key))
	branch = RCS_whatbranch(xrcsnode, p->key) ;

    buf = Xasprintf ("\t%-25s\t(%s: %s)\n", p->key,
		     branch ? "branch" : "revision",
		     branch ? branch : (char *)p->data);
    cvs_output (buf, 0);
    free (buf);

    if (branch)
	free (branch);

    return (0);
}
