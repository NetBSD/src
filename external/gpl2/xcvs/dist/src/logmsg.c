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

static int find_type (Node * p, void *closure);
static int fmt_proc (Node * p, void *closure);
static int logfile_write (const char *repository, const char *filter,
			  const char *message, FILE * logfp, List * changes);
static int logmsg_list_to_args_proc (Node *p, void *closure);
static int rcsinfo_proc (const char *repository, const char *template,
                         void *closure );
static int update_logfile_proc (const char *repository, const char *filter,
                                void *closure);
static void setup_tmpfile (FILE * xfp, char *xprefix, List * changes);
static int verifymsg_proc (const char *repository, const char *script,
                           void *closure );

static FILE *fp;
static Ctype type;

struct verifymsg_proc_data
{
    /* The name of the temp file storing the log message to be verified.  This
     * is initially NULL and verifymsg_proc() writes message into it so that it
     * can be shared when multiple verifymsg scripts exist.  do_verify() is
     * responsible for rereading the message from the file when
     * RereadLogAfterVerify is in effect and the file has changed.
     */
    char *fname;
    /* The initial message text to be verified.
     */
    char *message;
    /* The initial stats of the temp file so we can tell that the temp file has
     * been changed when RereadLogAfterVerify is STAT.
     */
    struct stat pre_stbuf;
   /* The list of files being changed, with new and old version numbers.
    */
   List *changes;
};

/*
 * Puts a standard header on the output which is either being prepared for an
 * editor session, or being sent to a logfile program.  The modified, added,
 * and removed files are included (if any) and formatted to look pretty. */
static char *prefix;
static int col;
static char *tag;
static void
setup_tmpfile (FILE *xfp, char *xprefix, List *changes)
{
    /* set up statics */
    fp = xfp;
    prefix = xprefix;

    type = T_MODIFIED;
    if (walklist (changes, find_type, NULL) != 0)
    {
	(void) fprintf (fp, "%sModified Files:\n", prefix);
	col = 0;
	(void) walklist (changes, fmt_proc, NULL);
	(void) fprintf (fp, "\n");
	if (tag != NULL)
	{
	    free (tag);
	    tag = NULL;
	}
    }
    type = T_ADDED;
    if (walklist (changes, find_type, NULL) != 0)
    {
	(void) fprintf (fp, "%sAdded Files:\n", prefix);
	col = 0;
	(void) walklist (changes, fmt_proc, NULL);
	(void) fprintf (fp, "\n");
	if (tag != NULL)
	{
	    free (tag);
	    tag = NULL;
	}
    }
    type = T_REMOVED;
    if (walklist (changes, find_type, NULL) != 0)
    {
	(void) fprintf (fp, "%sRemoved Files:\n", prefix);
	col = 0;
	(void) walklist (changes, fmt_proc, NULL);
	(void) fprintf (fp, "\n");
	if (tag != NULL)
	{
	    free (tag);
	    tag = NULL;
	}
    }
}

/*
 * Looks for nodes of a specified type and returns 1 if found
 */
static int
find_type (Node *p, void *closure)
{
    struct logfile_info *li = p->data;

    if (li->type == type)
	return (1);
    else
	return (0);
}

/*
 * Breaks the files list into reasonable sized lines to avoid line wrap...
 * all in the name of pretty output.  It only works on nodes whose types
 * match the one we're looking for
 */
static int
fmt_proc (Node *p, void *closure)
{
    struct logfile_info *li;

    li = p->data;
    if (li->type == type)
    {
        if (li->tag == NULL
	    ? tag != NULL
	    : tag == NULL || strcmp (tag, li->tag) != 0)
	{
	    if (col > 0)
	        (void) fprintf (fp, "\n");
	    (void) fputs (prefix, fp);
	    col = strlen (prefix);
	    while (col < 6)
	    {
	        (void) fprintf (fp, " ");
		++col;
	    }

	    if (li->tag == NULL)
	        (void) fprintf (fp, "No tag");
	    else
	        (void) fprintf (fp, "Tag: %s", li->tag);

	    if (tag != NULL)
	        free (tag);
	    tag = xstrdup (li->tag);

	    /* Force a new line.  */
	    col = 70;
	}

	if (col == 0)
	{
	    (void) fprintf (fp, "%s\t", prefix);
	    col = 8;
	}
	else if (col > 8 && (col + (int) strlen (p->key)) > 70)
	{
	    (void) fprintf (fp, "\n%s\t", prefix);
	    col = 8;
	}
	(void) fprintf (fp, "%s ", p->key);
	col += strlen (p->key) + 1;
    }
    return (0);
}

