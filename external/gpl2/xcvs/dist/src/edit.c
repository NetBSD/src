/* Implementation for "cvs edit", "cvs watch on", and related commands

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"
#include "getline.h"
#include "yesno.h"
#include "watch.h"
#include "edit.h"
#include "fileattr.h"

static int watch_onoff (int, char **);

static bool check_edited = false;
static int setting_default;
static int turning_on;

static bool setting_tedit;
static bool setting_tunedit;
static bool setting_tcommit;



static int
onoff_fileproc (void *callerdat, struct file_info *finfo)
{
    fileattr_get0 (finfo->file, "_watched");
    fileattr_set (finfo->file, "_watched", turning_on ? "" : NULL);
    return 0;
}



static int
onoff_filesdoneproc (void *callerdat, int err, const char *repository,
                     const char *update_dir, List *entries)
{
    if (setting_default)
    {
	fileattr_get0 (NULL, "_watched");
	fileattr_set (NULL, "_watched", turning_on ? "" : NULL);
    }
    return err;
}



static int
watch_onoff (int argc, char **argv)
{
    int c;
    int local = 0;
    int err;

    getoptreset ();
    while ((c = getopt (argc, argv, "+lR")) != -1)
    {
	switch (c)
	{
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case '?':
	    default:
		usage (watch_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	start_server ();

	ign_setup ();

	if (local)
	    send_arg ("-l");
	send_arg ("--");
	send_files (argc, argv, local, 0, SEND_NO_CONTENTS);
	send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_to_server (turning_on ? "watch-on\012" : "watch-off\012", 0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    setting_default = (argc <= 0);

    lock_tree_promotably (argc, argv, local, W_LOCAL, 0);

    err = start_recursion (onoff_fileproc, onoff_filesdoneproc, NULL, NULL,
			   NULL, argc, argv, local, W_LOCAL, 0, CVS_LOCK_WRITE,
			   NULL, 0, NULL);

    Lock_Cleanup ();
    return err;
}

int
watch_on (int argc, char **argv)
{
    turning_on = 1;
    return watch_onoff (argc, argv);
}

int
watch_off (int argc, char **argv)
{
    turning_on = 0;
    return watch_onoff (argc, argv);
}



static int
dummy_fileproc (void *callerdat, struct file_info *finfo)
{
    /* This is a pretty hideous hack, but the gist of it is that recurse.c
       won't call notify_check unless there is a fileproc, so we can't just
       pass NULL for fileproc.  */
    return 0;
}



/* Check for and process notifications.  Local only.  I think that doing
   this as a fileproc is the only way to catch all the
   cases (e.g. foo/bar.c), even though that means checking over and over
   for the same CVSADM_NOTIFY file which we removed the first time we
   processed the directory.  */
static int
ncheck_fileproc (void *callerdat, struct file_info *finfo)
{
    int notif_type;
    char *filename;
    char *val;
    char *cp;
    char *watches;

    FILE *fp;
    char *line = NULL;
    size_t line_len = 0;

    /* We send notifications even if noexec.  I'm not sure which behavior
       is most sensible.  */

    fp = CVS_FOPEN (CVSADM_NOTIFY, "r");
    if (fp == NULL)
    {
	if (!existence_error (errno))
	    error (0, errno, "cannot open %s", CVSADM_NOTIFY);
	return 0;
    }

    while (getline (&line, &line_len, fp) > 0)
    {
	notif_type = line[0];
	if (notif_type == '\0')
	    continue;
	filename = line + 1;
	cp = strchr (filename, '\t');
	if (cp == NULL)
	    continue;
	*cp++ = '\0';
	val = cp;
	cp = strchr (val, '\t');
	if (cp == NULL)
	    continue;
	*cp++ = '+';
	cp = strchr (cp, '\t');
	if (cp == NULL)
	    continue;
	*cp++ = '+';
	cp = strchr (cp, '\t');
	if (cp == NULL)
	    continue;
	*cp++ = '\0';
	watches = cp;
	cp = strchr (cp, '\n');
	if (cp == NULL)
	    continue;
	*cp = '\0';

	notify_do (notif_type, filename, finfo->update_dir, getcaller (), val,
		   watches, finfo->repository);
    }
    free (line);

    if (ferror (fp))
	error (0, errno, "cannot read %s", CVSADM_NOTIFY);
    if (fclose (fp) < 0)
	error (0, errno, "cannot close %s", CVSADM_NOTIFY);

    if ( CVS_UNLINK (CVSADM_NOTIFY) < 0)
	error (0, errno, "cannot remove %s", CVSADM_NOTIFY);

    return 0;
}



