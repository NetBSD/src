/* Implementation for "cvs watch add", "cvs watchers", and related commands

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"
#include "edit.h"
#include "fileattr.h"
#include "watch.h"

const char *const watch_usage[] =
{
    "Usage: %s %s {on|off|add|remove} [-lR] [-a <action>]... [<path>]...\n",
    "on/off: Turn on/off read-only checkouts of files.\n",
    "add/remove: Add or remove notification on actions.\n",
    "-l (on/off/add/remove): Local directory only, not recursive.\n",
    "-R (on/off/add/remove): Process directories recursively (default).\n",
    "-a (add/remove): Specify what actions, one of: `edit', `unedit',\n",
    "                 `commit', `all', or `none' (defaults to `all').\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};

static struct addremove_args the_args;

void
watch_modify_watchers (const char *file, struct addremove_args *what)
{
    char *curattr = fileattr_get0 (file, "_watchers");
    char *p;
    char *pend;
    char *nextp;
    char *who;
    int who_len;
    char *mycurattr;
    char *mynewattr;
    size_t mynewattr_size;

    int add_edit_pending;
    int add_unedit_pending;
    int add_commit_pending;
    int remove_edit_pending;
    int remove_unedit_pending;
    int remove_commit_pending;
    int add_tedit_pending;
    int add_tunedit_pending;
    int add_tcommit_pending;

    TRACE( TRACE_FUNCTION, "modify_watchers ( %s )", file );

    who = getcaller ();
    who_len = strlen (who);

    /* Look for current watcher types for this user.  */
    mycurattr = NULL;
    if (curattr != NULL)
    {
	p = curattr;
	while (1) {
	    if (strncmp (who, p, who_len) == 0
		&& p[who_len] == '>')
	    {
		/* Found this user.  */
		mycurattr = p + who_len + 1;
	    }
	    p = strchr (p, ',');
	    if (p == NULL)
		break;
	    ++p;
	}
    }
    if (mycurattr != NULL)
    {
	mycurattr = xstrdup (mycurattr);
	p = strchr (mycurattr, ',');
	if (p != NULL)
	    *p = '\0';
    }

    /* Now copy mycurattr to mynewattr, making the requisite modifications.
       Note that we add a dummy '+' to the start of mynewattr, to reduce
       special cases (but then we strip it off when we are done).  */

    mynewattr_size = sizeof "+edit+unedit+commit+tedit+tunedit+tcommit";
    if (mycurattr != NULL)
	mynewattr_size += strlen (mycurattr);
    mynewattr = xmalloc (mynewattr_size);
    mynewattr[0] = '\0';

    add_edit_pending = what->adding && what->edit;
    add_unedit_pending = what->adding && what->unedit;
    add_commit_pending = what->adding && what->commit;
    remove_edit_pending = !what->adding && what->edit;
    remove_unedit_pending = !what->adding && what->unedit;
    remove_commit_pending = !what->adding && what->commit;
    add_tedit_pending = what->add_tedit;
    add_tunedit_pending = what->add_tunedit;
    add_tcommit_pending = what->add_tcommit;

    /* Copy over existing watch types, except those to be removed.  */
    p = mycurattr;
    while (p != NULL)
    {
	pend = strchr (p, '+');
	if (pend == NULL)
	{
	    pend = p + strlen (p);
	    nextp = NULL;
	}
	else
	    nextp = pend + 1;

	/* Process this item.  */
	if (pend - p == 4 && strncmp ("edit", p, 4) == 0)
	{
	    if (!remove_edit_pending)
		strcat (mynewattr, "+edit");
	    add_edit_pending = 0;
	}
	else if (pend - p == 6 && strncmp ("unedit", p, 6) == 0)
	{
	    if (!remove_unedit_pending)
		strcat (mynewattr, "+unedit");
	    add_unedit_pending = 0;
	}
	else if (pend - p == 6 && strncmp ("commit", p, 6) == 0)
	{
	    if (!remove_commit_pending)
		strcat (mynewattr, "+commit");
	    add_commit_pending = 0;
	}
	else if (pend - p == 5 && strncmp ("tedit", p, 5) == 0)
	{
	    if (!what->remove_temp)
		strcat (mynewattr, "+tedit");
	    add_tedit_pending = 0;
	}
	else if (pend - p == 7 && strncmp ("tunedit", p, 7) == 0)
	{
	    if (!what->remove_temp)
		strcat (mynewattr, "+tunedit");
	    add_tunedit_pending = 0;
	}
	else if (pend - p == 7 && strncmp ("tcommit", p, 7) == 0)
	{
	    if (!what->remove_temp)
		strcat (mynewattr, "+tcommit");
	    add_tcommit_pending = 0;
	}
	else
	{
	    char *mp;

	    /* Copy over any unrecognized watch types, for future
	       expansion.  */
	    mp = mynewattr + strlen (mynewattr);
	    *mp++ = '+';
	    strncpy (mp, p, pend - p);
	    *(mp + (pend - p)) = '\0';
	}

	/* Set up for next item.  */
	p = nextp;
    }

    /* Add in new watch types.  */
    if (add_edit_pending)
	strcat (mynewattr, "+edit");
    if (add_unedit_pending)
	strcat (mynewattr, "+unedit");
    if (add_commit_pending)
	strcat (mynewattr, "+commit");
    if (add_tedit_pending)
	strcat (mynewattr, "+tedit");
    if (add_tunedit_pending)
	strcat (mynewattr, "+tunedit");
    if (add_tcommit_pending)
	strcat (mynewattr, "+tcommit");

    {
	char *curattr_new;

	curattr_new =
	  fileattr_modify (curattr,
			   who,
			   mynewattr[0] == '\0' ? NULL : mynewattr + 1,
			   '>',
			   ',');
	/* If the attribute is unchanged, don't rewrite the attribute file.  */
	if (!((curattr_new == NULL && curattr == NULL)
	      || (curattr_new != NULL
		  && curattr != NULL
		  && strcmp (curattr_new, curattr) == 0)))
	    fileattr_set (file,
			  "_watchers",
			  curattr_new);
	if (curattr_new != NULL)
	    free (curattr_new);
    }

    if (curattr != NULL)
	free (curattr);
    if (mycurattr != NULL)
	free (mycurattr);
    if (mynewattr != NULL)
	free (mynewattr);
}