/*
 * Builds a temporary file using setup_tmpfile() and invokes the user's
 * editor on the file.  The header garbage in the resultant file is then
 * stripped and the log message is stored in the "message" argument.
 * 
 * If REPOSITORY is non-NULL, process rcsinfo for that repository; if it
 * is NULL, use the CVSADM_TEMPLATE file instead.  REPOSITORY should be
 * NULL when running in client mode.
 *
 * GLOBALS
 *   Editor     Set to a default value by configure and overridable using the
 *              -e option to the CVS executable.
 */
void
do_editor (const char *dir, char **messagep, const char *repository,
           List *changes)
{
    static int reuse_log_message = 0;
    char *line;
    int line_length;
    size_t line_chars_allocated;
    char *fname;
    struct stat pre_stbuf, post_stbuf;
    int retcode = 0;

    assert (!current_parsed_root->isremote != !repository);

    if (noexec || reuse_log_message)
	return;

    /* Abort before creation of the temp file if no editor is defined. */
    if (strcmp (Editor, "") == 0)
        error(1, 0, "no editor defined, must use -e or -m");

  again:
    /* Create a temporary file.  */
    if( ( fp = cvs_temp_file( &fname ) ) == NULL )
	error( 1, errno, "cannot create temporary file" );

    if (*messagep)
    {
	(void) fputs (*messagep, fp);

	if ((*messagep)[0] == '\0' ||
	    (*messagep)[strlen (*messagep) - 1] != '\n')
	    (void) fprintf (fp, "\n");
    }
    else
	(void) fprintf (fp, "\n");

    if (repository != NULL)
	/* tack templates on if necessary */
	(void) Parse_Info (CVSROOTADM_RCSINFO, repository, rcsinfo_proc,
		PIOPT_ALL, NULL);
    else
    {
	FILE *tfp;
	char buf[1024];
	size_t n;
	size_t nwrite;

	/* Why "b"?  */
	tfp = CVS_FOPEN (CVSADM_TEMPLATE, "rb");
	if (tfp == NULL)
	{
	    if (!existence_error (errno))
		error (1, errno, "cannot read %s", CVSADM_TEMPLATE);
	}
	else
	{
	    while (!feof (tfp))
	    {
		char *p = buf;
		n = fread (buf, 1, sizeof buf, tfp);
		nwrite = n;
		while (nwrite > 0)
		{
		    n = fwrite (p, 1, nwrite, fp);
		    nwrite -= n;
		    p += n;
		}
		if (ferror (tfp))
		    error (1, errno, "cannot read %s", CVSADM_TEMPLATE);
	    }
	    if (fclose (tfp) < 0)
		error (0, errno, "cannot close %s", CVSADM_TEMPLATE);
	}
    }

    (void) fprintf (fp,
  "%s----------------------------------------------------------------------\n",
		    CVSEDITPREFIX);
    (void) fprintf (fp,
  "%sEnter Log.  Lines beginning with `%.*s' are removed automatically\n%s\n",
		    CVSEDITPREFIX, CVSEDITPREFIXLEN, CVSEDITPREFIX,
		    CVSEDITPREFIX);
    if (dir != NULL && *dir)
	(void) fprintf (fp, "%sCommitting in %s\n%s\n", CVSEDITPREFIX,
			dir, CVSEDITPREFIX);
    if (changes != NULL)
	setup_tmpfile (fp, CVSEDITPREFIX, changes);
    (void) fprintf (fp,
  "%s----------------------------------------------------------------------\n",
		    CVSEDITPREFIX);

    /* finish off the temp file */
    if (fclose (fp) == EOF)
        error (1, errno, "%s", fname);
    if (stat (fname, &pre_stbuf) == -1)
	pre_stbuf.st_mtime = 0;

    /* run the editor */
    run_setup (Editor);
    run_add_arg (fname);
    if ((retcode = run_exec (RUN_TTY, RUN_TTY, RUN_TTY,
			     RUN_NORMAL | RUN_SIGIGNORE | RUN_UNSETXID)) != 0)
	error (0, retcode == -1 ? errno : 0, "warning: editor session failed");

    /* put the entire message back into the *messagep variable */

    fp = xfopen (fname, "r");

    if (*messagep)
	free (*messagep);

    if (stat (fname, &post_stbuf) != 0)
	    error (1, errno, "cannot find size of temp file %s", fname);

    if (post_stbuf.st_size == 0)
	*messagep = NULL;
    else
    {
	/* On NT, we might read less than st_size bytes, but we won't
	   read more.  So this works.  */
	*messagep = (char *) xmalloc (post_stbuf.st_size + 1);
 	(*messagep)[0] = '\0';
    }

    line = NULL;
    line_chars_allocated = 0;

    if (*messagep)
    {
	size_t message_len = post_stbuf.st_size + 1;
	size_t offset = 0;
	while (1)
	{
	    line_length = getline (&line, &line_chars_allocated, fp);
	    if (line_length == -1)
	    {
		if (ferror (fp))
		    error (0, errno, "warning: cannot read %s", fname);
		break;
	    }
	    if (strncmp (line, CVSEDITPREFIX, CVSEDITPREFIXLEN) == 0)
		continue;
	    if (offset + line_length >= message_len)
		expand_string (messagep, &message_len,
				offset + line_length + 1);
	    (void) strcpy (*messagep + offset, line);
	    offset += line_length;
	}
    }
    if (fclose (fp) < 0)
	error (0, errno, "warning: cannot close %s", fname);

    /* canonicalize emply messages */
    if (*messagep != NULL &&
        (**messagep == '\0' || strcmp (*messagep, "\n") == 0))
    {
	free (*messagep);
	*messagep = NULL;
    }

    if (pre_stbuf.st_mtime == post_stbuf.st_mtime || *messagep == NULL)
    {
	for (;;)
	{
	    (void) printf ("\nLog message unchanged or not specified\n");
	    (void) printf ("a)bort, c)ontinue, e)dit, !)reuse this message unchanged for remaining dirs\n");
	    (void) printf ("Action: (continue) ");
	    (void) fflush (stdout);
	    line_length = getline (&line, &line_chars_allocated, stdin);
	    if (line_length < 0)
	    {
		error (0, errno, "cannot read from stdin");
		if (unlink_file (fname) < 0)
		    error (0, errno,
			   "warning: cannot remove temp file %s", fname);
		error (1, 0, "aborting");
	    }
	    else if (line_length == 0
		     || *line == '\n' || *line == 'c' || *line == 'C')
		break;
	    if (*line == 'a' || *line == 'A')
		{
		    if (unlink_file (fname) < 0)
			error (0, errno, "warning: cannot remove temp file %s", fname);
		    error (1, 0, "aborted by user");
		}
	    if (*line == 'e' || *line == 'E')
		goto again;
	    if (*line == '!')
	    {
		reuse_log_message = 1;
		break;
	    }
	    (void) printf ("Unknown input\n");
	}
    }
    if (line)
	free (line);
    if (unlink_file (fname) < 0)
	error (0, errno, "warning: cannot remove temp file %s", fname);
    free (fname);
}