/* Look through the CVSADM_NOTIFY file and process each item there
   accordingly.  */
static int
send_notifications (int argc, char **argv, int local)
{
    int err = 0;

#ifdef CLIENT_SUPPORT
    /* OK, we've done everything which needs to happen on the client side.
       Now we can try to contact the server; if we fail, then the
       notifications stay in CVSADM_NOTIFY to be sent next time.  */
    if (current_parsed_root->isremote)
    {
	if (strcmp (cvs_cmd_name, "release") != 0)
	{
	    start_server ();
	    ign_setup ();
	}

	err += start_recursion (dummy_fileproc, NULL, NULL, NULL, NULL, argc,
				argv, local, W_LOCAL, 0, 0, NULL, 0, NULL);

	send_to_server ("noop\012", 0);
	if (strcmp (cvs_cmd_name, "release") == 0)
	    err += get_server_responses ();
	else
	    err += get_responses_and_close ();
    }
    else
#endif
    {
	/* Local.  */

	err += start_recursion (ncheck_fileproc, NULL, NULL, NULL, NULL, argc,
				argv, local, W_LOCAL, 0, CVS_LOCK_WRITE, NULL,
				0, NULL);
	Lock_Cleanup ();
    }
    return err;
}



void editors_output (const char *fullname, const char *p)
{
    cvs_output (fullname, 0);

    while (1)
    {
        cvs_output ("\t", 1);
        while (*p != '>' && *p != '\0')
            cvs_output (p++, 1);
        if (*p == '\0')
        {
            /* Only happens if attribute is misformed.  */
            cvs_output ("\n", 1);
            break;
        }
        ++p;
        cvs_output ("\t", 1);
        while (1)
        {
            while (*p != '+' && *p != ',' && *p != '\0')
                cvs_output (p++, 1);
            if (*p == '\0')
            {
                cvs_output ("\n", 1);
                return;
            }
            if (*p == ',')
            {
                ++p;
                break;
            }
            ++p;
            cvs_output ("\t", 1);
        }
        cvs_output ("\n", 1);
    }
}



static int find_editors_and_output (struct file_info *finfo)
{
    char *them;

    them = fileattr_get0 (finfo->file, "_editors");
    if (them == NULL)
        return 0;

    editors_output (finfo->fullname, them);

    return 0;
}



/* Handle the client-side details of editing a file.
 *
 * These args could be const but this needs to fit the call_in_directory API.
 */
void
edit_file (void *data, List *ent_list, const char *short_pathname,
	   const char *filename)
{
    Node *node;
    struct file_info finfo;
    char *basefilename;

    xchmod (filename, 1);

    mkdir_if_needed (CVSADM_BASE);
    basefilename = Xasprintf ("%s/%s", CVSADM_BASE, filename);
    copy_file (filename, basefilename);
    free (basefilename);

    node = findnode_fn (ent_list, filename);
    if (node != NULL)
    {
	finfo.file = filename;
	finfo.fullname = short_pathname;
	finfo.update_dir = dir_name (short_pathname);
	base_register (&finfo, ((Entnode *) node->data)->version);
	free ((char *)finfo.update_dir);
    }
}



