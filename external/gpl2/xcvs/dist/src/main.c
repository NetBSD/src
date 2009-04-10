/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 *
 * You may distribute under the terms of the GNU General Public License
 * as specified in the README file that comes with the CVS source distribution.
 *
 * This is the main C driver for the CVS system.
 *
 * Credit to Dick Grune, Vrije Universiteit, Amsterdam, for writing
 * the shell-script CVS system that this is based on.
 *
 */

#include "cvs.h"

#include "closeout.h"
#include "setenv.h"
#include "strftime.h"
#include "xgethostname.h"

const char *program_name;
const char *program_path;
const char *cvs_cmd_name;

const char *global_session_id; /* Random session ID */

char *hostname;
/* FIXME: Perhaps this should be renamed original_hostname or the like?  */
char *server_hostname;

int use_editor = 1;
int use_cvsrc = 1;
int cvswrite = !CVSREAD_DFLT;
int really_quiet = 0;
int quiet = 0;
int trace = 0;
int noexec = 0;
int nolock = 0;
int readonlyfs = 0;
int logoff = 0;
const char *cvsDir = "CVS";



/***
 ***
 ***   CVSROOT/config options
 ***
 ***/
struct config *config;



mode_t cvsumask = UMASK_DFLT;

char *CurDir;

/*
 * Defaults, for the environment variables that are not set
 */
char *Editor = EDITOR_DFLT;



/* Temp dir stuff.  */

/* Temp dir, if set by the user.  */
static char *tmpdir_cmdline;



/* Returns in order of precedence:
 *
 *	1.  Temp dir as set via the command line.
 *	2.  Temp dir as set in CVSROOT/config.
 *	3.  Temp dir as set in $TMPDIR env var.
 *	4.  Contents of TMPDIR_DFLT preprocessor macro.
 *
 * ERRORS
 *  It is a fatal error if this function would otherwise return NULL or an
 *  empty string.
 */
const char *
get_cvs_tmp_dir (void)
{
    const char *retval;
    if (tmpdir_cmdline) retval = tmpdir_cmdline;
    else if (config && config->TmpDir) retval = config->TmpDir;
    else retval = get_system_temp_dir ();
    if (!retval) retval = TMPDIR_DFLT;

    if (!retval || !*retval) error (1, 0, "No temp dir specified.");

    return retval;
}



/* When our working directory contains subdirectories with different
   values in CVS/Root files, we maintain a list of them.  */
List *root_directories = NULL;

static const struct cmd
{
    const char *fullname;	/* Full name of the function (e.g. "commit") */

    /* Synonyms for the command, nick1 and nick2.  We supply them
       mostly for two reasons: (1) CVS has always supported them, and
       we need to maintain compatibility, (2) if there is a need for a
       version which is shorter than the fullname, for ease in typing.
       Synonyms have the disadvantage that people will see "new" and
       then have to think about it, or look it up, to realize that is
       the operation they know as "add".  Also, this means that one
       cannot create a command "cvs new" with a different meaning.  So
       new synonyms are probably best used sparingly, and where used
       should be abbreviations of the fullname (preferably consisting
       of the first 2 or 3 or so letters).

       One thing that some systems do is to recognize any unique
       abbreviation, for example "annotat" "annota", etc., for
       "annotate".  The problem with this is that scripts and user
       habits will expect a certain abbreviation to be unique, and in
       a future release of CVS it may not be.  So it is better to
       accept only an explicit list of abbreviations and plan on
       supporting them in the future as well as now.  */

    const char *nick1;
    const char *nick2;
    
    int (*func) (int, char **);	/* Function takes (argc, argv) arguments. */
    unsigned long attr;		/* Attributes. */
} cmds[] =