/* Runs the user-defined verification script as part of the commit or import 
   process.  This verification is meant to be run whether or not the user 
   included the -m attribute.  unlike the do_editor function, this is 
   independant of the running of an editor for getting a message.
 */
void
do_verify (char **messagep, const char *repository, List *changes)
{
    int err;
    struct verifymsg_proc_data data;
    struct stat post_stbuf;

    if (current_parsed_root->isremote)
	/* The verification will happen on the server.  */
	return;

    /* FIXME? Do we really want to skip this on noexec?  What do we do
       for the other administrative files?  */
    /* EXPLAIN: Why do we check for repository == NULL here? */
    if (noexec || repository == NULL)
	return;

    /* Get the name of the verification script to run  */

    data.message = *messagep;
    data.fname = NULL;
    data.changes = changes;
    if ((err = Parse_Info (CVSROOTADM_VERIFYMSG, repository,
	                  verifymsg_proc, 0, &data)) != 0)
    {
	int saved_errno = errno;
	/* Since following error() exits, delete the temp file now.  */
	if (data.fname != NULL && unlink_file( data.fname ) < 0)
	    error (0, errno, "cannot remove %s", data.fname);
	free (data.fname);

	errno = saved_errno;
	error (1, err == -1 ? errno : 0, "Message verification failed");
    }

    /* Return if no temp file was created.  That means that we didn't call any
     * verifymsg scripts.
     */
    if (data.fname == NULL)
	return;

    /* Get the mod time and size of the possibly new log message
     * in always and stat modes.
     */
    if (config->RereadLogAfterVerify == LOGMSG_REREAD_ALWAYS ||
	config->RereadLogAfterVerify == LOGMSG_REREAD_STAT)
    {
	if(stat (data.fname, &post_stbuf) != 0)
	    error (1, errno, "cannot find size of temp file %s", data.fname);
    }

    /* And reread the log message in `always' mode or in `stat' mode when it's
     * changed.
     */
    if (config->RereadLogAfterVerify == LOGMSG_REREAD_ALWAYS ||
	(config->RereadLogAfterVerify == LOGMSG_REREAD_STAT &&
	  (data.pre_stbuf.st_mtime != post_stbuf.st_mtime ||
	    data.pre_stbuf.st_size != post_stbuf.st_size)))
    {
	/* put the entire message back into the *messagep variable */

	if (*messagep) free (*messagep);

	if (post_stbuf.st_size == 0)
	    *messagep = NULL;
	else
	{
	    char *line = NULL;
	    int line_length;
	    size_t line_chars_allocated = 0;
	    char *p;
	    FILE *fp;

	    fp = xfopen (data.fname, "r");

	    /* On NT, we might read less than st_size bytes,
	       but we won't read more.  So this works.  */
	    p = *messagep = (char *) xmalloc (post_stbuf.st_size + 1);
	    *messagep[0] = '\0';

	    for (;;)
	    {
		line_length = getline( &line,
				       &line_chars_allocated,
				       fp);
		if (line_length == -1)
		{
		    if (ferror (fp))
			/* Fail in this case because otherwise we will have no
			 * log message
			 */
			error (1, errno, "cannot read %s", data.fname);
		    break;
		}
		if (strncmp (line, CVSEDITPREFIX, CVSEDITPREFIXLEN) == 0)
		    continue;
		(void) strcpy (p, line);
		p += line_length;
	    }
	    if (line) free (line);
	    if (fclose (fp) < 0)
	        error (0, errno, "warning: cannot close %s", data.fname);
	}
    }
    /* Delete the temp file  */
    if (unlink_file (data.fname) < 0)
	error (0, errno, "cannot remove `%s'", data.fname);
    free (data.fname);
}