static int addremove_fileproc (void *callerdat,
				      struct file_info *finfo);

static int
addremove_fileproc (void *callerdat, struct file_info *finfo)
{
    watch_modify_watchers (finfo->file, &the_args);
    return 0;
}

static int addremove_filesdoneproc (void * callerdat, int err, const char * repository,
                                           const char *update_dir, List * entries)
{
    int set_default = the_args.setting_default;
    int dir_check = 0;

    while ( !set_default && dir_check < the_args.num_dirs )
    {
	/* If we are recursing, then just see if the first part of update_dir 
	   matches any of the specified directories. Otherwise, it must be an exact
	   match. */
	if ( the_args.local )
	    set_default = strcmp( update_dir, the_args.dirs[ dir_check ] )==0;
	else 
	    set_default = strncmp( update_dir, the_args.dirs[ dir_check ], strlen( the_args.dirs[ dir_check ] ) ) == 0;
	dir_check++;
    }

    if (set_default)
	watch_modify_watchers (NULL, &the_args);
    return err;
}


static int
watch_addremove (int argc, char **argv)
{
    int c;
    int err;
    int a_omitted;
    int arg_index;
    int max_dirs;

    a_omitted = 1;
    the_args.commit = 0;
    the_args.edit = 0;
    the_args.unedit = 0;
    the_args.num_dirs = 0;
    the_args.dirs = NULL;
    the_args.local = 0;

    getoptreset ();
    while ((c = getopt (argc, argv, "+lRa:")) != -1)
    {
	switch (c)
	{
	    case 'l':
		the_args.local = 1;
		break;
	    case 'R':
		the_args.local = 0;
		break;
	    case 'a':
		a_omitted = 0;
		if (strcmp (optarg, "edit") == 0)
		    the_args.edit = 1;
		else if (strcmp (optarg, "unedit") == 0)
		    the_args.unedit = 1;
		else if (strcmp (optarg, "commit") == 0)
		    the_args.commit = 1;
		else if (strcmp (optarg, "all") == 0)
		{
		    the_args.edit = 1;
		    the_args.unedit = 1;
		    the_args.commit = 1;
		}
		else if (strcmp (optarg, "none") == 0)
		{
		    the_args.edit = 0;
		    the_args.unedit = 0;
		    the_args.commit = 0;
		}
		else
		    usage (watch_usage);
		break;
	    case '?':
	    default:
		usage (watch_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    the_args.num_dirs = 0;
    max_dirs = 4; /* Arbitrary choice. */
    the_args.dirs = xmalloc( sizeof( const char * ) * max_dirs );

    TRACE (TRACE_FUNCTION, "watch_addremove (%d)", argc);
    for ( arg_index=0; arg_index<argc; ++arg_index )
    {
	TRACE( TRACE_FUNCTION, "\t%s", argv[ arg_index ]);
	if ( isdir( argv[ arg_index ] ) )
	{
	    if ( the_args.num_dirs >= max_dirs )
	    {
		max_dirs *= 2;
		the_args.dirs = (const char ** )xrealloc( (void *)the_args.dirs, max_dirs );
	    }
	    the_args.dirs[ the_args.num_dirs++ ] = argv[ arg_index ];
	}
    }

    if (a_omitted)
    {
	the_args.edit = 1;
	the_args.unedit = 1;
	the_args.commit = 1;
    }

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	start_server ();
	ign_setup ();

	if (the_args.local)
	    send_arg ("-l");
	/* FIXME: copes poorly with "all" if server is extended to have
	   new watch types and client is still running an old version.  */
	if (the_args.edit)
	    option_with_arg ("-a", "edit");
	if (the_args.unedit)
	    option_with_arg ("-a", "unedit");
	if (the_args.commit)
	    option_with_arg ("-a", "commit");
	if (!the_args.edit && !the_args.unedit && !the_args.commit)
	    option_with_arg ("-a", "none");
	send_arg ("--");
	send_files (argc, argv, the_args.local, 0, SEND_NO_CONTENTS);
	send_file_names (argc, argv, SEND_EXPAND_WILD);
	send_to_server (the_args.adding ?
                        "watch-add\012" : "watch-remove\012",
                        0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    the_args.setting_default = (argc <= 0);

    lock_tree_promotably (argc, argv, the_args.local, W_LOCAL, 0);

    err = start_recursion
	(addremove_fileproc, addremove_filesdoneproc, NULL, NULL, NULL,
	 argc, argv, the_args.local, W_LOCAL, 0, CVS_LOCK_WRITE,
	 NULL, 1, NULL);

    Lock_Cleanup ();
    free( (void *)the_args.dirs );
    the_args.dirs = NULL;

    return err;
}



int
watch_add (int argc, char **argv)
{
    the_args.adding = 1;
    return watch_addremove (argc, argv);
}

int
watch_remove (int argc, char **argv)
{
    the_args.adding = 0;
    return watch_addremove (argc, argv);
}

int
watch (int argc, char **argv)
{
    if (argc <= 1)
	usage (watch_usage);
    if (strcmp (argv[1], "on") == 0)
    {
	--argc;
	++argv;
	return watch_on (argc, argv);
    }
    else if (strcmp (argv[1], "off") == 0)
    {
	--argc;
	++argv;
	return watch_off (argc, argv);
    }
    else if (strcmp (argv[1], "add") == 0)
    {
	--argc;
	++argv;
	return watch_add (argc, argv);
    }
    else if (strcmp (argv[1], "remove") == 0)
    {
	--argc;
	++argv;
	return watch_remove (argc, argv);
    }
    else
	usage (watch_usage);
    return 0;
}

static const char *const watchers_usage[] =
{
    "Usage: %s %s [-lR] [<file>]...\n",
    "-l\tProcess this directory only (not recursive).\n",
    "-R\tProcess directories recursively (default).\n",
    "(Specify the --help global option for a list of other help options.)\n",
    NULL
};

static int watchers_fileproc (void *callerdat,
				     struct file_info *finfo);

static int
watchers_fileproc (void *callerdat, struct file_info *finfo)
{
    char *them;
    char *p;

    them = fileattr_get0 (finfo->file, "_watchers");
    if (them == NULL)
	return 0;

    cvs_output (finfo->fullname, 0);

    p = them;
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
		goto out;
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
  out:;
    free (them);
    return 0;
}

int
watchers (int argc, char **argv)
{
    int local = 0;
    int c;

    if (argc == -1)
	usage (watchers_usage);

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
		usage (watchers_usage);
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
	send_to_server ("watchers\012", 0);
	return get_responses_and_close ();
    }
#endif /* CLIENT_SUPPORT */

    return start_recursion (watchers_fileproc, NULL, NULL,
			    NULL, NULL, argc, argv, local, W_LOCAL, 0,
			    CVS_LOCK_READ, NULL, 1, NULL);
}