{
    { "add",      "ad",       "new",       add,       CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "admin",    "adm",      "rcs",       admin,     CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "annotate", "ann",      NULL,        annotate,  CVS_CMD_USES_WORK_DIR },
    { "checkout", "co",       "get",       checkout,  0 },
    { "commit",   "ci",       "com",       commit,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "diff",     "di",       "dif",       diff,      CVS_CMD_USES_WORK_DIR },
    { "edit",     NULL,       NULL,        edit,      CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "editors",  NULL,       NULL,        editors,   CVS_CMD_USES_WORK_DIR },
    { "export",   "exp",      "ex",        checkout,  CVS_CMD_USES_WORK_DIR },
    { "history",  "hi",       "his",       history,   CVS_CMD_USES_WORK_DIR },
    { "import",   "im",       "imp",       import,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR | CVS_CMD_IGNORE_ADMROOT},
    { "init",     NULL,       NULL,        init,      CVS_CMD_MODIFIES_REPOSITORY },
#if defined (HAVE_KERBEROS) && defined (SERVER_SUPPORT)
    { "kserver",  NULL,       NULL,        server,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR }, /* placeholder */
#endif
    { "log",      "lo",       NULL,        cvslog,    CVS_CMD_USES_WORK_DIR },
#ifdef AUTH_CLIENT_SUPPORT
    { "login",    "logon",    "lgn",       login,     0 },
    { "logout",   NULL,       NULL,        logout,    0 },
#endif /* AUTH_CLIENT_SUPPORT */
    { "ls",       "dir",      "list",      ls,        0 },
#if (defined(AUTH_SERVER_SUPPORT) || defined (HAVE_GSSAPI)) && defined(SERVER_SUPPORT)
    { "pserver",  NULL,       NULL,        server,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR }, /* placeholder */
#endif
    { "rannotate","rann",     "ra",        annotate,  0 },
    { "rdiff",    "patch",    "pa",        patch,     0 },
    { "release",  "re",       "rel",       release,   CVS_CMD_MODIFIES_REPOSITORY },
    { "remove",   "rm",       "delete",    cvsremove, CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "rlog",     "rl",       NULL,        cvslog,    0 },
    { "rls",      "rdir",     "rlist",     ls,        0 },
    { "rtag",     "rt",       "rfreeze",   cvstag,    CVS_CMD_MODIFIES_REPOSITORY },
#ifdef SERVER_SUPPORT
    { "server",   NULL,       NULL,        server,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
#endif
    { "status",   "st",       "stat",      cvsstatus, CVS_CMD_USES_WORK_DIR },
    { "tag",      "ta",       "freeze",    cvstag,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "unedit",   NULL,       NULL,        unedit,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "update",   "up",       "upd",       update,    CVS_CMD_USES_WORK_DIR },
    { "version",  "ve",       "ver",       version,   0 },
    { "watch",    NULL,       NULL,        watch,     CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "watchers", NULL,       NULL,        watchers,  CVS_CMD_USES_WORK_DIR },
    { NULL, NULL, NULL, NULL, 0 },
};

static const char *const usg[] =
{
    /* CVS usage messages never have followed the GNU convention of
       putting metavariables in uppercase.  I don't know whether that
       is a good convention or not, but if it changes it would have to
       change in all the usage messages.  For now, they consistently
       use lowercase, as far as I know.  Punctuation is pretty funky,
       though.  Sometimes they use none, as here.  Sometimes they use
       single quotes (not the TeX-ish `' stuff), as in --help-options.
       Sometimes they use double quotes, as in cvs -H add.

       Most (not all) of the usage messages seem to have periods at
       the end of each line.  I haven't tried to duplicate this style
       in --help as it is a rather different format from the rest.  */

    "Usage: %s [cvs-options] command [command-options-and-arguments]\n",
    "  where cvs-options are -q, -n, etc.\n",
    "    (specify --help-options for a list of options)\n",
    "  where command is add, admin, etc.\n",
    "    (specify --help-commands for a list of commands\n",
    "     or --help-synonyms for a list of command synonyms)\n",
    "  where command-options-and-arguments depend on the specific command\n",
    "    (specify -H followed by a command name for command-specific help)\n",
    "  Specify --help to receive this message\n",
    "\n",

    /* Some people think that a bug-reporting address should go here.  IMHO,
       the web sites are better because anything else is very likely to go
       obsolete in the years between a release and when someone might be
       reading this help.  Besides, we could never adequately discuss
       bug reporting in a concise enough way to put in a help message.  */

    /* I was going to put this at the top, but usage() wants the %s to
       be in the first line.  */
    "The Concurrent Versions System (CVS) is a tool for version control.\n",
    /* I really don't think I want to try to define "version control"
       in one line.  I'm not sure one can get more concise than the
       paragraph in ../cvs.spec without assuming the reader knows what
       version control means.  */

    "For CVS updates and additional information, see\n",
    "    the CVS home page at http://www.nongnu.org/cvs/ or\n",
    "    the CVSNT home page at http://www.cvsnt.org/\n",
    NULL,
};

static const char *const cmd_usage[] =
{
    "CVS commands are:\n",
    "        add          Add a new file/directory to the repository\n",
    "        admin        Administration front end for rcs\n",
    "        annotate     Show last revision where each line was modified\n",
    "        checkout     Checkout sources for editing\n",
    "        commit       Check files into the repository\n",
    "        diff         Show differences between revisions\n",
    "        edit         Get ready to edit a watched file\n",
    "        editors      See who is editing a watched file\n",
    "        export       Export sources from CVS, similar to checkout\n",
    "        history      Show repository access history\n",
    "        import       Import sources into CVS, using vendor branches\n",
    "        init         Create a CVS repository if it doesn't exist\n",
#if defined (HAVE_KERBEROS) && defined (SERVER_SUPPORT)
    "        kserver      Kerberos server mode\n",
#endif
    "        log          Print out history information for files\n",
#ifdef AUTH_CLIENT_SUPPORT
    "        login        Prompt for password for authenticating server\n",
    "        logout       Removes entry in .cvspass for remote repository\n",
#endif /* AUTH_CLIENT_SUPPORT */
    "        ls           List files available from CVS\n",
#if (defined(AUTH_SERVER_SUPPORT) || defined (HAVE_GSSAPI)) && defined(SERVER_SUPPORT)
    "        pserver      Password server mode\n",
#endif
    "        rannotate    Show last revision where each line of module was modified\n",
    "        rdiff        Create 'patch' format diffs between releases\n",
    "        release      Indicate that a Module is no longer in use\n",
    "        remove       Remove an entry from the repository\n",
    "        rlog         Print out history information for a module\n",
    "        rls          List files in a module\n",
    "        rtag         Add a symbolic tag to a module\n",
#ifdef SERVER_SUPPORT
    "        server       Server mode\n",
#endif
    "        status       Display status information on checked out files\n",
    "        tag          Add a symbolic tag to checked out version of files\n",
    "        unedit       Undo an edit command\n",
    "        update       Bring work tree in sync with repository\n",
    "        version      Show current CVS version(s)\n",
    "        watch        Set watches\n",
    "        watchers     See who is watching a file\n",
    "(Specify the --help option for a list of other help options)\n",
    NULL,
};

static const char *const opt_usage[] =
{
    /* Omit -b because it is just for compatibility.  */
    "CVS global options (specified before the command name) are:\n",
    "    -H           Displays usage information for command.\n",
    "    -Q           Cause CVS to be really quiet.\n",
    "    -q           Cause CVS to be somewhat quiet.\n",
    "    -r           Make checked-out files read-only.\n",
    "    -w           Make checked-out files read-write (default).\n",
    "    -n           Do not execute anything that will change the disk.\n",
    "    -u           Don't create locks (implies -l).\n",
    "    -t           Show trace of program execution (repeat for more\n",
    "                 verbosity) -- try with -n.\n",
    "    -R           Assume repository is read-only, such as CDROM\n",
    "    -v           CVS version and copyright.\n",
    "    -T tmpdir    Use 'tmpdir' for temporary files.\n",
    "    -e editor    Use 'editor' for editing log information.\n",
    "    -d CVS_root  Overrides $CVSROOT as the root of the CVS tree.\n",
    "    -D dir       Use DIR as the bookkeeping directory instead of CVS.\n"
    "    -f           Do not use the ~/.cvsrc file.\n",
#ifdef CLIENT_SUPPORT
    "    -z #         Request compression level '#' for net traffic.\n",
#ifdef ENCRYPTION
    "    -x           Encrypt all net traffic.\n",
#endif
    "    -a           Authenticate all net traffic.\n",
#endif
    "    -s VAR=VAL   Set CVS user variable.\n",
    "(Specify the --help option for a list of other help options)\n",
    NULL
};


static int
set_root_directory (Node *p, void *ignored)
{
    if (current_parsed_root == NULL && p->data != NULL)
    {
	current_parsed_root = p->data;
	original_parsed_root = current_parsed_root;
	return 1;
    }
    return 0;
}


static const char * const*
cmd_synonyms (void)
{
    char ** synonyms;
    char ** line;
    const struct cmd *c = &cmds[0];
    /* Three more for title, "specify --help" line, and NULL.  */
    int numcmds = 3;

    while (c->fullname != NULL)
    {
	numcmds++;
	c++;
    }
    
    synonyms = xnmalloc (numcmds, sizeof(char *));
    line = synonyms;
    *line++ = "CVS command synonyms are:\n";
    for (c = &cmds[0]; c->fullname != NULL; c++)
    {
	if (c->nick1 || c->nick2)
	{
	    *line = Xasprintf ("        %-12s %s %s\n", c->fullname,
			       c->nick1 ? c->nick1 : "",
			       c->nick2 ? c->nick2 : "");
	    line++;
	}
    }
    *line++ = "(Specify the --help option for a list of other help options)\n";
    *line = NULL;
    
    return (const char * const*) synonyms; /* will never be freed */
}



unsigned long int
lookup_command_attribute (const char *cmd_name)
{
    const struct cmd *cm;

    for (cm = cmds; cm->fullname; cm++)
    {
	if (strcmp (cmd_name, cm->fullname) == 0)
	    break;
    }
    if (!cm->fullname)
	error (1, 0, "unknown command: %s", cmd_name);
    return cm->attr;
}



/*
 * Exit with an error code and an informative message about the signal
 * received.  This function, by virtue of causing an actual call to exit(),
 * causes all the atexit() handlers to be called.
 *
 * INPUTS
 *   sig	The signal recieved.
 *
 * ERRORS
 *   The cleanup routines registered via atexit() and the error function
 *   itself can potentially change the exit status.  They shouldn't do this
 *   unless they encounter problems doing their own jobs.
 *
 * RETURNS
 *   Nothing.  This function will always exit.  It should exit with an exit
 *   status of 1, but might not, as noted in the ERRORS section above.
 */
#ifndef DONT_USE_SIGNALS
static RETSIGTYPE main_cleanup (int) __attribute__ ((__noreturn__));
#endif /* DONT_USE_SIGNALS */
static RETSIGTYPE
main_cleanup (int sig)
{
#ifndef DONT_USE_SIGNALS
    const char *name;
    char temp[10];
    static int reenter = 0;

    if (reenter++)
	_exit(1);

    switch (sig)
    {
#ifdef SIGABRT
    case SIGABRT:
	name = "abort";
	break;
#endif
#ifdef SIGHUP
    case SIGHUP:
	name = "hangup";
	break;
#endif
#ifdef SIGINT
    case SIGINT:
	name = "interrupt";
	break;
#endif
#ifdef SIGQUIT
    case SIGQUIT:
	name = "quit";
	break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
	name = "broken pipe";
	break;
#endif
#ifdef SIGTERM
    case SIGTERM:
	name = "termination";
	break;
#endif
    default:
	/* This case should never be reached, because we list above all
	   the signals for which we actually establish a signal handler.  */
	sprintf (temp, "%d", sig);
	name = temp;
	break;
    }

    /* This always exits, which will cause our exit handlers to be called.  */
    error (1, 0, "received %s signal", name);
    /* but make the exit explicit to silence warnings when gcc processes the
     * noreturn attribute.
     */
    exit (EXIT_FAILURE);
#endif /* !DONT_USE_SIGNALS */
}



/* From server.c.
 *
 * When !defined ALLOW_CONFIG_OVERRIDE, this will never have any value but
 * NULL.
 */
extern char *gConfigPath;




enum {RANDOM_BYTES = 8};
enum {COMMITID_RAW_SIZE = (sizeof(time_t) + RANDOM_BYTES)};

static char const alphabet[62] =
  "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Divide BUF by D, returning the remainder.  Replace BUF by the
   quotient.  BUF[0] is the most significant part of BUF.
   D must not exceed UINT_MAX >> CHAR_BIT.  */
static unsigned int
divide_by (unsigned char buf[COMMITID_RAW_SIZE], unsigned int d)
{
    unsigned int carry = 0;
    int i;
    for (i = 0; i < COMMITID_RAW_SIZE; i++)
    {
	unsigned int byte = buf[i];
	unsigned int dividend = (carry << CHAR_BIT) + byte;
	buf[i] = dividend / d;
	carry = dividend % d;
    }
    return carry;
}

static void
convert (char const input[COMMITID_RAW_SIZE], char *output)
{
    static char const zero[COMMITID_RAW_SIZE] = { 0, };
    unsigned char buf[COMMITID_RAW_SIZE];
    size_t o = 0;
    memcpy (buf, input, COMMITID_RAW_SIZE);
    while (memcmp (buf, zero, COMMITID_RAW_SIZE) != 0)
	output[o++] = alphabet[divide_by (buf, sizeof alphabet)];
    if (! o)
	output[o++] = '0';
    output[o] = '\0';
}


int
main (int argc, char **argv)
{
    cvsroot_t *CVSroot_parsed = NULL;
    bool cvsroot_update_env = true;
    char *cp, *end;
    const struct cmd *cm;
    int c, err = 0;
    int free_Editor = 0;

    int help = 0;		/* Has the user asked for help?  This
				   lets us support the `cvs -H cmd'
				   convention to give help for cmd. */
    static const char short_options[] = "+QqrwtlnRvb:T:e:d:D:Hfz:s:xa";
    static struct option long_options[] =
    {
        {"help", 0, NULL, 'H'},
        {"version", 0, NULL, 'v'},
	{"help-commands", 0, NULL, 1},
	{"help-synonyms", 0, NULL, 2},
	{"help-options", 0, NULL, 4},
#ifdef SERVER_SUPPORT
	{"allow-root", required_argument, NULL, 3},
#endif /* SERVER_SUPPORT */
        {0, 0, 0, 0}
    };
    /* `getopt_long' stores the option index here, but right now we
        don't use it. */
    int option_index = 0;

#ifdef SYSTEM_INITIALIZE
    /* Hook for OS-specific behavior, for example socket subsystems on
       NT and OS2 or dealing with windows and arguments on Mac.  */
    SYSTEM_INITIALIZE (&argc, &argv);
#endif

#ifdef SYSTEM_CLEANUP
	/* Hook for OS-specific behavior, for example socket subsystems on
	   NT and OS2 or dealing with windows and arguments on Mac.  */
	cleanup_register (SYSTEM_CLEANUP);
#endif

#ifdef HAVE_TZSET
    /* On systems that have tzset (which is almost all the ones I know
       of), it's a good idea to call it.  */
    tzset ();
#endif

    /*
     * Just save the last component of the path for error messages
     */
    program_path = xstrdup (argv[0]);
#ifdef ARGV0_NOT_PROGRAM_NAME
    /* On some systems, e.g. VMS, argv[0] is not the name of the command
       which the user types to invoke the program.  */
    program_name = "cvs";
#else
    program_name = last_component (argv[0]);
#endif

    /*
     * Query the environment variables up-front, so that
     * they can be overridden by command line arguments
     */
    if ((cp = getenv (EDITOR1_ENV)) != NULL)
 	Editor = cp;
    else if ((cp = getenv (EDITOR2_ENV)) != NULL)
	Editor = cp;
    else if ((cp = getenv (EDITOR3_ENV)) != NULL)
	Editor = cp;
    if (getenv (CVSREAD_ENV) != NULL)
	cvswrite = 0;
    if (getenv (CVSREADONLYFS_ENV) != NULL) {
	readonlyfs = 1;
	logoff = 1;
    }

    /* Set this to 0 to force getopt initialization.  getopt() sets
       this to 1 internally.  */
    getoptreset ();

    /* We have to parse the options twice because else there is no
       chance to avoid reading the global options from ".cvsrc".  Set
       opterr to 0 for avoiding error messages about invalid options.
       */
    opterr = 0;

    while ((c = getopt_long
            (argc, argv, short_options, long_options, &option_index))
           != EOF)
    {
	if (c == 'f')
	    use_cvsrc = 0;
    }

#ifdef SERVER_SUPPORT
    /* Don't try and read a .cvsrc file if we are a server.  */
    if (optind < argc
	&& (false
# if defined (AUTH_SERVER_SUPPORT) || defined (HAVE_GSSAPI)
	    || !strcmp (argv[optind], "pserver")
# endif
# ifdef HAVE_KERBEROS
	    || !strcmp (argv[optind], "kserver")
# endif /* HAVE_KERBEROS */
	    || !strcmp (argv[optind], "server")))
	{
	    /* Avoid any .cvsrc file.  */
	    use_cvsrc = 0;
	    /* Pre-parse the server options to get the config path.  */
	    cvs_cmd_name = argv[optind];
	    parseServerOptions (argc - optind, argv + optind);
	}
#endif /* SERVER_SUPPORT */

    /*
     * Scan cvsrc file for global options.
     */
    if (use_cvsrc)
	read_cvsrc (&argc, &argv, "cvs");

    getoptreset();
    while ((c = getopt_long
            (argc, argv, short_options, long_options, &option_index))
           != EOF)
    {
	switch (c)
	{
            case 1:
	        /* --help-commands */
                usage (cmd_usage);
                break;
            case 2:
	        /* --help-synonyms */
                usage (cmd_synonyms());
                break;
	    case 4:
		/* --help-options */
		usage (opt_usage);
		break;
#ifdef SERVER_SUPPORT
	    case 3:
		/* --allow-root */
		root_allow_add (optarg, gConfigPath);
		break;
#endif /* SERVER_SUPPORT */
	    case 'Q':
		really_quiet = 1;
		/* FALL THROUGH */
	    case 'q':
		quiet = 1;
		break;
	    case 'r':
		cvswrite = 0;
		break;
	    case 'w':
		cvswrite = 1;
		break;
	    case 't':
		trace++;
		break;
	    case 'R':
		readonlyfs = -1;
		logoff = 1;
		break;
	    case 'n':
		noexec = 1;
	    case 'u':			/* Fall through */
		nolock = 1;
	    case 'l':			/* Fall through */
		logoff = 1;
		break;
	    case 'v':
		(void) fputs ("\n", stdout);
		version (0, NULL);    
		(void) fputs ("\n", stdout);
		(void) fputs ("\
Copyright (C) 2005 Free Software Foundation, Inc.\n\
\n\
Senior active maintainers include Larry Jones, Derek R. Price,\n\
and Mark D. Baushke.  Please see the AUTHORS and README files from the CVS\n\
distribution kit for a complete list of contributors and copyrights.\n",
		              stdout);
		(void) fputs ("\n", stdout);
		(void) fputs ("CVS may be copied only under the terms of the GNU General Public License,\n", stdout);
		(void) fputs ("a copy of which can be found with the CVS distribution kit.\n", stdout);
		(void) fputs ("\n", stdout);

		(void) fputs ("Specify the --help option for further information about CVS\n", stdout);

		exit (0);
		break;
	    case 'b':
		/* This option used to specify the directory for RCS
		   executables.  But since we don't run them any more,
		   this is a noop.  Silently ignore it so that .cvsrc
		   and scripts and inetd.conf and such can work with
		   either new or old CVS.  */
		break;
	    case 'T':
		if (tmpdir_cmdline) free (tmpdir_cmdline);
		tmpdir_cmdline = xstrdup (optarg);
		break;
	    case 'e':
		if (free_Editor) free (Editor);
		Editor = xstrdup (optarg);
		free_Editor = 1;
		break;
	    case 'd':
		if (CVSroot_cmdline != NULL)
		    free (CVSroot_cmdline);
		CVSroot_cmdline = xstrdup (optarg);
		break;
	    case 'H':
	        help = 1;
		break;
            case 'f':
		use_cvsrc = 0; /* unnecessary, since we've done it above */
		break;
	    case 'z':
#ifdef CLIENT_SUPPORT
		gzip_level = strtol (optarg, &end, 10);
		if (*end != '\0' || gzip_level < 0 || gzip_level > 9)
		  error (1, 0,
			 "gzip compression level must be between 0 and 9");
#endif /* CLIENT_SUPPORT */
		/* If no CLIENT_SUPPORT, we just silently ignore the gzip
		 * level, so that users can have it in their .cvsrc and not
		 * cause any trouble.
		 *
		 * We still parse the argument to -z for correctness since
		 * one user complained of being bitten by a run of
		 * `cvs -z -n up' which read -n as the argument to -z without
		 * complaining.  */
		break;
	    case 's':
		variable_set (optarg);
		break;
	    case 'x':
#ifdef CLIENT_SUPPORT
	        cvsencrypt = 1;
#endif /* CLIENT_SUPPORT */
		/* If no CLIENT_SUPPORT, ignore -x, so that users can
                   have it in their .cvsrc and not cause any trouble.
                   If no ENCRYPTION, we still accept -x, but issue an
                   error if we are being run as a client.  */
		break;
	    case 'a':
#ifdef CLIENT_SUPPORT
		cvsauthenticate = 1;
#endif
		/* If no CLIENT_SUPPORT, ignore -a, so that users can
                   have it in their .cvsrc and not cause any trouble.
                   We will issue an error later if stream
                   authentication is not supported.  */
		break;
	    case 'D':
		cvsDir = xstrdup (optarg);
		if (strchr (cvsDir, '/') != NULL)
		    error (1, 0, "cvsDir is not allowed to have slashes");
		break;
	    case '?':
	    default:
                usage (usg);
	}
    }

    argc -= optind;
    argv += optind;
    if (argc < 1)
	usage (usg);

    if (readonlyfs && !really_quiet) {
	error (0, 0,
	       "WARNING: Read-only repository access mode selected via `cvs -R'.\n\
Using this option to access a repository which some users write to may\n\
cause intermittent sandbox corruption.");
    }

    /* Calculate the cvs global session ID */

    {
	char buf[COMMITID_RAW_SIZE] = { 0, };
	char out[COMMITID_RAW_SIZE * 2];
	ssize_t len = 0;
	time_t rightnow = time (NULL);
	char *startrand = buf + sizeof (time_t);
	unsigned char *p = (unsigned char *) startrand;
	size_t randbytes = RANDOM_BYTES;
	int flags = O_RDONLY;
	int fd;
#ifdef O_NOCTTY
	flags |= O_NOCTTY;
#endif
	if (rightnow != (time_t)-1)
		while (rightnow > 0) {
		    *--p = rightnow % (UCHAR_MAX + 1);
		    rightnow /= UCHAR_MAX + 1;
		}
	else {
	    /* try to use more random data */
	    randbytes = COMMITID_RAW_SIZE;
	    startrand = buf;
	}
	fd = open ("/dev/urandom", flags);
	if (fd >= 0) {
	    len = read (fd, startrand, randbytes);
	    close (fd);
	}
	if (len <= 0) {
	    /* no random data was available so use pid */
	    long int pid = (long int)getpid ();
	    p = (unsigned char *) (startrand + sizeof (pid));
	    while (pid > 0) {
		*--p = pid % (UCHAR_MAX + 1);
		pid /= UCHAR_MAX + 1;
	    }
	}
	convert(buf, out);
	global_session_id = strdup (out);
    }


    TRACE (TRACE_FUNCTION, "main: Session ID is %s", global_session_id);

    /* Look up the command name. */

    cvs_cmd_name = argv[0];
    for (cm = cmds; cm->fullname; cm++)
    {
	if (cm->nick1 && !strcmp (cvs_cmd_name, cm->nick1))
	    break;
	if (cm->nick2 && !strcmp (cvs_cmd_name, cm->nick2))
	    break;
	if (!strcmp (cvs_cmd_name, cm->fullname))
	    break;
    }

    if (!cm->fullname)
    {
	fprintf (stderr, "Unknown command: `%s'\n\n", cvs_cmd_name);
	usage (cmd_usage);
    }
    else
	cvs_cmd_name = cm->fullname;	/* Global pointer for later use */

    if (help)
    {
	argc = -1;		/* some functions only check for this */
	err = (*(cm->func)) (argc, argv);
    }
    else
    {
	/* The user didn't ask for help, so go ahead and authenticate,
           set up CVSROOT, and the rest of it. */

	short int lock_cleanup_setup = 0;

	/* The UMASK environment variable isn't handled with the
	   others above, since we don't want to signal errors if the
	   user has asked for help.  This won't work if somebody adds
	   a command-line flag to set the umask, since we'll have to
	   parse it before we get here. */

	if ((cp = getenv (CVSUMASK_ENV)) != NULL)
	{
	    /* FIXME: Should be accepting symbolic as well as numeric mask.  */
	    cvsumask = strtol (cp, &end, 8) & 0777;
	    if (*end != '\0')
		error (1, errno, "invalid umask value in %s (%s)",
		       CVSUMASK_ENV, cp);
	}

	/* HOSTNAME & SERVER_HOSTNAME need to be set before they are
	 * potentially used in gserver_authenticate_connection() (called from
	 * pserver_authenticate_connection, below).
	 */
	hostname = xgethostname ();
	if (!hostname)
	{
            error (0, errno,
                   "xgethostname () returned NULL, using \"localhost\"");
            hostname = xstrdup ("localhost");
	}

	/* Keep track of this separately since the client can change
	 * HOSTNAME on the server.
	 */
	server_hostname = xstrdup (hostname);

#ifdef SERVER_SUPPORT

# ifdef HAVE_KERBEROS
	/* If we are invoked with a single argument "kserver", then we are
	   running as Kerberos server as root.  Do the authentication as
	   the very first thing, to minimize the amount of time we are
	   running as root.  */
	if (strcmp (cvs_cmd_name, "kserver") == 0)
	{
	    kserver_authenticate_connection ();

	    /* Pretend we were invoked as a plain server.  */
	    cvs_cmd_name = "server";
	}
# endif /* HAVE_KERBEROS */

# if defined (AUTH_SERVER_SUPPORT) || defined (HAVE_GSSAPI)
	if (strcmp (cvs_cmd_name, "pserver") == 0)
	{
	    /* The reason that --allow-root is not a command option
	       is mainly that it seems easier to make it a global option.  */

	    /* Gets username and password from client, authenticates, then
	       switches to run as that user and sends an ACK back to the
	       client. */
	    pserver_authenticate_connection ();
      
	    /* Pretend we were invoked as a plain server.  */
	    cvs_cmd_name = "server";
	}
# endif /* AUTH_SERVER_SUPPORT || HAVE_GSSAPI */
#endif /* SERVER_SUPPORT */

	server_active = strcmp (cvs_cmd_name, "server") == 0;

#ifdef SERVER_SUPPORT
	if (server_active)
	{
	    /* This is only used for writing into the history file.  For
	       remote connections, it might be nice to have hostname
	       and/or remote path, on the other hand I'm not sure whether
	       it is worth the trouble.  */
	    CurDir = xstrdup ("<remote>");
	    cleanup_register (server_cleanup);
	}
	else
#endif
	{
	    cleanup_register (close_stdout);
	    CurDir = xgetcwd ();
            if (CurDir == NULL)
		error (1, errno, "cannot get working directory");
	}

	{
	    char *val;
	    /* XXX pid < 10^32 */
	    val = Xasprintf ("%ld", (long) getpid ());
	    setenv (CVS_PID_ENV, val, 1);
	    free (val);
	}

	/* make sure we clean up on error */
	signals_register (main_cleanup);

#ifdef KLUDGE_FOR_WNT_TESTSUITE
	/* Probably the need for this will go away at some point once
	   we call fflush enough places (e.g. fflush (stdout) in
	   cvs_outerr).  */
	(void) setvbuf (stdout, NULL, _IONBF, 0);
	(void) setvbuf (stderr, NULL, _IONBF, 0);
#endif /* KLUDGE_FOR_WNT_TESTSUITE */

	if (use_cvsrc)
	    read_cvsrc (&argc, &argv, cvs_cmd_name);

	/* Fiddling with CVSROOT doesn't make sense if we're running
	 * in server mode, since the client will send the repository
	 * directory after the connection is made.
	 */
	if (!server_active)
	{
	    /* First check if a root was set via the command line.  */
	    if (CVSroot_cmdline)
	    {
		 if (!(CVSroot_parsed = parse_cvsroot (CVSroot_cmdline)))
		     error (1, 0, "Bad CVSROOT: `%s'.", CVSroot_cmdline);
	    }

	    /* See if we are able to find a 'better' value for CVSroot
	     * in the CVSADM_ROOT directory.
	     *
	     * "cvs import" shouldn't check CVS/Root; in general it
	     * ignores CVS directories and CVS/Root is likely to
	     * specify a different repository than the one we are
	     * importing to, but if this is not import and no root was
	     * specified on the command line, set the root from the
	     * CVS/Root file.
	     */
	    if (!CVSroot_parsed
		&& !(cm->attr & CVS_CMD_IGNORE_ADMROOT)
	       )
		CVSroot_parsed = Name_Root (NULL, NULL);

	    /* Now, if there is no root on the command line and we didn't find
	     * one in a file, set it via the $CVSROOT env var.
	     */
	    if (!CVSroot_parsed)
	    {
		char *tmp = getenv (CVSROOT_ENV);
		if (tmp)
		{
		    if (!(CVSroot_parsed = parse_cvsroot (tmp)))
			error (1, 0, "Bad CVSROOT: `%s'.", tmp);
		    cvsroot_update_env = false;
		}
	    }

#ifdef CVSROOT_DFLT
	    if (!CVSroot_parsed)
	    {
		if (!(CVSroot_parsed = parse_cvsroot (CVSROOT_DFLT)))
		    error (1, 0, "Bad CVSROOT: `%s'.", CVSROOT_DFLT);
	    }
#endif /* CVSROOT_DFLT */

	    /* Now we've reconciled CVSROOT from the command line, the
	       CVS/Root file, and the environment variable.  Do the
	       last sanity checks on the variable. */
	    if (!CVSroot_parsed)
	    {
		error (0, 0,
		       "No CVSROOT specified!  Please use the `-d' option");
		error (1, 0,
		       "or set the %s environment variable.", CVSROOT_ENV);
	    }
	}

	/* Here begins the big loop over unique cvsroot values.  We
           need to call do_recursion once for each unique value found
           in CVS/Root.  Prime the list with the current value. */

	/* Create the list. */
	assert (root_directories == NULL);
	root_directories = getlist ();

	/* Prime it. */
	if (CVSroot_parsed)
	{
	    Node *n;
	    n = getnode ();
	    n->type = NT_UNKNOWN;
	    n->key = xstrdup (CVSroot_parsed->original);
	    n->data = CVSroot_parsed;

	    if (addnode (root_directories, n))
		error (1, 0, "cannot add initial CVSROOT %s", n->key);
	}

	assert (current_parsed_root == NULL);

	/* If we're running the server, we want to execute this main
	   loop once and only once (we won't be serving multiple roots
	   from this connection, so there's no need to do it more than
	   once).  To get out of the loop, we perform a "break" at the
	   end of things.  */

	while (server_active ||
	       walklist (root_directories, set_root_directory, NULL))
	{
	    /* Fiddling with CVSROOT doesn't make sense if we're running
	       in server mode, since the client will send the repository
	       directory after the connection is made. */

	    if (!server_active)
	    {
		/* Now we're 100% sure that we have a valid CVSROOT
		   variable.  Parse it to see if we're supposed to do
		   remote accesses or use a special access method. */

		TRACE (TRACE_FUNCTION,
		       "main loop with CVSROOT=%s",
		       current_parsed_root ? current_parsed_root->directory
					   : "(null)");

		/*
		 * Check to see if the repository exists.
		 */
		if (!current_parsed_root->isremote && !nolock)
		{
		    char *path;
		    int save_errno;

		    path = Xasprintf ("%s/%s", current_parsed_root->directory,
				      CVSROOTADM);
		    if (!isaccessible (path, R_OK | X_OK))
		    {
			save_errno = errno;
			/* If this is "cvs init", the root need not exist yet.
			 */
			if (strcmp (cvs_cmd_name, "init"))
			    error (1, save_errno, "%s", path);
		    }
		    free (path);
		}

		/* Update the CVSROOT environment variable.  */
		if (cvsroot_update_env)
		    setenv (CVSROOT_ENV, current_parsed_root->original, 1);
	    }
	
	    /* Parse the CVSROOT/config file, but only for local.  For the
	       server, we parse it after we know $CVSROOT.  For the
	       client, it doesn't get parsed at all, obviously.  The
	       presence of the parse_config call here is not meant to
	       predetermine whether CVSROOT/config overrides things from
	       read_cvsrc and other such places or vice versa.  That sort
	       of thing probably needs more thought.  */
	    if (!server_active && !current_parsed_root->isremote)
	    {
		/* If there was an error parsing the config file, parse_config
		   already printed an error.  We keep going.  Why?  Because
		   if we didn't, then there would be no way to check in a new
		   CVSROOT/config file to fix the broken one!  */
		if (config) free_config (config);
		config = parse_config (current_parsed_root->directory, NULL);

		/* Can set TMPDIR in the environment if necessary now, since
		 * if it was set in config, we now know it.
		 */
		push_env_temp_dir ();
	    }

#ifdef CLIENT_SUPPORT
	    /* Need to check for current_parsed_root != NULL here since
	     * we could still be in server mode before the server function
	     * gets called below and sets the root
	     */
	    if (current_parsed_root != NULL && current_parsed_root->isremote)
	    {
		/* Create a new list for directory names that we've
		   sent to the server. */
		if (dirs_sent_to_server != NULL)
		    dellist (&dirs_sent_to_server);
		dirs_sent_to_server = getlist ();
	    }
#endif

	    if (
#ifdef SERVER_SUPPORT
		/* Don't worry about lock_cleanup_setup when the server is
		 * active since we can only go through this loop once in that
		 * case anyhow.
		 */
		server_active ||
#endif
	        (
#ifdef CLIENT_SUPPORT
		 !current_parsed_root->isremote &&
#endif
		 !lock_cleanup_setup))
	    {
		/* Set up to clean up any locks we might create on exit.  */
		cleanup_register (Lock_Cleanup);
		lock_cleanup_setup = 1;
	    }

	    /* Call our worker function.  */
	    err = (*(cm->func)) (argc, argv);
	
	    /* Mark this root directory as done.  When the server is
               active, our list will be empty -- don't try and
               remove it from the list. */

	    if (!server_active)
	    {
		Node *n = findnode (root_directories,
				    original_parsed_root->original);
		assert (n != NULL);
		assert (n->data != NULL);
		n->data = NULL;
		current_parsed_root = NULL;
	    }

	    if (server_active)
		break;
	} /* end of loop for cvsroot values */

	dellist (&root_directories);
    } /* end of stuff that gets done if the user DOESN'T ask for help */

    root_allow_free ();

    /* This is exit rather than return because apparently that keeps
       some tools which check for memory leaks happier.  */
    exit (err ? EXIT_FAILURE : 0);
	/* Keep picky/stupid compilers (e.g. Visual C++ 5.0) happy.  */
	return 0;
}



char *
Make_Date (const char *rawdate)
{
    struct timespec t;

    if (!get_date (&t, rawdate, NULL))
	error (1, 0, "Can't parse date/time: `%s'", rawdate);

    /* Truncate nanoseconds.  */
    return date_from_time_t (t.tv_sec);
}



/* Parse a string of the form TAG[:DATE], where TAG could be the empty string.
 *
 * INPUTS
 *   input	The string to be parsed.
 *
 * OUTPUTS
 *   tag	The tag found, if any.  If TAG is the empty string, then leave
 *		this value unchanged.
 *   date	The date found, if any.  If DATE is the empty string or is
 *		missing, leave this value unchanged.
 *
 * NOTES
 *   If either TAG or DATE is replaced for output, the previous value is freed.
 *
 * ERRORS
 *   If either TAG or DATE cannot be parsed, then this function will exit with
 *   a fatal error message.
 *
 * RETURNS
 *   Nothing.
 */
void
parse_tagdate (char **tag, char **date, const char *input)
{
    char *p;

    TRACE (TRACE_FUNCTION, "parse_tagdate (%s, %s, %s)",
	   *tag ? *tag : "(null)", *date ? *date : "(null)",
	   input);

    if ((p = strchr (input, ':')))
    {
	/* Parse the tag.  */
	if (p - input)
	{
	    /* The tag has > 0 length.  */
	    if (*tag) free (*tag);
	    *tag = xmalloc (p - input + 1);
	    strncpy (*tag, input, p - input);
	    (*tag)[p - input] = '\0';
	}

	/* Parse the date.  */
	if (*++p)
	{
	    if (*date) free (*date);
	    *date = Make_Date (p);
	}
    }
    else if (strlen (input))
    {
	/* The tag has > 0 length.  */
	if (*tag) free (*tag);
	*tag = xstrdup (input);
    }

    TRACE (TRACE_DATA, "parse_tagdate: got tag = `%s', date = `%s'",
	   *tag ? *tag : "(null)", *date ? *date : "(null)");
}



/* Convert a time_t to an RCS format date.  This is mainly for the
   use of "cvs history", because the CVSROOT/history file contains
   time_t format dates; most parts of CVS will want to avoid using
   time_t's directly, and instead use RCS_datecmp, Make_Date, &c.
   Assuming that the time_t is in GMT (as it generally should be),
   then the result will be in GMT too.

   Returns a newly malloc'd string.  */

char *
date_from_time_t (time_t unixtime)
{
    struct tm *ftm;
    char date[MAXDATELEN];
    char *ret;

    ftm = gmtime (&unixtime);
    if (ftm == NULL)
	/* This is a system, like VMS, where the system clock is in local
	   time.  Hopefully using localtime here matches the "zero timezone"
	   hack I added to get_date (get_date of course being the relevant
	   issue for Make_Date, and for history.c too I think).  */
	ftm = localtime (&unixtime);

    (void) sprintf (date, DATEFORM,
		    ftm->tm_year + (ftm->tm_year < 100 ? 0 : 1900),
		    ftm->tm_mon + 1, ftm->tm_mday, ftm->tm_hour,
		    ftm->tm_min, ftm->tm_sec);
    ret = xstrdup (date);
    return ret;
}


const char *
getCVSDir (const char *suffix)
{
    static const char *buf[20][2];
    size_t i, len;

    for (i = 0; i < 20; i++) {
	if (buf[i][0] == NULL)
	    break;
	if (strcmp (buf[i][0], suffix) == 0)
	    return buf[i][1];
    }

    if (i == 20)
	error (1, 0, "Out of static buffer space");

    buf[i][0] = suffix;
    buf[i][1] = xmalloc (len = strlen(cvsDir) + strlen(suffix) + 1);
    snprintf ((char *)buf[i][1], len, "%s%s", cvsDir, suffix);
    return buf[i][1];
}



/* Convert a date to RFC822/1123 format.  This is used in contexts like
   dates to send in the protocol; it should not vary based on locale or
   other such conventions for users.  We should have another routine which
   does that kind of thing.

   The SOURCE date is in our internal RCS format.  DEST should point to
   storage managed by the caller, at least MAXDATELEN characters.  */
void
date_to_internet (char *dest, const char *source)
{
    struct tm date;

    date_to_tm (&date, source);
    tm_to_internet (dest, &date);
}



void
date_to_tm (struct tm *dest, const char *source)
{
    if (sscanf (source, SDATEFORM,
		&dest->tm_year, &dest->tm_mon, &dest->tm_mday,
		&dest->tm_hour, &dest->tm_min, &dest->tm_sec)
	    != 6)
	/* Is there a better way to handle errors here?  I made this
	   non-fatal in case we are called from the code which can't
	   deal with fatal errors.  */
	error (0, 0, "internal error: bad date %s", source);

    if (dest->tm_year > 100)
	dest->tm_year -= 1900;

    dest->tm_mon -= 1;
}



/* Convert a date to RFC822/1123 format.  This is used in contexts like
   dates to send in the protocol; it should not vary based on locale or
   other such conventions for users.  We should have another routine which
   does that kind of thing.

   The SOURCE date is a pointer to a struct tm.  DEST should point to
   storage managed by the caller, at least MAXDATELEN characters.  */
void
tm_to_internet (char *dest, const struct tm *source)
{
    /* Just to reiterate, these strings are from RFC822 and do not vary
       according to locale.  */
    static const char *const month_names[] =
      {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    sprintf (dest, "%d %s %d %02d:%02d:%02d -0000", source->tm_mday,
	     source->tm_mon < 0 || source->tm_mon > 11
               ? "???" : month_names[source->tm_mon],
	     source->tm_year + 1900, source->tm_hour, source->tm_min,
             source->tm_sec);
}



/*
 * Format a date for the current locale.
 *
 * INPUT
 *   UNIXTIME	The UNIX seconds since the epoch.
 *
 * RETURNS
 *   If my_strftime() encounters an error, this function can return NULL.
 *
 *   Otherwise, returns a date string in ISO8601 format, e.g.:
 *
 *	2004-04-29 13:24:22 -0700
 *
 *   It is the responsibility of the caller to return of this string.
 */
static char *
format_time_t (time_t unixtime)
{
    static char buf[sizeof ("yyyy-mm-dd HH:MM:SS -HHMM")];
    /* Convert to a time in the local time zone.  */
    struct tm ltm = *(localtime (&unixtime));

    if (!my_strftime (buf, sizeof (buf), "%Y-%m-%d %H:%M:%S %z", &ltm, 0, 0))
	return NULL;

    return xstrdup (buf);
}



/* Like format_time_t(), but return time in UTC.
 */
char *
gmformat_time_t (time_t unixtime)
{
    static char buf[sizeof ("yyyy-mm-dd HH:MM:SS -HHMM")];
    /* Convert to a time in the local time zone.  */
    struct tm ltm = *(gmtime (&unixtime));

    if (!my_strftime (buf, sizeof (buf), "%Y-%m-%d %H:%M:%S %z", &ltm, 0, 0))
	return NULL;

    return xstrdup (buf);
}



/* Format a date in the local timezone using format_time_t() given a date from
 * an arbitrary timezone in a string.
 *
 * INPUT
 *   DATESTR	A string that looks like anything get_date() can parse, e.g.:
 *
 *                      2004-04-29 20:24:22
 *
 * ERRORS
 *   As get_date() & format_time_t().  Prints a warning if either provide
 *   error return values.  See RETURNS.
 *
 * RETURNS
 *   A freshly allocated string that is a copy of the input string if either
 *   get_date() or format_time_t() encounter an error and as format_time_t()
 *   otherwise.
 */
char *
format_date_alloc (char *datestr)
{
    struct timespec t;
    char *buf;

    TRACE (TRACE_FUNCTION, "format_date (%s)", datestr);

    /* Convert the date string to seconds since the epoch. */
    if (!get_date (&t, datestr, NULL))
    {
	error (0, 0, "Can't parse date/time: `%s'.", datestr);
	goto as_is;
    }

    /* Get the time into a string, truncating any nanoseconds returned by
     * getdate.
     */
    if ((buf = format_time_t (t.tv_sec)) == NULL)
    {
	error (0, 0, "Unable to reformat date `%s'.", datestr);
	goto as_is;
    }

    return buf;

 as_is:
    return xstrdup (datestr);
}

void
getoptreset (void)
{
#ifdef HAVE_GETOPT_OPTRESET
	optreset = 1;
	optind = 1;
#else
	optind = 0;
#endif
	opterr = 1;
}


void
usage (register const char *const *cpp)
{
    (void) fprintf (stderr, *cpp++, program_name, cvs_cmd_name);
    for (; *cpp; cpp++)
	(void) fprintf (stderr, *cpp);
    exit (EXIT_FAILURE);
}

/* vim:tabstop=8:shiftwidth=4
 */