/*
 * callback proc for Parse_Info for rcsinfo templates this routine basically
 * copies the matching template onto the end of the tempfile we are setting
 * up
 */
/* ARGSUSED */
static int
rcsinfo_proc (const char *repository, const char *template, void *closure)
{
    static char *last_template;
    FILE *tfp;

    /* nothing to do if the last one included is the same as this one */
    if (last_template && strcmp (last_template, template) == 0)
	return (0);
    if (last_template)
	free (last_template);
    last_template = xstrdup (template);

    if ((tfp = CVS_FOPEN (template, "r")) != NULL)
    {
	char *line = NULL;
	size_t line_chars_allocated = 0;

	while (getline (&line, &line_chars_allocated, tfp) >= 0)
	    (void) fputs (line, fp);
	if (ferror (tfp))
	    error (0, errno, "warning: cannot read %s", template);
	if (fclose (tfp) < 0)
	    error (0, errno, "warning: cannot close %s", template);
	if (line)
	    free (line);
	return (0);
    }
    else
    {
	error (0, errno, "Couldn't open rcsinfo template file %s", template);
	return (1);
    }
}

/*
 * Uses setup_tmpfile() to pass the updated message on directly to any
 * logfile programs that have a regular expression match for the checked in
 * directory in the source repository.  The log information is fed into the
 * specified program as standard input.
 */