static int
edit_fileproc (void *callerdat, struct file_info *finfo)
{
    FILE *fp;
    time_t now;
    char *ascnow;
    Vers_TS *vers;

#if defined (CLIENT_SUPPORT)
    assert (!(current_parsed_root->isremote && check_edited));
#else /* !CLIENT_SUPPORT */
    assert (!check_edited);
#endif /* CLIENT_SUPPORT */

    if (noexec)
	return 0;

    vers = Version_TS (finfo, NULL, NULL, NULL, 1, 0);

    if (!vers->vn_user)
    {
	error (0, 0, "no such file %s; ignored", finfo->fullname);
	return 1;
    }

#ifdef CLIENT_SUPPORT
    if (!current_parsed_root->isremote)
#endif /* CLIENT_SUPPORT */
    {
        char *editors = fileattr_get0 (finfo->file, "_editors");
        if (editors)
        {
	    if (check_edited)
	    {
		/* In the !CHECK_EDIT case, this message is printed by
		 * server_notify.
		 */
		if (!quiet)
		    editors_output (finfo->fullname, editors);
		 /* Now warn the user if we skip the file, then return.  */
		if (!really_quiet)
		    error (0, 0, "Skipping file `%s' due to existing editors.",
			   finfo->fullname);
		return 1;
	    }
            free (editors);
        }
    }

    fp = xfopen (CVSADM_NOTIFY, "a");

    (void) time (&now);
    ascnow = asctime (gmtime (&now));
    ascnow[24] = '\0';
    /* Fix non-standard format.  */
    if (ascnow[8] == '0') ascnow[8] = ' ';
    fprintf (fp, "E%s\t%s -0000\t%s\t%s\t", finfo->file,
	     ascnow, hostname, CurDir);
    if (setting_tedit)
	fprintf (fp, "E");
    if (setting_tunedit)
	fprintf (fp, "U");
    if (setting_tcommit)
	fprintf (fp, "C");
    fprintf (fp, "\n");

    if (fclose (fp) < 0)
    {
	if (finfo->update_dir[0] == '\0')
	    error (0, errno, "cannot close %s", CVSADM_NOTIFY);
	else
	    error (0, errno, "cannot close %s/%s", finfo->update_dir,
		   CVSADM_NOTIFY);
    }

    /* Now stash the file away in CVSADM so that unedit can revert even if
       it can't communicate with the server.  We stash away a writable
       copy so that if the user removes the working file, then restores it
       with "cvs update" (which clears _editors but does not update
       CVSADM_BASE), then a future "cvs edit" can still win.  */
    /* Could save a system call by only calling mkdir_if_needed if
       trying to create the output file fails.  But copy_file isn't
       set up to facilitate that.  */
#ifdef SERVER_SUPPORT
    if (server_active)
	server_edit_file (finfo);
    else
#endif /* SERVER_SUPPORT */
	edit_file (NULL, finfo->entries, finfo->fullname, finfo->file);

    return 0;
}