struct ulp_data {
    FILE *logfp;
    const char *message;
    List *changes;
};



void
Update_Logfile (const char *repository, const char *xmessage, FILE *xlogfp,
                List *xchanges)
{
    struct ulp_data ud;

    /* nothing to do if the list is empty */
    if (xchanges == NULL || xchanges->list->next == xchanges->list)
	return;

    /* set up vars for update_logfile_proc */
    ud.message = xmessage;
    ud.logfp = xlogfp;
    ud.changes = xchanges;

    /* call Parse_Info to do the actual logfile updates */
    (void) Parse_Info (CVSROOTADM_LOGINFO, repository, update_logfile_proc,
		       PIOPT_ALL, &ud);
}



/*
 * callback proc to actually do the logfile write from Update_Logfile
 */
static int
update_logfile_proc (const char *repository, const char *filter, void *closure)
{
    struct ulp_data *udp = closure;
    TRACE (TRACE_FUNCTION, "update_logfile_proc(%s,%s)", repository, filter);
    return logfile_write (repository, filter, udp->message, udp->logfp,
                          udp->changes);
}



/* static int
 * logmsg_list_to_args_proc( Node *p, void *closure )
 * This function is intended to be passed into walklist() with a list of tags
 * (nodes in the same format as pretag_list_proc() accepts - p->key = tagname
 * and p->data = a revision.
 *
 * closure will be a struct format_cmdline_walklist_closure
 * where closure is undefined.
 */
static int
logmsg_list_to_args_proc (Node *p, void *closure)
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
		arg = p->key;
		break;
	    case 'T':
	    case 't':
		li = p->data;
		arg = li->tag ? li->tag : "";
		break;
	    case 'V':
		li = p->data;
		arg = li->rev_old ? li->rev_old : "NONE";
		break;
	    case 'v':
		li = p->data;
		arg = li->rev_new ? li->rev_new : "NONE";
		break;
	    default:
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		if (c->onearg)
		{
		    /* The old deafult was to print the empty string for
		     * unknown args.
		     */
		    arg = "\0";
		}
		else
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		    error (1, 0,
		           "Unknown format character or not a list attribute: %c", f[-1]);
		/* NOTREACHED */
		break;
	}
	/* copy the attribute into an argument */
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
	if (c->onearg)
	{
	    if (c->firstpass)
	    {
		c->firstpass = 0;
		doff = d - *c->buf;
		expand_string (c->buf, c->length,
		               doff + strlen (c->srepos) + 1);
		d = *c->buf + doff;
		strncpy (d, c->srepos, strlen (c->srepos));
		d += strlen (c->srepos);
	    	*d++ = ' ';
	    }
	}
	else /* c->onearg */
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
	{
	    if (c->quotes)
	    {
		arg = cmdlineescape (c->quotes, arg);
	    }
	    else
	    {
		arg = cmdlinequote ('"', arg);
	    }
	} /* !c->onearg */
	doff = d - *c->buf;
	expand_string (c->buf, c->length, doff + strlen (arg));
	d = *c->buf + doff;
	strncpy (d, arg, strlen (arg));
	d += strlen (arg);
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
	if (!c->onearg)
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
	    free (arg);

	/* Always put the extra space on.  we'll have to back up a char
	 * when we're done, but that seems most efficient.
	 */
	doff = d - *c->buf;
	expand_string (c->buf, c->length, doff + 1);
	d = *c->buf + doff;
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
	if (c->onearg && *f) *d++ = ',';
	else
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
	    *d++ = ' ';
    }
    /* correct our original pointer into the buff */
    *c->d = d;
    return 0;
}



/*
 * Writes some stuff to the logfile "filter" and returns the status of the
 * filter program.
 */
static int
logfile_write (const char *repository, const char *filter, const char *message,
               FILE *logfp, List *changes)
{
    char *cmdline;
    FILE *pipefp;
    char *cp;
    int c;
    int pipestatus;
    const char *srepos = Short_Repository (repository);

    assert (repository);

    /* The user may specify a format string as part of the filter.
       Originally, `%s' was the only valid string.  The string that
       was substituted for it was:

         <repository-name> <file1> <file2> <file3> ...

       Each file was either a new directory/import (T_TITLE), or a
       added (T_ADDED), modified (T_MODIFIED), or removed (T_REMOVED)
       file.

       It is desirable to preserve that behavior so lots of commitlog
       scripts won't die when they get this new code.  At the same
       time, we'd like to pass other information about the files (like
       version numbers, statuses, or checkin times).

       The solution is to allow a format string that allows us to
       specify those other pieces of information.  The format string
       will be composed of `%' followed by a single format character,
       or followed by a set of format characters surrounded by `{' and
       `}' as separators.  The format characters are:

         s = file name
	 V = old version number (pre-checkin)
	 v = new version number (post-checkin)

       For example, valid format strings are:

         %{}
	 %s
	 %{s}
	 %{sVv}

       There's no reason that more items couldn't be added (like
       modification date or file status [added, modified, updated,
       etc.]) -- the code modifications would be minimal (logmsg.c
       (title_proc) and commit.c (check_fileproc)).

       The output will be a string of tokens separated by spaces.  For
       backwards compatibility, the the first token will be the
       repository name.  The rest of the tokens will be
       comma-delimited lists of the information requested in the
       format string.  For example, if `/u/src/master' is the
       repository, `%{sVv}' is the format string, and three files
       (ChangeLog, Makefile, foo.c) were modified, the output might
       be:

         /u/src/master ChangeLog,1.1,1.2 Makefile,1.3,1.4 foo.c,1.12,1.13

       Why this duplicates the old behavior when the format string is
       `%s' is left as an exercise for the reader. */

    /* %c = cvs_cmd_name
     * %p = shortrepos
     * %r = repository
     * %{sVv} = file name, old revision (precommit), new revision (postcommit)
     */
    /*
     * Cast any NULL arguments as appropriate pointers as this is an
     * stdarg function and we need to be certain the caller gets what
     * is expected.
     */
    cmdline = format_cmdline (
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
	                      !config->UseNewInfoFmtStrings, srepos,
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
	                      filter,
	                      "c", "s", cvs_cmd_name,
#ifdef SERVER_SUPPORT
	                      "R", "s", referrer ? referrer->original : "NONE",
#endif /* SERVER_SUPPORT */
	                      "p", "s", srepos,
	                      "r", "s", current_parsed_root->directory,
	                      "sVv", ",", changes,
			      logmsg_list_to_args_proc, (void *) NULL,
	                      (char *) NULL);
    if (!cmdline || !strlen (cmdline))
    {
	if (cmdline) free (cmdline);
	error (0, 0, "logmsg proc resolved to the empty string!");
	return 1;
    }

    if ((pipefp = run_popen (cmdline, "w")) == NULL)
    {
	if (!noexec)
	    error (0, 0, "cannot write entry to log filter: %s", cmdline);
	free (cmdline);
	return 1;
    }
    (void) fprintf (pipefp, "Update of %s\n", repository);
    (void) fprintf (pipefp, "In directory %s:", hostname);
    cp = xgetcwd ();
    if (cp == NULL)
	fprintf (pipefp, "<cannot get working directory: %s>\n\n",
		 strerror (errno));
    else
    {
	fprintf (pipefp, "%s\n\n", cp);
	free (cp);
    }

    setup_tmpfile (pipefp, "", changes);
    (void) fprintf (pipefp, "Log Message:\n%s\n", (message) ? message : "");
    if (logfp)
    {
	(void) fprintf (pipefp, "Status:\n");
	rewind (logfp);
	while ((c = getc (logfp)) != EOF)
	    (void) putc (c, pipefp);
    }
    free (cmdline);
    pipestatus = pclose (pipefp);
    return ((pipestatus == -1) || (pipestatus == 127)) ? 1 : 0;
}



/*  This routine is called by Parse_Info.  It runs the
 *  message verification script.
 */