static const char *const edit_usage[] =
{
    "Usage: %s %s [-lRcf] [-a <action>]... [<file>]...\n",
    "-l\tLocal directory only, not recursive.\n",
    "-R\tProcess directories recursively (default).\n",
    "-a\tSpecify action to register for temporary watch, one of:\n",
    "  \t`edit', `unedit', `commit', `all', `none' (defaults to `all').\n",
    "-c\tCheck for <file>s edited by others and abort if found.\n",
    "-f\tAllow edit if <file>s are edited by others (default).\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};

int
edit (int argc, char **argv)
{
    int local = 0;
    int c;
    int err = 0;
    bool a_omitted, a_all, a_none;

    if (argc == -1)
	usage (edit_usage);

    a_omitted = true;
    a_all = false;
    a_none = false;
    setting_tedit = false;
    setting_tunedit = false;
    setting_tcommit = false;
    getoptreset ();
    while ((c = getopt (argc, argv, "+cflRa:")) != -1)
    {
	switch (c)
	{
            case 'c':
                check_edited = true;
                break;
            case 'f':
                check_edited = false;
                break;
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case 'a':
		a_omitted = false;
		if (strcmp (optarg, "edit") == 0)
		    setting_tedit = true;
		else if (strcmp (optarg, "unedit") == 0)
		    setting_tunedit = true;
		else if (strcmp (optarg, "commit") == 0)
		    setting_tcommit = true;
		else if (strcmp (optarg, "all") == 0)
		{
		    a_all = true;
		    a_none = false;
		    setting_tedit = true;
		    setting_tunedit = true;
		    setting_tcommit = true;
		}
		else if (strcmp (optarg, "none") == 0)
		{
		    a_none = true;
		    a_all = false;
		    setting_tedit = false;
		    setting_tunedit = false;
		    setting_tcommit = false;
		}
		else
		    usage (edit_usage);
		break;
	    case '?':
	    default:
		usage (edit_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    if (strpbrk (hostname, "+,>;=\t\n") != NULL)
	error (1, 0,
	       "host name (%s) contains an invalid character (+,>;=\\t\\n)",
	       hostname);
    if (strpbrk (CurDir, "+,>;=\t\n") != NULL)
	error (1, 0,
"current directory (%s) contains an invalid character (+,>;=\\t\\n)",
	       CurDir);

#ifdef CLIENT_SUPPORT
    if (check_edited && current_parsed_root->isremote)
    {
	/* When CHECK_EDITED, we might as well contact the server and let it do
	 * the work since we don't want an edit unless we know it is safe.
	 *
	 * When !CHECK_EDITED, we set up notifications and then attempt to
	 * contact the server in order to allow disconnected edits.
	 */
	start_server();

	if (!supported_request ("edit"))
	    error (1, 0, "Server does not support enforced advisory locks.");

	ign_setup();

	send_to_server ("Hostname ", 0);
	send_to_server (hostname, 0);
	send_to_server ("\012", 1);
	send_to_server ("LocalDir ", 0);
	send_to_server (CurDir, 0);
	send_to_server ("\012", 1);

	if (local)
	    send_arg ("-l");
	send_arg ("-c");
	if (!a_omitted)
	{
	    if (a_all)
		option_with_arg ("-a", "all");
	    else if (a_none)
		option_with_arg ("-a", "none");
	    else
	    {
		if (setting_tedit)
		    option_with_arg ("-a", "edit");
		if (setting_tunedit)
		    option_with_arg ("-a", "unedit");
		if (setting_tcommit)
		    option_with_arg ("-a", "commit");
	    }
	}
	send_arg ("--");
	send_files (argc, argv, local, 0, SEND_NO_CONTENTS);
	send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_to_server ("edit\012", 0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    /* Now, either SERVER_ACTIVE, local mode, or !CHECK_EDITED.  */

    if (a_omitted)
    {
	setting_tedit = true;
	setting_tunedit = true;
	setting_tcommit = true;
    }

    TRACE (TRACE_DATA, "edit(): EUC: %d%d%d edit-check: %d",
	   setting_tedit, setting_tunedit, setting_tcommit, check_edited);

    err = start_recursion (edit_fileproc, NULL, NULL, NULL, NULL, argc, argv,
			   local, W_LOCAL, 0, 0, NULL, 0, NULL);

    err += send_notifications (argc, argv, local);

    return err;
}

static int unedit_fileproc (void *callerdat, struct file_info *finfo);

static int
unedit_fileproc (void *callerdat, struct file_info *finfo)
{
    FILE *fp;
    time_t now;
    char *ascnow;
    char *basefilename = NULL;

    if (noexec)
	return 0;

    basefilename = Xasprintf ("%s/%s", CVSADM_BASE, finfo->file);
    if (!isfile (basefilename))
    {
	/* This file apparently was never cvs edit'd (e.g. we are uneditting
	   a directory where only some of the files were cvs edit'd.  */
	free (basefilename);
	return 0;
    }

    if (xcmp (finfo->file, basefilename) != 0)
    {
	printf ("%s has been modified; revert changes? ", finfo->fullname);
	fflush (stderr);
	fflush (stdout);
	if (!yesno ())
	{
	    /* "no".  */
	    free (basefilename);
	    return 0;
	}
    }
    rename_file (basefilename, finfo->file);
    free (basefilename);

    fp = xfopen (CVSADM_NOTIFY, "a");

    (void) time (&now);
    ascnow = asctime (gmtime (&now));
    ascnow[24] = '\0';
    /* Fix non-standard format.  */
    if (ascnow[8] == '0') ascnow[8] = ' ';
    fprintf (fp, "U%s\t%s -0000\t%s\t%s\t\n", finfo->file,
	     ascnow, hostname, CurDir);

    if (fclose (fp) < 0)
    {
	if (finfo->update_dir[0] == '\0')
	    error (0, errno, "cannot close %s", CVSADM_NOTIFY);
	else
	    error (0, errno, "cannot close %s/%s", finfo->update_dir,
		   CVSADM_NOTIFY);
    }

    /* Now update the revision number in CVS/Entries from CVS/Baserev.
       The basic idea here is that we are reverting to the revision
       that the user edited.  If we wanted "cvs update" to update
       CVS/Base as we go along (so that an unedit could revert to the
       current repository revision), we would need:

       update (or all send_files?) (client) needs to send revision in
       new Entry-base request.  update (server/local) needs to check
       revision against repository and send new Update-base response
       (like Update-existing in that the file already exists.  While
       we are at it, might try to clean up the syntax by having the
       mode only in a "Mode" response, not in the Update-base itself).  */
    {
	char *baserev;
	Node *node;
	Entnode *entdata;

	baserev = base_get (finfo);
	node = findnode_fn (finfo->entries, finfo->file);
	/* The case where node is NULL probably should be an error or
	   something, but I don't want to think about it too hard right
	   now.  */
	if (node != NULL)
	{
	    entdata = node->data;
	    if (baserev == NULL)
	    {
		/* This can only happen if the CVS/Baserev file got
		   corrupted.  We suspect it might be possible if the
		   user interrupts CVS, although I haven't verified
		   that.  */
		error (0, 0, "%s not mentioned in %s", finfo->fullname,
		       CVSADM_BASEREV);

		/* Since we don't know what revision the file derives from,
		   keeping it around would be asking for trouble.  */
		if (unlink_file (finfo->file) < 0)
		    error (0, errno, "cannot remove %s", finfo->fullname);

		/* This is cheesy, in a sense; why shouldn't we do the
		   update for the user?  However, doing that would require
		   contacting the server, so maybe this is OK.  */
		error (0, 0, "run update to complete the unedit");
		return 0;
	    }
	    Register (finfo->entries, finfo->file, baserev, entdata->timestamp,
		      entdata->options, entdata->tag, entdata->date,
		      entdata->conflict);
	}
	free (baserev);
	base_deregister (finfo);
    }

    xchmod (finfo->file, 0);
    return 0;
}

static const char *const unedit_usage[] =
{
    "Usage: %s %s [-lR] [<file>]...\n",
    "-l\tLocal directory only, not recursive.\n",
    "-R\tProcess directories recursively (default).\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};

int
unedit (int argc, char **argv)
{
    int local = 0;
    int c;
    int err;

    if (argc == -1)
	usage (unedit_usage);

    getoptreset ();
    while ((c = getopt (argc, argv, "+lR")) != -1)
    {
	switch (c)
	{
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case '?':
	    default:
		usage (unedit_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    /* No need to readlock since we aren't doing anything to the
       repository.  */
    err = start_recursion (unedit_fileproc, NULL, NULL, NULL, NULL, argc, argv,
			   local, W_LOCAL, 0, 0, NULL, 0, NULL);

    err += send_notifications (argc, argv, local);

    return err;
}



void
mark_up_to_date (const char *file)
{
    char *base;

    /* The file is up to date, so we better get rid of an out of
       date file in CVSADM_BASE.  */
    base = Xasprintf ("%s/%s", CVSADM_BASE, file);
    if (unlink_file (base) < 0 && ! existence_error (errno))
	error (0, errno, "cannot remove %s", file);
    free (base);
}



void
editor_set (const char *filename, const char *editor, const char *val)
{
    char *edlist;
    char *newlist;

    edlist = fileattr_get0 (filename, "_editors");
    newlist = fileattr_modify (edlist, editor, val, '>', ',');
    /* If the attributes is unchanged, don't rewrite the attribute file.  */
    if (!((edlist == NULL && newlist == NULL)
	  || (edlist != NULL
	      && newlist != NULL
	      && strcmp (edlist, newlist) == 0)))
	fileattr_set (filename, "_editors", newlist);
    if (edlist != NULL)
	free (edlist);
    if (newlist != NULL)
	free (newlist);
}

struct notify_proc_args {
    /* What kind of notification, "edit", "tedit", etc.  */
    const char *type;
    /* User who is running the command which causes notification.  */
    const char *who;
    /* User to be notified.  */
    const char *notifyee;
    /* File.  */
    const char *file;
};



static int
notify_proc (const char *repository, const char *filter, void *closure)
{
    char *cmdline;
    FILE *pipefp;
    const char *srepos = Short_Repository (repository);
    struct notify_proc_args *args = closure;

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
			      "s", "s", args->notifyee,
			      (char *) NULL);
    if (!cmdline || !strlen (cmdline))
    {
	if (cmdline) free (cmdline);
	error (0, 0, "pretag proc resolved to the empty string!");
	return 1;
    }

    pipefp = run_popen (cmdline, "w");
    if (pipefp == NULL)
    {
	error (0, errno, "cannot write entry to notify filter: %s", cmdline);
	free (cmdline);
	return 1;
    }

    fprintf (pipefp, "%s %s\n---\n", srepos, args->file);
    fprintf (pipefp, "Triggered %s watch on %s\n", args->type, repository);
    fprintf (pipefp, "By %s\n", args->who);

    /* Lots more potentially useful information we could add here; see
       logfile_write for inspiration.  */

    free (cmdline);
    return pclose (pipefp);
}



/* FIXME: this function should have a way to report whether there was
   an error so that server.c can know whether to report Notified back
   to the client.  */
void
notify_do (int type, const char *filename, const char *update_dir,
	   const char *who, const char *val, const char *watches,
	   const char *repository)
{
    static struct addremove_args blank;
    struct addremove_args args;
    char *watchers;
    char *p;
    char *endp;
    char *nextp;

    /* Print out information on current editors if we were called during an
     * edit.
     */
    if (type == 'E' && !check_edited && !quiet)
    {
	char *editors = fileattr_get0 (filename, "_editors");
	if (editors)
	{
	    /* In the CHECK_EDIT case, this message is printed by
	     * edit_check.  It needs to be done here too since files
	     * which are found to be edited when CHECK_EDIT are not
	     * added to the notify list.
	     */
	    const char *tmp;
	    if (update_dir && *update_dir)
		tmp  = Xasprintf ("%s/%s", update_dir, filename);
	    else
		tmp = filename;

	    editors_output (tmp, editors);

	    if (update_dir && *update_dir) free ((char *)tmp);
	    free (editors);
	}
    }

    /* Initialize fields to 0, NULL, or 0.0.  */
    args = blank;
    switch (type)
    {
	case 'E':
	    if (strpbrk (val, ",>;=\n") != NULL)
	    {
		error (0, 0, "invalid character in editor value");
		return;
	    }
	    editor_set (filename, who, val);
	    break;
	case 'U':
	case 'C':
	    editor_set (filename, who, NULL);
	    break;
	default:
	    return;
    }

    watchers = fileattr_get0 (filename, "_watchers");
    p = watchers;
    while (p != NULL)
    {
	char *q;
	char *endq;
	char *nextq;
	char *notif;

	endp = strchr (p, '>');
	if (endp == NULL)
	    break;
	nextp = strchr (p, ',');

	if ((size_t)(endp - p) == strlen (who) && strncmp (who, p, endp - p) == 0)
	{
	    /* Don't notify user of their own changes.  Would perhaps
	       be better to check whether it is the same working
	       directory, not the same user, but that is hairy.  */
	    p = nextp == NULL ? nextp : nextp + 1;
	    continue;
	}

	/* Now we point q at a string which looks like
	   "edit+unedit+commit,"... and walk down it.  */
	q = endp + 1;
	notif = NULL;
	while (q != NULL)
	{
	    endq = strchr (q, '+');
	    if (endq == NULL || (nextp != NULL && endq > nextp))
	    {
		if (nextp == NULL)
		    endq = q + strlen (q);
		else
		    endq = nextp;
		nextq = NULL;
	    }
	    else
		nextq = endq + 1;

	    /* If there is a temporary and a regular watch, send a single
	       notification, for the regular watch.  */
	    if (type == 'E' && endq - q == 4 && strncmp ("edit", q, 4) == 0)
	    {
		notif = "edit";
	    }
	    else if (type == 'U'
		     && endq - q == 6 && strncmp ("unedit", q, 6) == 0)
	    {
		notif = "unedit";
	    }
	    else if (type == 'C'
		     && endq - q == 6 && strncmp ("commit", q, 6) == 0)
	    {
		notif = "commit";
	    }
	    else if (type == 'E'
		     && endq - q == 5 && strncmp ("tedit", q, 5) == 0)
	    {
		if (notif == NULL)
		    notif = "temporary edit";
	    }
	    else if (type == 'U'
		     && endq - q == 7 && strncmp ("tunedit", q, 7) == 0)
	    {
		if (notif == NULL)
		    notif = "temporary unedit";
	    }
	    else if (type == 'C'
		     && endq - q == 7 && strncmp ("tcommit", q, 7) == 0)
	    {
		if (notif == NULL)
		    notif = "temporary commit";
	    }
	    q = nextq;
	}
	if (nextp != NULL)
	    ++nextp;

	if (notif != NULL)
	{
	    struct notify_proc_args args;
	    size_t len = endp - p;
	    FILE *fp;
	    char *usersname;
	    char *line = NULL;
	    size_t line_len = 0;

	    args.notifyee = NULL;
	    usersname = Xasprintf ("%s/%s/%s", current_parsed_root->directory,
				   CVSROOTADM, CVSROOTADM_USERS);
	    fp = CVS_FOPEN (usersname, "r");
	    if (fp == NULL && !existence_error (errno))
		error (0, errno, "cannot read %s", usersname);
	    if (fp != NULL)
	    {
		while (getline (&line, &line_len, fp) >= 0)
		{
		    if (strncmp (line, p, len) == 0
			&& line[len] == ':')
		    {
			char *cp;
			args.notifyee = xstrdup (line + len + 1);

                        /* There may or may not be more
                           colon-separated fields added to this in the
                           future; in any case, we ignore them right
                           now, and if there are none we make sure to
                           chop off the final newline, if any. */
			cp = strpbrk (args.notifyee, ":\n");

			if (cp != NULL)
			    *cp = '\0';
			break;
		    }
		}
		if (ferror (fp))
		    error (0, errno, "cannot read %s", usersname);
		if (fclose (fp) < 0)
		    error (0, errno, "cannot close %s", usersname);
	    }
	    free (usersname);
	    if (line != NULL)
		free (line);

	    if (args.notifyee == NULL)
	    {
		char *tmp;
		tmp = xmalloc (endp - p + 1);
		strncpy (tmp, p, endp - p);
		tmp[endp - p] = '\0';
		args.notifyee = tmp;
	    }

	    args.type = notif;
	    args.who = who;
	    args.file = filename;

	    (void) Parse_Info (CVSROOTADM_NOTIFY, repository, notify_proc,
			       PIOPT_ALL, &args);
            /* It's okay to cast out the const for the free() below since we
             * just allocated this a few lines above.  The const was for
             * everybody else.
             */
	    free ((char *)args.notifyee);
	}

	p = nextp;
    }
    if (watchers != NULL)
	free (watchers);

    switch (type)
    {
	case 'E':
	    if (*watches == 'E')
	    {
		args.add_tedit = 1;
		++watches;
	    }
	    if (*watches == 'U')
	    {
		args.add_tunedit = 1;
		++watches;
	    }
	    if (*watches == 'C')
	    {
		args.add_tcommit = 1;
	    }
	    watch_modify_watchers (filename, &args);
	    break;
	case 'U':
	case 'C':
	    args.remove_temp = 1;
	    watch_modify_watchers (filename, &args);
	    break;
    }
}



#ifdef CLIENT_SUPPORT
/* Check and send notifications.  This is only for the client.  */
void
notify_check (const char *repository, const char *update_dir)
{
    FILE *fp;
    char *line = NULL;
    size_t line_len = 0;

    if (!server_started)
	/* We are in the midst of a command which is not to talk to
	   the server (e.g. the first phase of a cvs edit).  Just chill
	   out, we'll catch the notifications on the flip side.  */
	return;

    /* We send notifications even if noexec.  I'm not sure which behavior
       is most sensible.  */

    fp = CVS_FOPEN (CVSADM_NOTIFY, "r");
    if (fp == NULL)
    {
	if (!existence_error (errno))
	    error (0, errno, "cannot open %s", CVSADM_NOTIFY);
	return;
    }
    while (getline (&line, &line_len, fp) > 0)
    {
	int notif_type;
	char *filename;
	char *val;
	char *cp;

	notif_type = line[0];
	if (notif_type == '\0')
	    continue;
	filename = line + 1;
	cp = strchr (filename, '\t');
	if (cp == NULL)
	    continue;
	*cp++ = '\0';
	val = cp;

	client_notify (repository, update_dir, filename, notif_type, val);
    }
    if (line)
	free (line);
    if (ferror (fp))
	error (0, errno, "cannot read %s", CVSADM_NOTIFY);
    if (fclose (fp) < 0)
	error (0, errno, "cannot close %s", CVSADM_NOTIFY);

    /* Leave the CVSADM_NOTIFY file there, until the server tells us it
       has dealt with it.  */
}
#endif /* CLIENT_SUPPORT */


static const char *const editors_usage[] =
{
    "Usage: %s %s [-lR] [<file>]...\n",
    "-l\tProcess this directory only (not recursive).\n",
    "-R\tProcess directories recursively (default).\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};



static int
editors_fileproc (void *callerdat, struct file_info *finfo)
{
    return find_editors_and_output (finfo);
}



int
editors (int argc, char **argv)
{
    int local = 0;
    int c;

    if (argc == -1)
	usage (editors_usage);

    getoptreset ();
    while ((c = getopt (argc, argv, "+lR")) != -1)
    {
	switch (c)
	{
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case '?':
	    default:
		usage (editors_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	start_server ();
	ign_setup ();

	if (local)
	    send_arg ("-l");
	send_arg ("--");
	send_files (argc, argv, local, 0, SEND_NO_CONTENTS);
	send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_to_server ("editors\012", 0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    return start_recursion (editors_fileproc, NULL, NULL, NULL, NULL,
			    argc, argv, local, W_LOCAL, 0, CVS_LOCK_READ, NULL,
			    0, NULL);
}