static int
verifymsg_proc (const char *repository, const char *script, void *closure)
{
    char *verifymsg_script;
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    char *newscript = NULL;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
    struct verifymsg_proc_data *vpd = closure;
    const char *srepos = Short_Repository (repository);

#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    if (!strchr (script, '%'))
    {
	error (0, 0,
	       "warning: verifymsg line doesn't contain any format strings:\n"
               "    \"%s\"\n"
               "Appending default format string (\" %%l\"), but be aware that this usage is\n"
               "deprecated.", script);
	script = newscript = Xasprintf ("%s %%l", script);
    }
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */

    /* If we don't already have one, open a temporary file, write the message
     * to the temp file, and close the file.
     *
     * We do this here so that we only create the file when there is a
     * verifymsg script specified and we only create it once when there is
     * more than one verifymsg script specified.
     */
    if (vpd->fname == NULL)
    {
	FILE *fp;
	if ((fp = cvs_temp_file (&(vpd->fname))) == NULL)
	    error (1, errno, "cannot create temporary file %s", vpd->fname);

	if (vpd->message != NULL)
	    fputs (vpd->message, fp);
	if (vpd->message == NULL ||
	    (vpd->message)[0] == '\0' ||
	    (vpd->message)[strlen (vpd->message) - 1] != '\n')
	    putc ('\n', fp);
	if (fclose (fp) == EOF)
	    error (1, errno, "%s", vpd->fname);

	if (config->RereadLogAfterVerify == LOGMSG_REREAD_STAT)
	{
	    /* Remember the status of the temp file for later */
	    if (stat (vpd->fname, &(vpd->pre_stbuf)) != 0)
		error (1, errno, "cannot stat temp file %s", vpd->fname);

	    /*
	     * See if we need to sleep before running the verification
	     * script to avoid time-stamp races.
	     */
	    sleep_past (vpd->pre_stbuf.st_mtime);
	}
    } /* if (vpd->fname == NULL) */

    /*
     * Cast any NULL arguments as appropriate pointers as this is an
     * stdarg function and we need to be certain the caller gets what
     * is expected.
     */
    verifymsg_script = format_cmdline (
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
                                       false, srepos,
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
                                       script,
				       "c", "s", cvs_cmd_name,
#ifdef SERVER_SUPPORT
				       "R", "s", referrer
				       ? referrer->original : "NONE",
#endif /* SERVER_SUPPORT */
                                       "p", "s", srepos,
                                       "r", "s",
                                       current_parsed_root->directory,
                                       "l", "s", vpd->fname,
				       "sV", ",", vpd->changes,
				       logmsg_list_to_args_proc, (void *) NULL,
				       (char *) NULL);

#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    if (newscript) free (newscript);
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */

    if (!verifymsg_script || !strlen (verifymsg_script))
    {
	if (verifymsg_script) free (verifymsg_script);
	verifymsg_script = NULL;
	error (0, 0, "verifymsg proc resolved to the empty string!");
	return 1;
    }

    run_setup (verifymsg_script);

    free (verifymsg_script);

    /* FIXME - because run_exec can return negative values and Parse_Info adds
     * the values of each call to this function to get a total error, we are
     * calling abs on the value of run_exec to ensure two errors do not sum to
     * zero.
     *
     * The only REALLY obnoxious thing about this, I guess, is that a -1 return
     * code from run_exec can mean we failed to call the process for some
     * reason and should care about errno or that the process we called
     * returned -1 and the value of errno is undefined.  In other words,
     * run_exec should probably be rewritten to have two return codes.  one
     * which is its own exit status and one which is the child process's.  So
     * there.  :P
     *
     * Once run_exec is returning two error codes, we should probably be
     * failing here with an error message including errno when we get the
     * return code which means we care about errno, in case you missed that
     * little tidbit.
     *
     * I do happen to know we just fail for a non-zero value anyway and I
     * believe the docs actually state that if the verifymsg_proc returns a
     * "non-zero" value we will fail.
     */
    return abs (run_exec (RUN_TTY, RUN_TTY, RUN_TTY,
			  RUN_NORMAL | RUN_SIGIGNORE));
}
