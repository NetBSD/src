/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"

/* CVS */
#include "edit.h"
#include "fileattr.h"
#include "watch.h"

/* GNULIB */
#include "buffer.h"
#include "getline.h"
#include "getnline.h"

int server_active = 0;

#if defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT)

# include "log-buffer.h"
# include "ms-buffer.h"
#endif	/* defined(SERVER_SUPPORT) || defined(CLIENT_SUPPORT) */

#if defined (HAVE_GSSAPI) && defined (SERVER_SUPPORT)
# include "canon-host.h"
# include "gssapi-client.h"

/* This stuff isn't included solely with SERVER_SUPPORT since some of these
 * functions (encryption & the like) get compiled with or without server
 * support.
 *
 * FIXME - They should be in a different file.
 */
/* We use Kerberos 5 routines to map the GSSAPI credential to a user
   name.  */
# include <krb5.h>

static void gserver_authenticate_connection (void);

/* Whether we are already wrapping GSSAPI communication.  */
static int cvs_gssapi_wrapping;

#endif	/* defined (HAVE_GSSAPI) && defined (SERVER_SUPPORT) */

#ifdef SERVER_SUPPORT

extern char *server_hostname;

# if defined (AUTH_SERVER_SUPPORT) || defined (HAVE_KERBEROS) || defined (HAVE_GSSAPI)
#   include <sys/socket.h>
# endif

# ifdef HAVE_SYSLOG_H
#   include <syslog.h>
#   ifndef LOG_DAEMON   /* for ancient syslogs */
#     define LOG_DAEMON 0
#   endif
# endif /* HAVE_SYSLOG_H */

# ifdef HAVE_KERBEROS
#   include <netinet/in.h>
#   include <krb.h>
#   ifndef HAVE_KRB_GET_ERR_TEXT
#     define krb_get_err_text(status) krb_err_txt[status]
#   endif

/* Information we need if we are going to use Kerberos encryption.  */
static C_Block kblock;
static Key_schedule sched;

# endif /* HAVE_KERBEROS */

/* for select */
# include "xselect.h"

# ifndef O_NONBLOCK
#   define O_NONBLOCK O_NDELAY
# endif

/* For initgroups().  */
# if HAVE_INITGROUPS
#   include <grp.h>
# endif /* HAVE_INITGROUPS */

# ifdef AUTH_SERVER_SUPPORT

#   ifdef HAVE_GETSPNAM
#     include <shadow.h>
#   endif

/* The cvs username sent by the client, which might or might not be
   the same as the system username the server eventually switches to
   run as.  CVS_Username gets set iff password authentication is
   successful. */
char *CVS_Username = NULL;

/* Used to check that same repos is transmitted in pserver auth and in
   later CVS protocol.  Exported because root.c also uses. */
static char *Pserver_Repos = NULL;

# endif /* AUTH_SERVER_SUPPORT */

# ifdef HAVE_PAM
#   if defined(HAVE_SECURITY_PAM_APPL_H)
#     include <security/pam_appl.h>
#   elif defined(HAVE_PAM_PAM_APPL_H)
#     include <pam/pam_appl.h>
#   endif

static pam_handle_t *pamh = NULL;

static char *pam_username;
static char *pam_password;
# endif /* HAVE_PAM */



/* While processing requests, this buffer accumulates data to be sent to
   the client, and then once we are in do_cvs_command, we use it
   for all the data to be sent.  */
static struct buffer *buf_to_net;

/* This buffer is used to read input from the client.  */
static struct buffer *buf_from_net;



# ifdef PROXY_SUPPORT
/* These are the secondary log buffers so that we can disable them after
 * creation, when it is determined that they are unneeded, regardless of what
 * other filters have been prepended to the buffer chain.
 */
static struct buffer *proxy_log;
static struct buffer *proxy_log_out;

/* Set while we are reprocessing a log so that we can avoid sending responses
 * to some requests twice.
 */
static bool reprocessing;
# endif /* PROXY_SUPPORT */



/* Arguments storage for `Argument' & `Argumentx' requests.  */
static int argument_count;
static char **argument_vector;
static int argument_vector_size;

/*
 * This is where we stash stuff we are going to use.  Format string
 * which expects a single directory within it, starting with a slash.
 */
static char *server_temp_dir;

/* This is the original value of server_temp_dir, before any possible
   changes inserted by serve_max_dotdot.  */
static char *orig_server_temp_dir;

/* Nonzero if we should keep the temp directory around after we exit.  */
static int dont_delete_temp;

static void server_write_entries (void);

cvsroot_t *referrer;



/* Populate all of the directories between BASE_DIR and its relative
   subdirectory DIR with CVSADM directories.  Return 0 for success or
   errno value.  */
static int
create_adm_p (char *base_dir, char *dir)
{
    char *dir_where_cvsadm_lives, *dir_to_register, *p, *tmp;
    int retval, done;
    FILE *f;

    if (strcmp (dir, ".") == 0)
	return 0;			/* nothing to do */

    /* Allocate some space for our directory-munging string. */
    p = xmalloc (strlen (dir) + 1);
    if (p == NULL)
	return ENOMEM;

    dir_where_cvsadm_lives = xmalloc (strlen (base_dir) + strlen (dir) + 100);
    if (dir_where_cvsadm_lives == NULL)
    {
	free (p);
	return ENOMEM;
    }

    /* Allocate some space for the temporary string in which we will
       construct filenames. */
    tmp = xmalloc (strlen (base_dir) + strlen (dir) + 100);
    if (tmp == NULL)
    {
	free (p);
	free (dir_where_cvsadm_lives);
	return ENOMEM;
    }


    /* We make several passes through this loop.  On the first pass,
       we simply create the CVSADM directory in the deepest directory.
       For each subsequent pass, we try to remove the last path
       element from DIR, create the CVSADM directory in the remaining
       pathname, and register the subdirectory in the newly created
       CVSADM directory. */

    retval = done = 0;

    strcpy (p, dir);
    strcpy (dir_where_cvsadm_lives, base_dir);
    strcat (dir_where_cvsadm_lives, "/");
    strcat (dir_where_cvsadm_lives, p);
    dir_to_register = NULL;

    while (1)
    {
	/* Create CVSADM. */
	(void) sprintf (tmp, "%s/%s", dir_where_cvsadm_lives, CVSADM);
	if ((CVS_MKDIR (tmp, 0777) < 0) && (errno != EEXIST))
	{
	    retval = errno;
	    goto finish;
	}

	/* Create CVSADM_REP. */
	(void) sprintf (tmp, "%s/%s", dir_where_cvsadm_lives, CVSADM_REP);
	if (! isfile (tmp))
	{
	    /* Use Emptydir as the placeholder until the client sends
	       us the real value.  This code is similar to checkout.c
	       (emptydir_name), but the code below returns errors
	       differently.  */

	    char *empty;
	    empty = xmalloc (strlen (current_parsed_root->directory)
			    + sizeof (CVSROOTADM)
			    + sizeof (CVSNULLREPOS)
			    + 3);
	    if (! empty)
	    {
		retval = ENOMEM;
		goto finish;
	    }

	    /* Create the directory name. */
	    (void) sprintf (empty, "%s/%s/%s", current_parsed_root->directory,
			    CVSROOTADM, CVSNULLREPOS);

	    /* Create the directory if it doesn't exist. */
	    if (! isfile (empty))
	    {
		mode_t omask;
		omask = umask (cvsumask);
		if (CVS_MKDIR (empty, 0777) < 0)
		{
		    retval = errno;
		    free (empty);
		    goto finish;
		}
		(void) umask (omask);
	    }

	    f = CVS_FOPEN (tmp, "w");
	    if (f == NULL)
	    {
		retval = errno;
		free (empty);
		goto finish;
	    }
	    /* Write the directory name to CVSADM_REP. */
	    if (fprintf (f, "%s\n", empty) < 0)
	    {
		retval = errno;
		fclose (f);
		free (empty);
		goto finish;
	    }
	    if (fclose (f) == EOF)
	    {
		retval = errno;
		free (empty);
		goto finish;
	    }

	    /* Clean up after ourselves. */
	    free (empty);
	}

	/* Create CVSADM_ENT.  We open in append mode because we
	   don't want to clobber an existing Entries file.  */
	(void) sprintf (tmp, "%s/%s", dir_where_cvsadm_lives, CVSADM_ENT);
	f = CVS_FOPEN (tmp, "a");
	if (f == NULL)
	{
	    retval = errno;
	    goto finish;
	}
	if (fclose (f) == EOF)
	{
	    retval = errno;
	    goto finish;
	}

	if (dir_to_register != NULL)
	{
	    /* FIXME: Yes, this results in duplicate entries in the
	       Entries.Log file, but it doesn't currently matter.  We
	       might need to change this later on to make sure that we
	       only write one entry.  */

	    Subdir_Register (NULL, dir_where_cvsadm_lives, dir_to_register);
	}

	if (done)
	    break;

	dir_to_register = strrchr (p, '/');
	if (dir_to_register == NULL)
	{
	    dir_to_register = p;
	    strcpy (dir_where_cvsadm_lives, base_dir);
	    done = 1;
	}
	else
	{
	    *dir_to_register = '\0';
	    dir_to_register++;
	    strcpy (dir_where_cvsadm_lives, base_dir);
	    strcat (dir_where_cvsadm_lives, "/");
	    strcat (dir_where_cvsadm_lives, p);
	}
    }

  finish:
    free (tmp);
    free (dir_where_cvsadm_lives);
    free (p);
    return retval;
}



/*
 * Make directory DIR, including all intermediate directories if necessary.
 * Returns 0 for success or errno code.
 */
static int
mkdir_p (char *dir)
{
    char *p;
    char *q = xmalloc (strlen (dir) + 1);
    int retval;

    if (q == NULL)
	return ENOMEM;

    retval = 0;

    /*
     * Skip over leading slash if present.  We won't bother to try to
     * make '/'.
     */
    p = dir + 1;
    while (1)
    {
	while (*p != '/' && *p != '\0')
	    ++p;
	if (*p == '/')
	{
	    strncpy (q, dir, p - dir);
	    q[p - dir] = '\0';
	    if (q[p - dir - 1] != '/'  &&  CVS_MKDIR (q, 0777) < 0)
	    {
		int saved_errno = errno;

		if (saved_errno != EEXIST
		    && ((saved_errno != EACCES && saved_errno != EROFS)
			|| !isdir (q)))
		{
		    retval = saved_errno;
		    goto done;
		}
	    }
	    ++p;
	}
	else
	{
	    if (CVS_MKDIR (dir, 0777) < 0)
		retval = errno;
	    goto done;
	}
    }
  done:
    free (q);
    return retval;
}



/*
 * Print the error response for error code STATUS.  The caller is
 * reponsible for making sure we get back to the command loop without
 * any further output occuring.
 * Must be called only in contexts where it is OK to send output.
 */
static void
print_error (int status)
{
    char *msg;
    char tmpstr[80];

    buf_output0 (buf_to_net, "error  ");
    msg = strerror (status);
    if (msg == NULL)
    {
       sprintf (tmpstr, "unknown error %d", status);
       msg = tmpstr;
    }
    buf_output0 (buf_to_net, msg);
    buf_append_char (buf_to_net, '\n');

    buf_flush (buf_to_net, 0);
}



static int pending_error;
/*
 * Malloc'd text for pending error.  Each line must start with "E ".  The
 * last line should not end with a newline.
 */
static char *pending_error_text;
static char *pending_warning_text;

/* If an error is pending, print it and return 1.  If not, return 0.
   Also prints pending warnings, but this does not affect the return value.
   Must be called only in contexts where it is OK to send output.  */
static int
print_pending_error (void)
{
    /* Check this case first since it usually means we are out of memory and
     * the buffer output routines might try and allocate memory.
     */
    if (!pending_error_text && pending_error)
    {
	print_error (pending_error);
	pending_error = 0;
	return 1;
    }

    if (pending_warning_text)
    {
	buf_output0 (buf_to_net, pending_warning_text);
	buf_append_char (buf_to_net, '\n');
	buf_flush (buf_to_net, 0);

	free (pending_warning_text);
	pending_warning_text = NULL;
    }

    if (pending_error_text)
    {
	buf_output0 (buf_to_net, pending_error_text);
	buf_append_char (buf_to_net, '\n');
	if (pending_error)
	    print_error (pending_error);
	else
	    buf_output0 (buf_to_net, "error  \n");

	buf_flush (buf_to_net, 0);

	pending_error = 0;
	free (pending_error_text);
	pending_error_text = NULL;
	return 1;
    }

    return 0;
}



/* Is an error pending?  */
# define error_pending() (pending_error || pending_error_text)
# define warning_pending() (pending_warning_text)

/* Allocate SIZE bytes for pending_error_text and return nonzero
   if we could do it.  */
static inline int
alloc_pending_internal (char **dest, size_t size)
{
    *dest = malloc (size);
    if (!*dest)
    {
	pending_error = ENOMEM;
	return 0;
    }
    return 1;
}



/* Allocate SIZE bytes for pending_error_text and return nonzero
   if we could do it.  */
static int
alloc_pending (size_t size)
{
    if (error_pending ())
	/* Probably alloc_pending callers will have already checked for
	   this case.  But we might as well handle it if they don't, I
	   guess.  */
	return 0;
    return alloc_pending_internal (&pending_error_text, size);
}



/* Allocate SIZE bytes for pending_error_text and return nonzero
   if we could do it.  */
static int
alloc_pending_warning (size_t size)
{
    if (warning_pending ())
	/* Warnings can be lost here.  */
	return 0;
    return alloc_pending_internal (&pending_warning_text, size);
}



static int
supported_response (char *name)
{
    struct response *rs;

    for (rs = responses; rs->name != NULL; ++rs)
	if (strcmp (rs->name, name) == 0)
	    return rs->status == rs_supported;
    error (1, 0, "internal error: testing support for unknown response?");
    /* NOTREACHED */
    return 0;
}



/*
 * Return true if we need to relay write requests to a primary server
 * and false otherwise.
 *
 * NOTES
 *
 *   - primarily handles :ext: method as this seems most likely to be used in
 *     practice.
 *
 *   - :fork: method is handled for testing.
 *
 *   - Could handle pserver too, but would have to store the password
 *     the client sent us.
 *
 *
 * GLOBALS
 *   config->PrimaryServer
 *                        The parsed setting from CVSROOT/config, if any, or
 *                        NULL, otherwise.
 *   current_parsed_root  The current repository.
 *
 * RETURNS
 *   true                 If this server is configured as a secondary server.
 *   false                Otherwise.
 */
static inline bool
isProxyServer (void)
{
    assert (current_parsed_root);

    /***
     *** The following is done as a series of if/return combinations an an
     *** optimization.
     ***/

    /* If there is no primary server defined in CVSROOT/config, then we can't
     * be a secondary.
     */
    if (!config || !config->PrimaryServer) return false;

    /* The directory must not match for all methods.  */
    if (!isSamePath (config->PrimaryServer->directory,
		     current_parsed_root->directory))
	return true;

    /* Only the directory is important for fork.  */
    if (config->PrimaryServer->method == fork_method)
	return false;

    /* Must be :ext: method, then.  This is enforced when CVSROOT/config is
     * parsed.
     */
    assert (config->PrimaryServer->isremote);

    if (isThisHost (config->PrimaryServer->hostname))
	return false;

    return true;
}



static void
serve_valid_responses (char *arg)
{
    char *p = arg;
    char *q;
    struct response *rs;

# ifdef PROXY_SUPPORT
    /* Process this in the first pass since the data it gathers can be used
     * prior to a `Root' request.
     */
    if (reprocessing) return;
# endif /* PROXY_SUPPORT */

    do
    {
	q = strchr (p, ' ');
	if (q != NULL)
	    *q++ = '\0';
	for (rs = responses; rs->name != NULL; ++rs)
	{
	    if (strcmp (rs->name, p) == 0)
		break;
	}
	if (rs->name == NULL)
	    /*
	     * It is a response we have never heard of (and thus never
	     * will want to use).  So don't worry about it.
	     */
	    ;
	else
	    rs->status = rs_supported;
	p = q;
    } while (q != NULL);
    for (rs = responses; rs->name != NULL; ++rs)
    {
	if (rs->status == rs_essential)
	{
	    buf_output0 (buf_to_net, "E response `");
	    buf_output0 (buf_to_net, rs->name);
	    buf_output0 (buf_to_net, "' not supported by client\nerror  \n");

	    /* FIXME: This call to buf_flush could conceivably
	       cause deadlock, as noted in server_cleanup.  */
	    buf_flush (buf_to_net, 1);

	    exit (EXIT_FAILURE);
	}
	else if (rs->status == rs_optional)
	    rs->status = rs_not_supported;
    }
}



/*
 * Process IDs of the subprocess, or negative if that subprocess
 * does not exist.
 */
static pid_t command_pid;

static void
outbuf_memory_error (struct buffer *buf)
{
    static const char msg[] = "E Fatal server error\n\
error ENOMEM Virtual memory exhausted.\n";
    if (command_pid > 0)
	kill (command_pid, SIGTERM);

    /*
     * We have arranged things so that printing this now either will
     * be valid, or the "E fatal error" line will get glommed onto the
     * end of an existing "E" or "M" response.
     */

    /* If this gives an error, not much we could do.  syslog() it?  */
    write (STDOUT_FILENO, msg, sizeof (msg) - 1);
# ifdef HAVE_SYSLOG_H
    syslog (LOG_DAEMON | LOG_ERR, "virtual memory exhausted");
# endif /* HAVE_SYSLOG_H */
    exit (EXIT_FAILURE);
}



static void
input_memory_error (struct buffer *buf)
{
    outbuf_memory_error (buf);
}



# ifdef PROXY_SUPPORT
/* This function rewinds the net connection using the write proxy log file.
 *
 * GLOBALS
 *   proxy_log	The buffer object containing the write proxy log.
 *
 * RETURNS
 *   Nothing.
 */
static void
rewind_buf_from_net (void)
{
    struct buffer *log;

    assert (proxy_log);

    /* Free the arguments since we processed some of them in the first pass.
     */
    {
	/* argument_vector[0] is a dummy argument, we don't mess with
	 * it.
	 */
	char **cp;
	for (cp = argument_vector + 1;
	     cp < argument_vector + argument_count;
	     ++cp)
	    free (*cp);

	argument_count = 1;
    }

    log = log_buffer_rewind (proxy_log);
    proxy_log = NULL;
    /* Dispose of any read but unused data in the net buffer since it will
     * already be in the log.
     */
    buf_free_data (buf_from_net);
    buf_from_net = ms_buffer_initialize (outbuf_memory_error, log,
					 buf_from_net);
    reprocessing = true;
}
# endif /* PROXY_SUPPORT */



char *gConfigPath;



/*
 * This request cannot be ignored by a potential secondary since it is used to
 * determine if we _are_ a secondary.
 */
static void
serve_root (char *arg)
{
    char *path;

    TRACE (TRACE_FUNCTION, "serve_root (%s)", arg ? arg : "(null)");

    /* Don't process this twice or when errors are pending.  */
    if (error_pending()
# ifdef PROXY_SUPPORT
	|| reprocessing
# endif /* PROXY_SUPPORT */
       ) return;

    if (!ISABSOLUTE (arg))
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text,
		     "E Root %s must be an absolute pathname", arg);
	return;
    }

    /* Sending "Root" twice is invalid.

       The other way to handle a duplicate Root requests would be as a
       request to clear out all state and start over as if it was a
       new connection.  Doing this would cause interoperability
       headaches, so it should be a different request, if there is
       any reason why such a feature is needed.  */
    if (current_parsed_root != NULL)
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text,
		     "E Protocol error: Duplicate Root request, for %s", arg);
	return;
    }

    /* Set original_parsed_root here, not because it can be changed in the
     * client Redirect sense, but so we don't have to switch in code that
     * runs in both modes to decide which to print.
     */
    original_parsed_root = current_parsed_root = local_cvsroot (arg);

# ifdef AUTH_SERVER_SUPPORT
    if (Pserver_Repos != NULL)
    {
	if (strcmp (Pserver_Repos, current_parsed_root->directory) != 0)
	{
	    if (alloc_pending (80 + strlen (Pserver_Repos)
			       + strlen (current_parsed_root->directory)))
		/* The explicitness is to aid people who are writing clients.
		   I don't see how this information could help an
		   attacker.  */
		sprintf (pending_error_text, "\
E Protocol error: Root says \"%s\" but pserver says \"%s\"",
			 current_parsed_root->directory, Pserver_Repos);
	    return;
	}
    }
# endif

    /* For pserver, this will already have happened, and the call will do
       nothing.  But for rsh, we need to do it now.  */
    config = get_root_allow_config (current_parsed_root->directory,
				    gConfigPath);

# ifdef PROXY_SUPPORT
    /* At this point we have enough information to determine if we are a
     * secondary server or not.
     */
    if (proxy_log && !isProxyServer ())
    {
	/* Else we are not a secondary server.  There is no point in
	 * reprocessing since we handle all the requests we can receive
	 * before `Root' as we receive them.  But close the logs.
	 */
	log_buffer_closelog (proxy_log);
	log_buffer_closelog (proxy_log_out);
	proxy_log = NULL;
	/*
	 * Don't need this.  We assume it when proxy_log == NULL.
	 *
	 *   proxy_log_out = NULL;
	 */
    }
# endif /* PROXY_SUPPORT */

    /* Now set the TMPDIR environment variable.  If it was set in the config
     * file, we now know it.
     */
    push_env_temp_dir ();

    /* OK, now figure out where we stash our temporary files.  */
    {
	char *p;

	/* The code which wants to chdir into server_temp_dir is not set
	 * up to deal with it being a relative path.  So give an error
	 * for that case.
	 */
	if (!ISABSOLUTE (get_cvs_tmp_dir ()))
	{
	    if (alloc_pending (80 + strlen (get_cvs_tmp_dir ())))
		sprintf (pending_error_text,
			 "E Value of %s for TMPDIR is not absolute",
			 get_cvs_tmp_dir ());

	    /* FIXME: we would like this error to be persistent, that
	     * is, not cleared by print_pending_error.  The current client
	     * will exit as soon as it gets an error, but the protocol spec
	     * does not require a client to do so.
	     */
	}
	else
	{
	    int status;
	    int i = 0;

	    server_temp_dir = xmalloc (strlen (get_cvs_tmp_dir ()) + 80);
	    if (!server_temp_dir)
	    {
		/* Strictly speaking, we're not supposed to output anything
		 * now.  But we're about to exit(), give it a try.
		 */
		printf ("E Fatal server error, aborting.\n\
error ENOMEM Virtual memory exhausted.\n");

		exit (EXIT_FAILURE);
	    }
	    strcpy (server_temp_dir, get_cvs_tmp_dir ());

	    /* Remove a trailing slash from TMPDIR if present.  */
	    p = server_temp_dir + strlen (server_temp_dir) - 1;
	    if (*p == '/')
		*p = '\0';

	    /* I wanted to use cvs-serv/PID, but then you have to worry about
	     * the permissions on the cvs-serv directory being right.  So
	     * use cvs-servPID.
	     */
	    strcat (server_temp_dir, "/cvs-serv");

	    p = server_temp_dir + strlen (server_temp_dir);
	    sprintf (p, "%ld", (long) getpid ());

	    orig_server_temp_dir = server_temp_dir;

	    /* Create the temporary directory, and set the mode to
	     * 700, to discourage random people from tampering with
	     * it.
	     */
	    while ((status = mkdir_p (server_temp_dir)) == EEXIST)
	    {
		static const char suffix[] = "abcdefghijklmnopqrstuvwxyz";

		if (i >= sizeof suffix - 1) break;
		if (i == 0) p = server_temp_dir + strlen (server_temp_dir);
		p[0] = suffix[i++];
		p[1] = '\0';
	    }
	    if (status)
	    {
		if (alloc_pending (80 + strlen (server_temp_dir)))
		    sprintf (pending_error_text,
			    "E can't create temporary directory %s",
			    server_temp_dir);
		pending_error = status;
	    }
#ifndef CHMOD_BROKEN
	    else if (chmod (server_temp_dir, S_IRWXU) < 0)
	    {
		int save_errno = errno;
		if (alloc_pending (80 + strlen (server_temp_dir)))
		    sprintf (pending_error_text,
"E cannot change permissions on temporary directory %s",
			     server_temp_dir);
		pending_error = save_errno;
	    }
#endif
	    else if (CVS_CHDIR (server_temp_dir) < 0)
	    {
		int save_errno = errno;
		if (alloc_pending (80 + strlen (server_temp_dir)))
		    sprintf (pending_error_text,
"E cannot change to temporary directory %s",
			     server_temp_dir);
		pending_error = save_errno;
	    }
	}
    }

    /* Now that we have a config, verify our compression level.  Since 
     * most clients do not send Gzip-stream requests until after the root
     * request, wait until the first request following Root to verify that
     * compression is being used when level 0 is not allowed.
     */
    if (gzip_level)
    {
	bool forced = false;

	if (gzip_level < config->MinCompressionLevel)
	{
	    gzip_level = config->MinCompressionLevel;
	    forced = true;
	}

	if (gzip_level > config->MaxCompressionLevel)
	{
	    gzip_level = config->MaxCompressionLevel;
	    forced = true;
	}

	if (forced && !quiet
	    && alloc_pending_warning (120 + strlen (program_name)))
	    sprintf (pending_warning_text,
"E %s server: Forcing compression level %d (allowed: %d <= z <= %d).",
		     program_name, gzip_level, config->MinCompressionLevel,
		     config->MaxCompressionLevel);
    }

    if (!nolock) {
    path = xmalloc (strlen (current_parsed_root->directory)
		   + sizeof (CVSROOTADM)
		   + 2);
    if (path == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    (void) sprintf (path, "%s/%s", current_parsed_root->directory, CVSROOTADM);
    if (!isaccessible (path, R_OK | X_OK))
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (path)))
	    sprintf (pending_error_text, "E Cannot access %s", path);
	pending_error = save_errno;
    }
    free (path);
    }

    setenv (CVSROOT_ENV, current_parsed_root->directory, 1);
}



static int max_dotdot_limit = 0;

/* Is this pathname OK to recurse into when we are running as the server?
   If not, call error() with a fatal error.  */
void
server_pathname_check (char *path)
{
    TRACE (TRACE_FUNCTION, "server_pathname_check (%s)",
	   path ? path : "(null)");

    /* An absolute pathname is almost surely a path on the *client* machine,
       and is unlikely to do us any good here.  It also is probably capable
       of being a security hole in the anonymous readonly case.  */
    if (ISABSOLUTE (path))
	/* Giving an error is actually kind of a cop-out, in the sense
	   that it would be nice for "cvs co -d /foo/bar/baz" to work.
	   A quick fix in the server would be requiring Max-dotdot of
	   at least one if pathnames are absolute, and then putting
	   /abs/foo/bar/baz in the temp dir beside the /d/d/d stuff.
	   A cleaner fix in the server might be to decouple the
	   pathnames we pass back to the client from pathnames in our
	   temp directory (this would also probably remove the need
	   for Max-dotdot).  A fix in the client would have the client
	   turn it into "cd /foo/bar; cvs co -d baz" (more or less).
	   This probably has some problems with pathnames which appear
	   in messages.  */
	error ( 1, 0,
		"absolute pathnames invalid for server (specified `%s')",
		path );
    if (pathname_levels (path) > max_dotdot_limit)
    {
	/* Similar to the ISABSOLUTE case in security implications.  */
	error (0, 0, "protocol error: `%s' contains more leading ..", path);
	error (1, 0, "than the %d which Max-dotdot specified",
	       max_dotdot_limit);
    }
}



/* Is file or directory REPOS an absolute pathname within the
   current_parsed_root->directory?  If yes, return 0.  If no, set pending_error
   and return 1.  */
static int
outside_root (char *repos)
{
    size_t repos_len = strlen (repos);
    size_t root_len = strlen (current_parsed_root->directory);

    /* ISABSOLUTE (repos) should always be true, but
       this is a good security precaution regardless. -DRP
     */
    if (!ISABSOLUTE (repos))
    {
	if (alloc_pending (repos_len + 80))
	    sprintf (pending_error_text, "\
E protocol error: %s is not absolute", repos);
	return 1;
    }

    if (repos_len < root_len
	|| strncmp (current_parsed_root->directory, repos, root_len) != 0)
    {
    not_within:
	if (alloc_pending (strlen (current_parsed_root->directory)
			   + strlen (repos)
			   + 80))
	    sprintf (pending_error_text, "\
E protocol error: directory '%s' not within root '%s'",
		     repos, current_parsed_root->directory);
	return 1;
    }
    if (repos_len > root_len)
    {
	if (repos[root_len] != '/')
	    goto not_within;
	if (pathname_levels (repos + root_len + 1) > 0)
	    goto not_within;
    }
    return 0;
}



/* Is file or directory FILE outside the current directory (that is, does
   it contain '/')?  If no, return 0.  If yes, set pending_error
   and return 1.  */
static int
outside_dir (char *file)
{
    if (strchr (file, '/') != NULL)
    {
	if (alloc_pending (strlen (file)
			   + 80))
	    sprintf (pending_error_text, "\
E protocol error: directory '%s' not within current directory",
		     file);
	return 1;
    }
    return 0;
}



/*
 * Add as many directories to the temp directory as the client tells us it
 * will use "..", so we never try to access something outside the temp
 * directory via "..".
 */
static void
serve_max_dotdot (char *arg)
{
    int lim = atoi (arg);
    int i;
    char *p;

#ifdef PROXY_SUPPORT
    if (proxy_log) return;
#endif /* PROXY_SUPPORT */

    if (lim < 0 || lim > 10000)
	return;
    p = xmalloc (strlen (server_temp_dir) + 2 * lim + 10);
    if (p == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    strcpy (p, server_temp_dir);
    for (i = 0; i < lim; ++i)
	strcat (p, "/d");
    if (server_temp_dir != orig_server_temp_dir)
	free (server_temp_dir);
    server_temp_dir = p;
    max_dotdot_limit = lim;
}



static char *gDirname;
static char *gupdate_dir;

static void
dirswitch (char *dir, char *repos)
{
    int status;
    FILE *f;
    size_t dir_len;

    TRACE (TRACE_FUNCTION, "dirswitch (%s, %s)", dir ? dir : "(null)",
	   repos ? repos : "(null)");

    server_write_entries ();

    if (error_pending()) return;

    /* Check for bad directory name.

       FIXME: could/should unify these checks with server_pathname_check
       except they need to report errors differently.  */
    if (ISABSOLUTE (dir))
    {
	if (alloc_pending (80 + strlen (dir)))
	    sprintf ( pending_error_text,
		      "E absolute pathnames invalid for server (specified `%s')",
		      dir);
	return;
    }
    if (pathname_levels (dir) > max_dotdot_limit)
    {
	if (alloc_pending (80 + strlen (dir)))
	    sprintf (pending_error_text,
		     "E protocol error: `%s' has too many ..", dir);
	return;
    }

    dir_len = strlen (dir);

    /* Check for a trailing '/'.  This is not ISSLASH because \ in the
       protocol is an ordinary character, not a directory separator (of
       course, it is perhaps unwise to use it in directory names, but that
       is another issue).  */
    if (dir_len > 0
	&& dir[dir_len - 1] == '/')
    {
	if (alloc_pending (80 + dir_len))
	    sprintf (pending_error_text,
		     "E protocol error: invalid directory syntax in %s", dir);
	return;
    }

    if (gDirname != NULL)
	free (gDirname);
    if (gupdate_dir != NULL)
	free (gupdate_dir);

    if (!strcmp (dir, "."))
	gupdate_dir = xstrdup ("");
    else
	gupdate_dir = xstrdup (dir);

    gDirname = xmalloc (strlen (server_temp_dir) + dir_len + 40);
    if (gDirname == NULL)
    {
	pending_error = ENOMEM;
	return;
    }

    strcpy (gDirname, server_temp_dir);
    strcat (gDirname, "/");
    strcat (gDirname, dir);

    status = mkdir_p (gDirname);
    if (status != 0
	&& status != EEXIST)
    {
	if (alloc_pending (80 + strlen (gDirname)))
	    sprintf (pending_error_text, "E cannot mkdir %s", gDirname);
	pending_error = status;
	return;
    }

    /* We need to create adm directories in all path elements because
       we want the server to descend them, even if the client hasn't
       sent the appropriate "Argument xxx" command to match the
       already-sent "Directory xxx" command.  See recurse.c
       (start_recursion) for a big discussion of this.  */

    status = create_adm_p (server_temp_dir, dir);
    if (status != 0)
    {
	if (alloc_pending (80 + strlen (gDirname)))
	    sprintf (pending_error_text, "E cannot create_adm_p %s", gDirname);
	pending_error = status;
	return;
    }

    if ( CVS_CHDIR (gDirname) < 0)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (gDirname)))
	    sprintf (pending_error_text, "E cannot change to %s", gDirname);
	pending_error = save_errno;
	return;
    }
    /*
     * This is pretty much like calling Create_Admin, but Create_Admin doesn't
     * report errors in the right way for us.
     */
    if ((CVS_MKDIR (CVSADM, 0777) < 0) && (errno != EEXIST))
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (gDirname) + strlen (CVSADM)))
	    sprintf (pending_error_text,
		     "E cannot mkdir %s/%s", gDirname, CVSADM);
	pending_error = save_errno;
	return;
    }

    /* The following will overwrite the contents of CVSADM_REP.  This
       is the correct behavior -- mkdir_p may have written a
       placeholder value to this file and we need to insert the
       correct value. */

    f = CVS_FOPEN (CVSADM_REP, "w");
    if (f == NULL)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (gDirname) + strlen (CVSADM_REP)))
	    sprintf (pending_error_text,
		     "E cannot open %s/%s", gDirname, CVSADM_REP);
	pending_error = save_errno;
	return;
    }
    if (fprintf (f, "%s", repos) < 0)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (gDirname) + strlen (CVSADM_REP)))
	    sprintf (pending_error_text,
		     "E error writing %s/%s", gDirname, CVSADM_REP);
	pending_error = save_errno;
	fclose (f);
	return;
    }
    /* Non-remote CVS handles a module representing the entire tree
       (e.g., an entry like ``world -a .'') by putting /. at the end
       of the Repository file, so we do the same.  */
    if (strcmp (dir, ".") == 0
	&& current_parsed_root != NULL
	&& current_parsed_root->directory != NULL
	&& strcmp (current_parsed_root->directory, repos) == 0)
    {
	if (fprintf (f, "/.") < 0)
	{
	    int save_errno = errno;
	    if (alloc_pending (80 + strlen (gDirname) + strlen (CVSADM_REP)))
		sprintf (pending_error_text,
			 "E error writing %s/%s", gDirname, CVSADM_REP);
	    pending_error = save_errno;
	    fclose (f);
	    return;
	}
    }
    if (fprintf (f, "\n") < 0)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (gDirname) + strlen (CVSADM_REP)))
	    sprintf (pending_error_text,
		     "E error writing %s/%s", gDirname, CVSADM_REP);
	pending_error = save_errno;
	fclose (f);
	return;
    }
    if (fclose (f) == EOF)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (gDirname) + strlen (CVSADM_REP)))
	    sprintf (pending_error_text,
		     "E error closing %s/%s", gDirname, CVSADM_REP);
	pending_error = save_errno;
	return;
    }
    /* We open in append mode because we don't want to clobber an
       existing Entries file.  */
    f = CVS_FOPEN (CVSADM_ENT, "a");
    if (f == NULL)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_ENT)))
	    sprintf (pending_error_text, "E cannot open %s", CVSADM_ENT);
	pending_error = save_errno;
	return;
    }
    if (fclose (f) == EOF)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_ENT)))
	    sprintf (pending_error_text, "E cannot close %s", CVSADM_ENT);
	pending_error = save_errno;
	return;
    }
}



static void
serve_repository (char *arg)
{
# ifdef PROXY_SUPPORT
    assert (!proxy_log);
# endif /* PROXY_SUPPORT */

    if (alloc_pending (80))
	strcpy (pending_error_text,
		"E Repository request is obsolete; aborted");
    return;
}



static void
serve_directory (char *arg)
{
    int status;
    char *repos;

    TRACE (TRACE_FUNCTION, "serve_directory (%s)", arg ? arg : "(null)");


    /* The data needs to be read into the secondary log regardless, but
     * processing of anything other than errors is skipped until later.
     */
    status = buf_read_line (buf_from_net, &repos, NULL);
    if (status == 0)
    {
	if (!ISABSOLUTE (repos))
	{
	    /* Make absolute.
	     *
	     * FIXME: This is kinda hacky - we should probably only ever store
	     * and pass SHORT_REPOS (perhaps with the occassional exception
	     * for optimizations, but many, many functions end up
	     * deconstructing REPOS to gain SHORT_REPOS anyhow) - the
	     * CVSROOT portion of REPOS is redundant with
	     * current_parsed_root->directory - but since this is the way
	     * things have always been done, changing this will likely involve
	     * a major overhaul.
	     */
	    char *short_repos;

	    short_repos = repos;
	    repos = Xasprintf ("%s/%s",
	                      current_parsed_root->directory, short_repos);
	    free (short_repos);
	}
	else
	    repos = xstrdup (primary_root_translate (repos));

	if (
# ifdef PROXY_SUPPORT
	    !proxy_log &&
# endif /* PROXY_SUPPORT */
	    !outside_root (repos))
	    dirswitch (arg, repos);
	free (repos);
    }
    else if (status == -2)
    {
	pending_error = ENOMEM;
    }
    else if (status != 0)
    {
	pending_error_text = xmalloc (80 + strlen (arg));
	if (pending_error_text == NULL)
	{
	    pending_error = ENOMEM;
	}
	else if (status == -1)
	{
	    sprintf (pending_error_text,
		     "E end of file reading mode for %s", arg);
	}
	else
	{
	    sprintf (pending_error_text,
		     "E error reading mode for %s", arg);
	    pending_error = status;
	}
    }
}



static void
serve_static_directory (char *arg)
{
    FILE *f;

    if (error_pending ()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       ) return;

    f = CVS_FOPEN (CVSADM_ENTSTAT, "w+");
    if (f == NULL)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_ENTSTAT)))
	    sprintf (pending_error_text, "E cannot open %s", CVSADM_ENTSTAT);
	pending_error = save_errno;
	return;
    }
    if (fclose (f) == EOF)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_ENTSTAT)))
	    sprintf (pending_error_text, "E cannot close %s", CVSADM_ENTSTAT);
	pending_error = save_errno;
	return;
    }
}



static void
serve_sticky (char *arg)
{
    FILE *f;

    if (error_pending ()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       ) return;

    f = CVS_FOPEN (CVSADM_TAG, "w+");
    if (f == NULL)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_TAG)))
	    sprintf (pending_error_text, "E cannot open %s", CVSADM_TAG);
	pending_error = save_errno;
	return;
    }
    if (fprintf (f, "%s\n", arg) < 0)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_TAG)))
	    sprintf (pending_error_text, "E cannot write to %s", CVSADM_TAG);
	pending_error = save_errno;
	return;
    }
    if (fclose (f) == EOF)
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_TAG)))
	    sprintf (pending_error_text, "E cannot close %s", CVSADM_TAG);
	pending_error = save_errno;
	return;
    }
}



/*
 * Read SIZE bytes from buf_from_net, write them to FILE.
 *
 * Currently this isn't really used for receiving parts of a file --
 * the file is still sent over in one chunk.  But if/when we get
 * spiffy in-process gzip support working, perhaps the compressed
 * pieces could be sent over as they're ready, if the network is fast
 * enough.  Or something.
 */
static void
receive_partial_file (size_t size, int file)
{
    while (size > 0)
    {
	int status;
	size_t nread;
	char *data;

	status = buf_read_data (buf_from_net, size, &data, &nread);
	if (status != 0)
	{
	    if (status == -2)
		pending_error = ENOMEM;
	    else
	    {
		pending_error_text = xmalloc (80);
		if (pending_error_text == NULL)
		    pending_error = ENOMEM;
		else if (status == -1)
		{
		    sprintf (pending_error_text,
			     "E premature end of file from client");
		    pending_error = 0;
		}
		else
		{
		    sprintf (pending_error_text,
			     "E error reading from client");
		    pending_error = status;
		}
	    }
	    return;
	}

	size -= nread;

	while (nread > 0)
	{
	    ssize_t nwrote;

	    nwrote = write (file, data, nread);
	    if (nwrote < 0)
	    {
		int save_errno = errno;
		if (alloc_pending (40))
		    strcpy (pending_error_text, "E unable to write");
		pending_error = save_errno;

		/* Read and discard the file data.  */
		while (size > 0)
		{
		    int status;
		    size_t nread;
		    char *data;

		    status = buf_read_data (buf_from_net, size, &data, &nread);
		    if (status != 0)
			return;
		    size -= nread;
		}

		return;
	    }
	    nread -= nwrote;
	    data += nwrote;
	}
    }
}



/* Receive SIZE bytes, write to filename FILE.  */
static void
receive_file (size_t size, char *file, int gzipped)
{
    int fd;
    char *arg = file;

    /* Write the file.  */
    fd = CVS_OPEN (arg, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
	int save_errno = errno;
	if (alloc_pending (40 + strlen (arg)))
	    sprintf (pending_error_text, "E cannot open %s", arg);
	pending_error = save_errno;
	return;
    }

    if (gzipped)
    {
	/* Using gunzip_and_write isn't really a high-performance
	   approach, because it keeps the whole thing in memory
	   (contiguous memory, worse yet).  But it seems easier to
	   code than the alternative (and less vulnerable to subtle
	   bugs).  Given that this feature is mainly for
	   compatibility, that is the better tradeoff.  */

	size_t toread = size;
	char *filebuf;
	char *p;

	filebuf = xmalloc (size);
	p = filebuf;
	/* If NULL, we still want to read the data and discard it.  */

	while (toread > 0)
	{
	    int status;
	    size_t nread;
	    char *data;

	    status = buf_read_data (buf_from_net, toread, &data, &nread);
	    if (status != 0)
	    {
		if (status == -2)
		    pending_error = ENOMEM;
		else
		{
		    pending_error_text = xmalloc (80);
		    if (pending_error_text == NULL)
			pending_error = ENOMEM;
		    else if (status == -1)
		    {
			sprintf (pending_error_text,
				 "E premature end of file from client");
			pending_error = 0;
		    }
		    else
		    {
			sprintf (pending_error_text,
				 "E error reading from client");
			pending_error = status;
		    }
		}
		return;
	    }

	    toread -= nread;

	    if (filebuf != NULL)
	    {
		memcpy (p, data, nread);
		p += nread;
	    }
	}
	if (filebuf == NULL)
	{
	    pending_error = ENOMEM;
	    goto out;
	}

	if (gunzip_and_write (fd, file, (unsigned char *) filebuf, size))
	{
	    if (alloc_pending (80))
		sprintf (pending_error_text,
			 "E aborting due to compression error");
	}
	free (filebuf);
    }
    else
	receive_partial_file (size, fd);

    if (pending_error_text)
    {
	char *p = xrealloc (pending_error_text,
			   strlen (pending_error_text) + strlen (arg) + 30);
	if (p)
	{
	    pending_error_text = p;
	    sprintf (p + strlen (p), ", file %s", arg);
	}
	/* else original string is supposed to be unchanged */
    }

 out:
    if (close (fd) < 0 && !error_pending ())
    {
	int save_errno = errno;
	if (alloc_pending (40 + strlen (arg)))
	    sprintf (pending_error_text, "E cannot close %s", arg);
	pending_error = save_errno;
	return;
    }
}



/* Kopt for the next file sent in Modified or Is-modified.  */
static char *kopt;

/* Timestamp (Checkin-time) for next file sent in Modified or
   Is-modified.  */
static int checkin_time_valid;
static time_t checkin_time;



/*
 * Used to keep track of Entry requests.
 */
struct an_entry {
    struct an_entry *next;
    char *entry;
};

static struct an_entry *entries;

static void
serve_is_modified (char *arg)
{
    struct an_entry *p;
    char *name;
    char *cp;
    char *timefield;
    /* Have we found this file in "entries" yet.  */
    int found;

    if (error_pending ()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       ) return;

    if (outside_dir (arg))
	return;

    /* Rewrite entries file to have `M' in timestamp field.  */
    found = 0;
    for (p = entries; p != NULL; p = p->next)
    {
	name = p->entry + 1;
	cp = strchr (name, '/');
	if (cp != NULL
	    && strlen (arg) == cp - name
	    && strncmp (arg, name, cp - name) == 0)
	{
	    if (!(timefield = strchr (cp + 1, '/')) || *++timefield == '\0')
	    {
		/* We didn't find the record separator or it is followed by
		 * the end of the string, so just exit.
		 */
		if (alloc_pending (80))
		    sprintf (pending_error_text,
		             "E Malformed Entry encountered.");
		return;
	    }
	    /* If the time field is not currently empty, then one of
	     * serve_modified, serve_is_modified, & serve_unchanged were
	     * already called for this file.  We would like to ignore the
	     * reinvocation silently or, better yet, exit with an error
	     * message, but we just avoid the copy-forward and overwrite the
	     * value from the last invocation instead.  See the comment below
	     * for more.
	     */
	    if (*timefield == '/')
	    {
		/* Copy forward one character.  Space was allocated for this
		 * already in serve_entry().  */
		cp = timefield + strlen (timefield);
		cp[1] = '\0';
		while (cp > timefield)
		{
		    *cp = cp[-1];
		    --cp;
		}

		/* *timefield == '/';  */
	    }
	    /* If *TIMEFIELD wasn't '/' and wasn't '+', we assume that it was
	     * because of multiple calls to Is-modified & Unchanged by the
	     * client and just overwrite the value from the last call.
	     * Technically, we should probably either ignore calls after the
	     * first or send the client an error, since the client/server
	     * protocol specification specifies that only one call to either
	     * Is-Modified or Unchanged is allowed, but broken versions of
	     * CVSNT (at least 2.0.34 - 2.0.41, reported fixed in 2.0.41a) and
	     * the WinCVS & TortoiseCVS clients which depend on those broken
	     * versions of CVSNT (WinCVS 1.3 & at least one TortoiseCVS
	     * release) rely on this behavior.
	     */
	    if (*timefield != '+')
		*timefield = 'M';

	    if (kopt != NULL)
	    {
		if (alloc_pending (strlen (name) + 80))
		    sprintf (pending_error_text,
			     "E protocol error: both Kopt and Entry for %s",
			     arg);
		free (kopt);
		kopt = NULL;
		return;
	    }
	    found = 1;
	    break;
	}
    }
    if (!found)
    {
	/* We got Is-modified but no Entry.  Add a dummy entry.
	   The "D" timestamp is what makes it a dummy.  */
	p = xmalloc (sizeof (struct an_entry));
	if (p == NULL)
	{
	    pending_error = ENOMEM;
	    return;
	}
	p->entry = xmalloc (strlen (arg) + 80);
	if (p->entry == NULL)
	{
	    pending_error = ENOMEM;
	    free (p);
	    return;
	}
	strcpy (p->entry, "/");
	strcat (p->entry, arg);
	strcat (p->entry, "//D/");
	if (kopt != NULL)
	{
	    strcat (p->entry, kopt);
	    free (kopt);
	    kopt = NULL;
	}
	strcat (p->entry, "/");
	p->next = entries;
	entries = p;
    }
}



static void
serve_modified (char *arg)
{
    size_t size;
    int read_size;
    int status;
    char *size_text;
    char *mode_text;

    int gzipped = 0;

    /*
     * This used to return immediately if error_pending () was true.
     * However, that fails, because it causes each line of the file to
     * be echoed back to the client as an unrecognized command.  The
     * client isn't reading from the socket, so eventually both
     * processes block trying to write to the other.  Now, we try to
     * read the file if we can.
     */

    status = buf_read_line (buf_from_net, &mode_text, NULL);
    if (status != 0)
    {
	if (status == -2)
	    pending_error = ENOMEM;
	else
	{
	    pending_error_text = xmalloc (80 + strlen (arg));
	    if (pending_error_text == NULL)
		pending_error = ENOMEM;
	    else
	    {
		if (status == -1)
		    sprintf (pending_error_text,
			     "E end of file reading mode for %s", arg);
		else
		{
		    sprintf (pending_error_text,
			     "E error reading mode for %s", arg);
		    pending_error = status;
		}
	    }
	}
	return;
    }

    status = buf_read_line (buf_from_net, &size_text, NULL);
    if (status != 0)
    {
	if (status == -2)
	    pending_error = ENOMEM;
	else
	{
	    pending_error_text = xmalloc (80 + strlen (arg));
	    if (pending_error_text == NULL)
		pending_error = ENOMEM;
	    else
	    {
		if (status == -1)
		    sprintf (pending_error_text,
			     "E end of file reading size for %s", arg);
		else
		{
		    sprintf (pending_error_text,
			     "E error reading size for %s", arg);
		    pending_error = status;
		}
	    }
	}
	free (mode_text);
	return;
    }
    if (size_text[0] == 'z')
    {
	gzipped = 1;
	read_size = atoi (size_text + 1);
    }
    else
	read_size = atoi (size_text);
    free (size_text);

    if (read_size < 0 && alloc_pending (80))
    {
	sprintf (pending_error_text,
		 "E client sent invalid (negative) file size");
	return;
    }
    else
	size = read_size;

    if (error_pending ())
    {
	/* Now that we know the size, read and discard the file data.  */
	while (size > 0)
	{
	    int status;
	    size_t nread;
	    char *data;

	    status = buf_read_data (buf_from_net, size, &data, &nread);
	    if (status != 0)
		return;
	    size -= nread;
	}
	free (mode_text);
	return;
    }

    if (
# ifdef PROXY_SUPPORT
	!proxy_log &&
# endif /* PROXY_SUPPORT */
	outside_dir (arg))
    {
	free (mode_text);
	return;
    }

    receive_file (size,
# ifdef PROXY_SUPPORT
	          proxy_log ? DEVNULL :
# endif /* PROXY_SUPPORT */
			      arg,
		  gzipped);
    if (error_pending ())
    {
	free (mode_text);
	return;
    }

# ifdef PROXY_SUPPORT
    /* We've read all the data that needed to be read if we're still logging
     * for a secondary.  Return.
     */
    if (proxy_log) return;
# endif /* PROXY_SUPPORT */

    if (checkin_time_valid)
    {
	struct utimbuf t;

	memset (&t, 0, sizeof (t));
	t.modtime = t.actime = checkin_time;
	if (utime (arg, &t) < 0)
	{
	    int save_errno = errno;
	    if (alloc_pending (80 + strlen (arg)))
		sprintf (pending_error_text, "E cannot utime %s", arg);
	    pending_error = save_errno;
	    free (mode_text);
	    return;
	}
	checkin_time_valid = 0;
    }

    {
	int status = change_mode (arg, mode_text, 0);
	free (mode_text);
	if (status)
	{
	    if (alloc_pending (40 + strlen (arg)))
		sprintf (pending_error_text,
			 "E cannot change mode for %s", arg);
	    pending_error = status;
	    return;
	}
    }

    /* Make sure that the Entries indicate the right kopt.  We probably
       could do this even in the non-kopt case and, I think, save a stat()
       call in time_stamp_server.  But for conservatism I'm leaving the
       non-kopt case alone.  */
    if (kopt != NULL)
	serve_is_modified (arg);
}



static void
serve_enable_unchanged (char *arg)
{
# ifdef PROXY_SUPPORT
    /* Might as well skip this since this function does nothing anyhow.  If
     * it did do anything and could generate errors, then the line below would
     * be necessary since this can be processed before a `Root' request.
     *
     *     if (reprocessing) return;
     */
# endif /* PROXY_SUPPORT */
}



static void
serve_unchanged (char *arg)
{
    struct an_entry *p;
    char *name;
    char *cp;
    char *timefield;

    if (error_pending ()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       ) return;

    if (outside_dir (arg))
	return;

    /* Rewrite entries file to have `=' in timestamp field.  */
    for (p = entries; p != NULL; p = p->next)
    {
	name = p->entry + 1;
	cp = strchr (name, '/');
	if (cp != NULL
	    && strlen (arg) == cp - name
	    && strncmp (arg, name, cp - name) == 0)
	{
	    if (!(timefield = strchr (cp + 1, '/')) || *++timefield == '\0')
	    {
		/* We didn't find the record separator or it is followed by
		 * the end of the string, so just exit.
		 */
		if (alloc_pending (80))
		    sprintf (pending_error_text,
		             "E Malformed Entry encountered.");
		return;
	    }
	    /* If the time field is not currently empty, then one of
	     * serve_modified, serve_is_modified, & serve_unchanged were
	     * already called for this file.  We would like to ignore the
	     * reinvocation silently or, better yet, exit with an error
	     * message, but we just avoid the copy-forward and overwrite the
	     * value from the last invocation instead.  See the comment below
	     * for more.
	     */
	    if (*timefield == '/')
	    {
		/* Copy forward one character.  Space was allocated for this
		 * already in serve_entry().  */
		cp = timefield + strlen (timefield);
		cp[1] = '\0';
		while (cp > timefield)
		{
		    *cp = cp[-1];
		    --cp;
		}

		/* *timefield == '/';  */
	    }
	    if (*timefield != '+')
	    {
		/* '+' is a conflict marker and we don't want to mess with it
		 * until Version_TS catches it.
		 */
		if (timefield[1] != '/')
		{
		    /* Obliterate anything else in TIMEFIELD.  This is again to
		     * support the broken CVSNT clients mentioned below, in
		     * conjunction with strict timestamp string boundry
		     * checking in time_stamp_server() from vers_ts.c &
		     * file_has_conflict() from subr.c, since the broken
		     * clients used to send malformed timestamp fields in the
		     * Entry request that they then depended on the subsequent
		     * Unchanged request to overwrite.
		     */
		    char *d = timefield + 1;
		    if ((cp = strchr (d, '/')))
		    {
			while (*cp)
			{
			    *d++ = *cp++;
			}
			*d = '\0';
		    }
		}
		/* If *TIMEFIELD wasn't '/', we assume that it was because of
		 * multiple calls to Is-modified & Unchanged by the client and
		 * just overwrite the value from the last call.  Technically,
		 * we should probably either ignore calls after the first or
		 * send the client an error, since the client/server protocol
		 * specification specifies that only one call to either
		 * Is-Modified or Unchanged is allowed, but broken versions of
		 * CVSNT (at least 2.0.34 - 2.0.41, reported fixed in 2.0.41a)
		 * and the WinCVS & TortoiseCVS clients which depend on those
		 * broken versions of CVSNT (WinCVS 1.3 & at least one
		 * TortoiseCVS release) rely on this behavior.
		 */
		*timefield = '=';
	    }
	    break;
	}
    }
}



static void
serve_entry (char *arg)
{
    struct an_entry *p;
    char *cp;
    int i = 0;

    if (error_pending()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       ) return;

    /* Verify that the entry is well-formed.  This can avoid problems later.
     * At the moment we only check that the Entry contains five slashes in
     * approximately the correct locations since some of the code makes
     * assumptions about this.
     */
    cp = arg;
    if (*cp == 'D') cp++;
    while (i++ < 5)
    {
	if (!cp || *cp != '/')
	{
	    if (alloc_pending (80))
		sprintf (pending_error_text,
			 "E protocol error: Malformed Entry");
	    return;
	}
	cp = strchr (cp + 1, '/');
    }

    p = xmalloc (sizeof (struct an_entry));
    if (p == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    /* Leave space for serve_unchanged to write '=' if it wants.  */
    cp = xmalloc (strlen (arg) + 2);
    if (cp == NULL)
    {
	free (p);
	pending_error = ENOMEM;
	return;
    }
    strcpy (cp, arg);
    p->next = entries;
    p->entry = cp;
    entries = p;
}



static void
serve_kopt (char *arg)
{
    if (error_pending ()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       )
	return;

    if (kopt != NULL)
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text,
		     "E protocol error: duplicate Kopt request: %s", arg);
	return;
    }

    /* Do some sanity checks.  In particular, that it is not too long.
       This lets the rest of the code not worry so much about buffer
       overrun attacks.  Probably should call RCS_check_kflag here,
       but that would mean changing RCS_check_kflag to handle errors
       other than via exit(), fprintf(), and such.  */
    if (strlen (arg) > 10)
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text,
		     "E protocol error: invalid Kopt request: %s", arg);
	return;
    }

    kopt = xmalloc (strlen (arg) + 1);
    if (kopt == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    strcpy (kopt, arg);
}



static void
serve_checkin_time (char *arg)
{
    struct timespec t;

    if (error_pending ()
# ifdef PROXY_SUPPORT
	|| proxy_log
# endif /* PROXY_SUPPORT */
       )
	return;

    if (checkin_time_valid)
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text,
		     "E protocol error: duplicate Checkin-time request: %s",
		     arg);
	return;
    }

    if (!get_date (&t, arg, NULL))
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text, "E cannot parse date %s", arg);
	return;
    }

    /* Truncate any nanoseconds returned by get_date().  */
    checkin_time = t.tv_sec;
    checkin_time_valid = 1;
}



static void
server_write_entries (void)
{
    FILE *f;
    struct an_entry *p;
    struct an_entry *q;

    if (entries == NULL)
	return;

    f = NULL;
    /* Note that we free all the entries regardless of errors.  */
    if (!error_pending ())
    {
	/* We open in append mode because we don't want to clobber an
	   existing Entries file.  If we are checking out a module
	   which explicitly lists more than one file in a particular
	   directory, then we will wind up calling
	   server_write_entries for each such file.  */
	f = CVS_FOPEN (CVSADM_ENT, "a");
	if (f == NULL)
	{
	    int save_errno = errno;
	    if (alloc_pending (80 + strlen (CVSADM_ENT)))
		sprintf (pending_error_text, "E cannot open %s", CVSADM_ENT);
	    pending_error = save_errno;
	}
    }
    for (p = entries; p != NULL;)
    {
	if (!error_pending ())
	{
	    if (fprintf (f, "%s\n", p->entry) < 0)
	    {
		int save_errno = errno;
		if (alloc_pending (80 + strlen(CVSADM_ENT)))
		    sprintf (pending_error_text,
			     "E cannot write to %s", CVSADM_ENT);
		pending_error = save_errno;
	    }
	}
	free (p->entry);
	q = p->next;
	free (p);
	p = q;
    }
    entries = NULL;
    if (f != NULL && fclose (f) == EOF && !error_pending ())
    {
	int save_errno = errno;
	if (alloc_pending (80 + strlen (CVSADM_ENT)))
	    sprintf (pending_error_text, "E cannot close %s", CVSADM_ENT);
	pending_error = save_errno;
    }
}



# ifdef PROXY_SUPPORT
/*
 * callback proc to run a script when admin finishes.
 */
static int
prepost_proxy_proc (const char *repository, const char *filter, void *closure)
{
    char *cmdline;
    bool *pre = closure;

    /* %c = cvs_cmd_name
     * %p = shortrepos
     * %r = repository
     */
    TRACE (TRACE_FUNCTION, "prepost_proxy_proc (%s, %s, %s)", repository,
	   filter, *pre ? "pre" : "post");

    /*
     * Cast any NULL arguments as appropriate pointers as this is an
     * stdarg function and we need to be certain the caller gets what
     * is expected.
     */
    cmdline = format_cmdline (
# ifdef SUPPORT_OLD_INFO_FMT_STRINGS
	                      0, ".",
# endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
	                      filter,
	                      "c", "s", cvs_cmd_name,
	                      "R", "s", referrer ? referrer->original : "NONE",
	                      "p", "s", ".",
	                      "r", "s", current_parsed_root->directory,
	                      "P", "s", config->PrimaryServer->original,
	                      (char *) NULL);

    if (!cmdline || !strlen (cmdline))
    {
	if (cmdline) free (cmdline);
	if (*pre)
	    error (0, 0, "preadmin proc resolved to the empty string!");
	else
	    error (0, 0, "postadmin proc resolved to the empty string!");
	return 1;
    }

    run_setup (cmdline);

    free (cmdline);

    /* FIXME - read the comment in verifymsg_proc() about why we use abs()
     * below() and shouldn't.
     */
    return abs (run_exec (RUN_TTY, RUN_TTY, RUN_TTY,
			  RUN_NORMAL | RUN_SIGIGNORE));
}



/* Become a secondary write proxy to a master server.
 *
 * This function opens the connection to the primary, dumps the secondary log
 * to the primary, then reads data from any available connection and writes it
 * to its partner:
 *
 *   buf_from_net -> buf_to_primary
 *   buf_from_primary -> buf_to_net
 *
 * When all "from" connections have sent EOF and all data has been sent to
 * "to" connections, this function closes the "to" pipes and returns.
 */
static void
become_proxy (void)
{
    struct buffer *buf_to_primary;
    struct buffer *buf_from_primary;

    /* Close the client log and open it for read.  */
    struct buffer *buf_clientlog = log_buffer_rewind (proxy_log_out);
    int status, to_primary_fd, from_primary_fd, to_net_fd, from_net_fd;

    /* Call presecondary script.  */
    bool pre = true;

	    char *data;
	    size_t thispass, got;
	    int s;
	    char *newdata;

    Parse_Info (CVSROOTADM_PREPROXY, current_parsed_root->directory,
		prepost_proxy_proc, PIOPT_ALL, &pre);

    /* Open connection to primary server.  */
    open_connection_to_server (config->PrimaryServer, &buf_to_primary,
                               &buf_from_primary);
    setup_logfiles ("CVS_SECONDARY_LOG", &buf_to_primary, &buf_from_primary);
    if ((status = set_nonblock (buf_from_primary)))
	error (1, status, "failed to set nonblocking io from primary");
    if ((status = set_nonblock (buf_from_net)))
	error (1, status, "failed to set nonblocking io from client");
    if ((status = set_nonblock (buf_to_primary)))
	error (1, status, "failed to set nonblocking io to primary");
    if ((status = set_nonblock (buf_to_net)))
	error (1, status, "failed to set nonblocking io to client");

    to_primary_fd = buf_get_fd (buf_to_primary);
    from_primary_fd = buf_get_fd (buf_from_primary);
    to_net_fd = buf_get_fd (buf_to_net);
    assert (to_primary_fd >= 0 && from_primary_fd >= 0 && to_net_fd >= 0);

    /* Close the client log and open it for read.  */
    rewind_buf_from_net ();

    while (from_primary_fd >= 0 || to_primary_fd >= 0)
    {
	fd_set readfds, writefds;
	int status, numfds = -1;
	struct timeval *timeout_ptr;
	struct timeval timeout;
	size_t toread;

	FD_ZERO (&readfds);
	FD_ZERO (&writefds);

	/* The fd for a multi-source buffer can change with any read.  */
	from_net_fd = buf_from_net ? buf_get_fd (buf_from_net) : -1;

	if ((buf_from_net && !buf_empty_p (buf_from_net))
	    || (buf_from_primary && !buf_empty_p (buf_from_primary)))
	{
	    /* There is data pending so don't block if we don't find any new
	     * data on the fds.
	     */
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 0;
	    timeout_ptr = &timeout;
	}
	else
	    /* block indefinately */
	    timeout_ptr = NULL;

	/* Set writefds if data is pending.  */
	if (to_net_fd >= 0 && !buf_empty_p (buf_to_net))
	{
	    FD_SET (to_net_fd, &writefds);
	    numfds = MAX (numfds, to_net_fd);
	}
	if (to_primary_fd >= 0 && !buf_empty_p (buf_to_primary))
	{
	    FD_SET (to_primary_fd, &writefds);
	    numfds = MAX (numfds, to_primary_fd);
	}

	/* Set readfds if descriptors are still open.  */
	if (from_net_fd >= 0)
	{
	    FD_SET (from_net_fd, &readfds);
	    numfds = MAX (numfds, from_net_fd);
	}
	if (from_primary_fd >= 0)
	{
	    FD_SET (from_primary_fd, &readfds);
	    numfds = MAX (numfds, from_primary_fd);
	}

	/* NUMFDS needs to be the highest descriptor + 1 according to the
	 * select spec.
	 */
	numfds++;

	do {
	    /* This used to select on exceptions too, but as far
	       as I know there was never any reason to do that and
	       SCO doesn't let you select on exceptions on pipes.  */
	    numfds = select (numfds, &readfds, &writefds,
			     NULL, timeout_ptr);
	    if (numfds < 0 && errno != EINTR)
	    {
		/* Sending an error to the client, possibly in the middle of a
		 * separate protocol message, will likely not mean much to the
		 * client, but it's better than nothing, I guess.
		 */
		buf_output0 (buf_to_net, "E select failed\n");
		print_error (errno);
		exit (EXIT_FAILURE);
	    }
	} while (numfds < 0);

	if (numfds == 0)
	{
	    FD_ZERO (&readfds);
	    FD_ZERO (&writefds);
	}

	if (to_net_fd >= 0 && FD_ISSET (to_net_fd, &writefds))
	{
	    /* What should we do with errors?  syslog() them?  */
	    buf_send_output (buf_to_net);
	    buf_flush (buf_to_net, false);
	}

	status = 0;
	if (from_net_fd >= 0 && (FD_ISSET (from_net_fd, &readfds)))
	    status = buf_input_data (buf_from_net, NULL);

	if (buf_from_net && !buf_empty_p (buf_from_net))
	{
	    if (buf_to_primary)
		buf_append_buffer (buf_to_primary, buf_from_net);
	    else
		/* (Sys?)log this?  */;
		
	}

	if (status == -1 /* EOF */)
	{
	    SIG_beginCrSect();
	    /* Need only to shut this down and set to NULL, really, in
	     * crit sec, to ensure no double-dispose and to make sure
	     * network pipes are closed as properly as possible, but I
	     * don't see much optimization potential in saving values and
	     * postponing the free.
	     */
	    buf_shutdown (buf_from_net);
	    buf_free (buf_from_net);
	    buf_from_net = NULL;
	    /* So buf_to_primary will be closed at the end of this loop.  */
	    from_net_fd = -1;
	    SIG_endCrSect();
	}
	else if (status > 0 /* ERRNO */)
	{
	    buf_output0 (buf_to_net,
			 "E buf_input_data failed reading from client\n");
	    print_error (status);
	    exit (EXIT_FAILURE);
	}

	if (to_primary_fd >= 0 && FD_ISSET (to_primary_fd, &writefds))
	{
	    /* What should we do with errors?  syslog() them?  */
	    buf_send_output (buf_to_primary);
	    buf_flush (buf_to_primary, false);
	}

	status = 0;
	if (from_primary_fd >= 0 && FD_ISSET (from_primary_fd, &readfds))
	    status = buf_input_data (buf_from_primary, &toread);

	/* Avoid resending data from the server which we already sent to the
	 * client.  Otherwise clients get really confused.
	 */
	if (buf_clientlog
	    && buf_from_primary && !buf_empty_p (buf_from_primary))
	{
	    /* Dispose of data we already sent to the client.  */
	    while (buf_clientlog && toread > 0)
	    {
		s = buf_read_data (buf_clientlog, toread, &data, &got);
		if (s == -2)
		    error (1, ENOMEM, "Failed to read data.");
		if (s == -1)
		{
		    buf_shutdown (buf_clientlog);
		    buf_clientlog = NULL;
		}
		else if (s)
		    error (1, s, "Error reading writeproxy log.");
		else
		{
		    thispass = got;
		    while (thispass > 0)
		    {
			/* No need to check for errors here since we know we
			 * won't read more than buf_input read into
			 * BUF_FROM_PRIMARY (see how TOREAD is set above).
			 */
			buf_read_data (buf_from_primary, thispass, &newdata,
				       &got);
			/* Verify that we are throwing away what we think we
			 * are.
			 *
			 * It is valid to assume that the secondary and primary
			 * are closely enough in sync that this portion of the
			 * communication will be in sync beacuse if they were
			 * not, then the secondary might provide a
			 * valid-request string to the client which contained a
			 * request that the primary didn't support.  If the
			 * client later used the request, the primary server
			 * would exit anyhow.
			 *
			 * FIXME?
			 * An alternative approach might be to make sure that
			 * the secondary provides the same string as the
			 * primary regardless, for purposes like pointing a
			 * secondary at an unwitting primary, in which case it
			 * might be useful to have some way to override the
			 * valid-requests string on a secondary, but it seems
			 * much easier to simply sync the versions, at the
			 * moment.
			 */
			if (memcmp (data, newdata, got))
			    error (1, 0, "Secondary out of sync with primary!");
			data += got;
			thispass -= got;
		    }
		    toread -= got;
		}
	    }
	}

	if (buf_from_primary && !buf_empty_p (buf_from_primary))
	{
	    if (buf_to_net)
		buf_append_buffer (buf_to_net, buf_from_primary);
	    else
		/* (Sys?)log this?  */;
		
	}

	if (status == -1 /* EOF */)
	{
	    buf_shutdown (buf_from_primary);
	    buf_from_primary = NULL;
	    from_primary_fd = -1;
	}
	else if (status > 0 /* ERRNO */)
	{
	    buf_output0 (buf_to_net,
			 "E buf_input_data failed reading from primary\n");
	    print_error (status);
	    exit (EXIT_FAILURE);
	}

	/* If our "source pipe" is closed and all data has been sent, avoid
	 * selecting it for writability, but don't actually close the buffer in
	 * case other routines want to use it later.  The buffer will be closed
	 * in server_cleanup ().
	 */
	if (from_primary_fd < 0
	    && buf_to_net && buf_empty_p (buf_to_net))
	    to_net_fd = -1;

	if (buf_to_primary
	    && (/* Assume that there is no further reason to keep the buffer to
	         * the primary open if we can no longer read its responses.
	         */
	        (from_primary_fd < 0 && buf_to_primary)
	        /* Also close buf_to_primary when it becomes impossible to find
	         * more data to send to it.  We don't close buf_from_primary
	         * yet since there may be data pending or the primary may react
	         * to the EOF on its input pipe.
	         */
	        || (from_net_fd < 0 && buf_empty_p (buf_to_primary))))
	{
	    buf_shutdown (buf_to_primary);
	    buf_free (buf_to_primary);
	    buf_to_primary = NULL;

	    /* Setting the fd < 0 with from_primary_fd already < 0 will cause
	     * an escape from this while loop.
	     */
	    to_primary_fd = -1;
	}
    }

    /* Call postsecondary script.  */
    pre = false;
    Parse_Info (CVSROOTADM_POSTPROXY, current_parsed_root->directory,
		prepost_proxy_proc, PIOPT_ALL, &pre);
}
# endif /* PROXY_SUPPORT */



struct notify_note {
    /* Directory in which this notification happens.  xmalloc'd*/
    char *dir;

    /* xmalloc'd.  */
    char *update_dir;

    /* xmalloc'd.  */
    char *filename;

    /* The following three all in one xmalloc'd block, pointed to by TYPE.
       Each '\0' terminated.  */
    /* "E" or "U".  */
    char *type;
    /* time+host+dir */
    char *val;
    char *watches;

    struct notify_note *next;
};

static struct notify_note *notify_list;
/* Used while building list, to point to the last node that already exists.  */
static struct notify_note *last_node;

static void
serve_notify (char *arg)
{
    struct notify_note *new = NULL;
    char *data = NULL;
    int status;

    if (error_pending ()) return;

    if (isProxyServer())
    {
# ifdef PROXY_SUPPORT
	if (!proxy_log)
	{
# endif /* PROXY_SUPPORT */
	    if (alloc_pending (160) + strlen (program_name))
		sprintf (pending_error_text, 
"E This CVS server does not support disconnected `%s edit'.  For now, remove all `%s' files in your workspace and try your command again.",
			 program_name, CVSADM_NOTIFY);
	return;
# ifdef PROXY_SUPPORT
	}
	else
	{
	    /* This is effectively a write command, so run it on the primary.  */
	    become_proxy ();
	    exit (EXIT_SUCCESS);
	}
# endif /* PROXY_SUPPORT */
    }

    if (outside_dir (arg))
	return;

    if (gDirname == NULL)
	goto error;

    new = xmalloc (sizeof (struct notify_note));
    if (new == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    new->dir = xmalloc (strlen (gDirname) + 1);
    new->update_dir = xmalloc (strlen (gupdate_dir) + 1);
    new->filename = xmalloc (strlen (arg) + 1);
    if (new->dir == NULL || new->update_dir == NULL || new->filename == NULL)
    {
	pending_error = ENOMEM;
	if (new->dir != NULL)
	    free (new->dir);
	free (new);
	return;
    }
    strcpy (new->dir, gDirname);
    strcpy (new->update_dir, gupdate_dir);
    strcpy (new->filename, arg);

    status = buf_read_line (buf_from_net, &data, NULL);
    if (status != 0)
    {
	if (status == -2)
	    pending_error = ENOMEM;
	else
	{
	    pending_error_text = xmalloc (80 + strlen (arg));
	    if (pending_error_text == NULL)
		pending_error = ENOMEM;
	    else
	    {
		if (status == -1)
		    sprintf (pending_error_text,
			     "E end of file reading notification for %s", arg);
		else
		{
		    sprintf (pending_error_text,
			     "E error reading notification for %s", arg);
		    pending_error = status;
		}
	    }
	}
	free (new->filename);
	free (new->dir);
	free (new);
    }
    else
    {
	char *cp;

	if (!data[0])
	    goto error;

	if (strchr (data, '+'))
	    goto error;

	new->type = data;
	if (data[1] != '\t')
	    goto error;
	data[1] = '\0';
	cp = data + 2;
	new->val = cp;
	cp = strchr (cp, '\t');
	if (cp == NULL)
	    goto error;
	*cp++ = '+';
	cp = strchr (cp, '\t');
	if (cp == NULL)
	    goto error;
	*cp++ = '+';
	cp = strchr (cp, '\t');
	if (cp == NULL)
	    goto error;
	*cp++ = '\0';
	new->watches = cp;
	/* If there is another tab, ignore everything after it,
	   for future expansion.  */
	cp = strchr (cp, '\t');
	if (cp != NULL)
	    *cp = '\0';

	new->next = NULL;

	if (last_node == NULL)
	    notify_list = new;
	else
	    last_node->next = new;
	last_node = new;
    }
    return;
  error:
    pending_error = 0;
    if (alloc_pending (80))
	strcpy (pending_error_text,
		"E Protocol error; misformed Notify request");
    if (data != NULL)
	free (data);
    if (new != NULL)
    {
	free (new->filename);
	free (new->update_dir);
	free (new->dir);
	free (new);
    }
    return;
}



static void
serve_hostname (char *arg)
{
    free (hostname);
    hostname = xstrdup (arg);
    return;
}



static void
serve_localdir (char *arg)
{
    if (CurDir) free (CurDir);
    CurDir = xstrdup (arg);
}



/* Process all the Notify requests that we have stored up.  Returns 0
   if successful, if not prints error message (via error()) and
   returns negative value.  */
static int
server_notify (void)
{
    struct notify_note *p;
    char *repos;

    TRACE (TRACE_FUNCTION, "server_notify()");

    while (notify_list != NULL)
    {
	if (CVS_CHDIR (notify_list->dir) < 0)
	{
	    error (0, errno, "cannot change to %s", notify_list->dir);
	    return -1;
	}
	repos = Name_Repository (NULL, NULL);

	lock_dir_for_write (repos);

	fileattr_startdir (repos);

	notify_do (*notify_list->type, notify_list->filename,
		   notify_list->update_dir, getcaller(), notify_list->val,
		   notify_list->watches, repos);

	buf_output0 (buf_to_net, "Notified ");
	{
	    char *dir = notify_list->dir + strlen (server_temp_dir) + 1;
	    if (dir[0] == '\0')
		buf_append_char (buf_to_net, '.');
	    else
		buf_output0 (buf_to_net, dir);
	    buf_append_char (buf_to_net, '/');
	    buf_append_char (buf_to_net, '\n');
	}
	buf_output0 (buf_to_net, repos);
	buf_append_char (buf_to_net, '/');
	buf_output0 (buf_to_net, notify_list->filename);
	buf_append_char (buf_to_net, '\n');
	free (repos);

	p = notify_list->next;
	free (notify_list->filename);
	free (notify_list->dir);
	free (notify_list->type);
	free (notify_list);
	notify_list = p;

	fileattr_write ();
	fileattr_free ();

	Lock_Cleanup ();
    }

    last_node = NULL;

    /* The code used to call fflush (stdout) here, but that is no
       longer necessary.  The data is now buffered in buf_to_net,
       which will be flushed by the caller, do_cvs_command.  */

    return 0;
}



/* This request is processed in all passes since requests which must
 * sometimes be processed before it is known whether we are running as a
 * secondary or not, for instance the `expand-modules' request, sometimes use
 * the `Arguments'.
 */
static void
serve_argument (char *arg)
{
    char *p;

    if (error_pending()) return;

    if (argument_count >= 10000)
    {
	if (alloc_pending (80))
	    sprintf (pending_error_text, 
		     "E Protocol error: too many arguments");
	return;
    }

    if (argument_vector_size <= argument_count)
    {
	argument_vector_size *= 2;
	argument_vector = xnrealloc (argument_vector,
				     argument_vector_size, sizeof (char *));
	if (argument_vector == NULL)
	{
	    pending_error = ENOMEM;
	    return;
	}
    }
    p = xmalloc (strlen (arg) + 1);
    if (p == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    strcpy (p, arg);
    argument_vector[argument_count++] = p;
}



/* For secondary servers, this is handled in all passes, as is the `Argument'
 * request, and for the same reasons.
 */
static void
serve_argumentx (char *arg)
{
    char *p;

    if (error_pending()) return;
    
    if (argument_count <= 1) 
    {
	if (alloc_pending (80))
	    sprintf (pending_error_text,
"E Protocol error: called argumentx without prior call to argument");
	return;
    }

    p = argument_vector[argument_count - 1];
    p = xrealloc (p, strlen (p) + 1 + strlen (arg) + 1);
    if (p == NULL)
    {
	pending_error = ENOMEM;
	return;
    }
    strcat (p, "\n");
    strcat (p, arg);
    argument_vector[argument_count - 1] = p;
}



static void
serve_global_option (char *arg)
{
# ifdef PROXY_SUPPORT
    /* This can generate error messages and termination before `Root' requests,
     * so it must be dealt with in the first pass.
     */ 
    if (reprocessing) return;
# endif /* PROXY_SUPPORT */

    if (arg[0] != '-' || arg[1] == '\0' || arg[2] != '\0')
    {
    error_return:
	if (alloc_pending (strlen (arg) + 80))
	    sprintf (pending_error_text,
		     "E Protocol error: bad global option %s",
		     arg);
	return;
    }
    switch (arg[1])
    {
	case 'l':
	    error(0, 0, "WARNING: global `-l' option ignored.");
	    break;
	case 'n':
	    noexec = 1;
	    logoff = 1;
	    break;
	case 'u':
	    nolock = 1;
	    break;
	case 'q':
	    quiet = 1;
	    break;
	case 'r':
	    cvswrite = 0;
	    break;
	case 'Q':
	    really_quiet = 1;
	    break;
	case 't':
	    trace++;
	    break;
	default:
	    goto error_return;
    }
}



/* This needs to be processed before Root requests, so we allow it to be
 * be processed before knowing whether we are running as a secondary server
 * to allow `noop' and `Root' requests to generate errors as before.
 */
static void
serve_set (char *arg)
{
# ifdef PROXY_SUPPORT
    if (reprocessing) return;
# endif /* PROXY_SUPPORT */

    /* FIXME: This sends errors immediately (I think); they should be
       put into pending_error.  */
    variable_set (arg);
}

# ifdef ENCRYPTION

#   ifdef HAVE_KERBEROS

static void
serve_kerberos_encrypt( char *arg )
{
#     ifdef PROXY_SUPPORT
    assert (!proxy_log);
#     endif /* PROXY_SUPPORT */

    /* All future communication with the client will be encrypted.  */

    buf_to_net = krb_encrypt_buffer_initialize (buf_to_net, 0, sched,
						kblock,
						buf_to_net->memory_error);
    buf_from_net = krb_encrypt_buffer_initialize (buf_from_net, 1, sched,
						  kblock,
						  buf_from_net->memory_error);
}

#   endif /* HAVE_KERBEROS */

#   ifdef HAVE_GSSAPI

static void
serve_gssapi_encrypt( char *arg )
{
#     ifdef PROXY_SUPPORT
    assert (!proxy_log);
#     endif /* PROXY_SUPPORT */

    if (cvs_gssapi_wrapping)
    {
	/* We're already using a gssapi_wrap buffer for stream
	   authentication.  Flush everything we've output so far, and
	   turn on encryption for future data.  On the input side, we
	   should only have unwrapped as far as the Gssapi-encrypt
	   command, so future unwrapping will become encrypted.  */
	buf_flush (buf_to_net, 1);
	cvs_gssapi_encrypt = 1;
	return;
    }

    /* All future communication with the client will be encrypted.  */

    cvs_gssapi_encrypt = 1;

    buf_to_net = cvs_gssapi_wrap_buffer_initialize (buf_to_net, 0,
						    gcontext,
						    buf_to_net->memory_error);
    buf_from_net = cvs_gssapi_wrap_buffer_initialize (buf_from_net, 1,
						      gcontext,
						      buf_from_net->memory_error);

    cvs_gssapi_wrapping = 1;
}

#   endif /* HAVE_GSSAPI */

# endif /* ENCRYPTION */

# ifdef HAVE_GSSAPI

static void
serve_gssapi_authenticate (char *arg)
{
#   ifdef PROXY_SUPPORT
    assert (!proxy_log);
#   endif /* PROXY_SUPPORT */

    if (cvs_gssapi_wrapping)
    {
	/* We're already using a gssapi_wrap buffer for encryption.
	   That includes authentication, so we don't have to do
	   anything further.  */
	return;
    }

    buf_to_net = cvs_gssapi_wrap_buffer_initialize (buf_to_net, 0,
						    gcontext,
						    buf_to_net->memory_error);
    buf_from_net = cvs_gssapi_wrap_buffer_initialize (buf_from_net, 1,
						      gcontext,
						      buf_from_net->memory_error);

    cvs_gssapi_wrapping = 1;
}

# endif /* HAVE_GSSAPI */



# ifdef SERVER_FLOWCONTROL
/* The maximum we'll queue to the remote client before blocking.  */
#   ifndef SERVER_HI_WATER
#     define SERVER_HI_WATER (2 * 1024 * 1024)
#   endif /* SERVER_HI_WATER */
/* When the buffer drops to this, we restart the child */
#   ifndef SERVER_LO_WATER
#     define SERVER_LO_WATER (1 * 1024 * 1024)
#   endif /* SERVER_LO_WATER */
# endif /* SERVER_FLOWCONTROL */



static void
serve_questionable (char *arg)
{
    static int initted;

# ifdef PROXY_SUPPORT
    if (proxy_log) return;
# endif /* PROXY_SUPPORT */

    if (error_pending ()) return;

    if (!initted)
    {
	/* Pick up ignores from CVSROOTADM_IGNORE, $HOME/.cvsignore on server,
	   and CVSIGNORE on server.  */
	ign_setup ();
	initted = 1;
    }

    if (gDirname == NULL)
    {
	if (alloc_pending (80))
	    sprintf (pending_error_text,
"E Protocol error: `Directory' missing");
	return;
    }

    if (outside_dir (arg))
	return;

    if (!ign_name (arg))
    {
	char *update_dir;

	buf_output (buf_to_net, "M ? ", 4);
	update_dir = gDirname + strlen (server_temp_dir) + 1;
	if (!(update_dir[0] == '.' && update_dir[1] == '\0'))
	{
	    buf_output0 (buf_to_net, update_dir);
	    buf_output (buf_to_net, "/", 1);
	}
	buf_output0 (buf_to_net, arg);
	buf_output (buf_to_net, "\n", 1);
    }
}



static struct buffer *protocol = NULL;

/* This is the output which we are saving up to send to the server, in the
   child process.  We will push it through, via the `protocol' buffer, when
   we have a complete line.  */
static struct buffer *saved_output;

/* Likewise, but stuff which will go to stderr.  */
static struct buffer *saved_outerr;



static void
protocol_memory_error (struct buffer *buf)
{
    error (1, ENOMEM, "Virtual memory exhausted");
}



/* If command is valid, return 1.
 * Else if command is invalid and croak_on_invalid is set, then die.
 * Else just return 0 to indicate that command is invalid.
 */
static bool
check_command_valid_p (char *cmd_name)
{
    /* Right now, only pserver notices invalid commands -- namely,
     * write attempts by a read-only user.  Therefore, if CVS_Username
     * is not set, this just returns 1, because CVS_Username unset
     * means pserver is not active.
     */
# ifdef AUTH_SERVER_SUPPORT
    if (CVS_Username == NULL)
	return true;

    if (lookup_command_attribute (cmd_name) & CVS_CMD_MODIFIES_REPOSITORY)
    {
	/* This command has the potential to modify the repository, so
	 * we check if the user have permission to do that.
	 *
	 * (Only relevant for remote users -- local users can do
	 * whatever normal Unix file permissions allow them to do.)
	 *
	 * The decision method:
	 *
	 *    If $CVSROOT/CVSADMROOT_READERS exists and user is listed
	 *    in it, then read-only access for user.
	 *
	 *    Or if $CVSROOT/CVSADMROOT_WRITERS exists and user NOT
	 *    listed in it, then also read-only access for user.
	 *
	 *    Else read-write access for user.
	 */

	 char *linebuf = NULL;
	 int num_red = 0;
	 size_t linebuf_len = 0;
	 char *fname;
	 size_t flen;
	 FILE *fp;
	 int found_it = 0;

	 /* else */
	 flen = strlen (current_parsed_root->directory)
		+ strlen (CVSROOTADM)
		+ strlen (CVSROOTADM_READERS)
		+ 3;

	 fname = xmalloc (flen);
	 (void) sprintf (fname, "%s/%s/%s", current_parsed_root->directory,
			CVSROOTADM, CVSROOTADM_READERS);

	 fp = fopen (fname, "r");

	 if (fp == NULL)
	 {
	     if (!existence_error (errno))
	     {
		 /* Need to deny access, so that attackers can't fool
		    us with some sort of denial of service attack.  */
		 error (0, errno, "cannot open %s", fname);
		 free (fname);
		 return false;
	     }
	 }
         else  /* successfully opened readers file */
         {
             while ((num_red = getline (&linebuf, &linebuf_len, fp)) >= 0)
             {
                 /* Hmmm, is it worth importing my own readline
                    library into CVS?  It takes care of chopping
                    leading and trailing whitespace, "#" comments, and
                    newlines automatically when so requested.  Would
                    save some code here...  -kff */

                 /* Chop newline by hand, for strcmp()'s sake. */
                 if (num_red > 0 && linebuf[num_red - 1] == '\n')
                     linebuf[num_red - 1] = '\0';

                 if (strcmp (linebuf, CVS_Username) == 0)
                     goto handle_invalid;
             }
	     if (num_red < 0 && !feof (fp))
		 error (0, errno, "cannot read %s", fname);

	     /* If not listed specifically as a reader, then this user
		has write access by default unless writers are also
		specified in a file . */
	     if (fclose (fp) < 0)
		 error (0, errno, "cannot close %s", fname);
	 }
	 free (fname);

	 /* Now check the writers file.  */

	 flen = strlen (current_parsed_root->directory)
		+ strlen (CVSROOTADM)
		+ strlen (CVSROOTADM_WRITERS)
		+ 3;

	 fname = xmalloc (flen);
	 (void) sprintf (fname, "%s/%s/%s", current_parsed_root->directory,
			CVSROOTADM, CVSROOTADM_WRITERS);

	 fp = fopen (fname, "r");

	 if (fp == NULL)
	 {
	     if (linebuf)
		 free (linebuf);
	     if (existence_error (errno))
	     {
		 /* Writers file does not exist, so everyone is a writer,
		    by default.  */
		 free (fname);
		 return true;
	     }
	     else
	     {
		 /* Need to deny access, so that attackers can't fool
		    us with some sort of denial of service attack.  */
		 error (0, errno, "cannot read %s", fname);
		 free (fname);
		 return false;
	     }
	 }

	 found_it = 0;
	 while ((num_red = getline (&linebuf, &linebuf_len, fp)) >= 0)
	 {
	     /* Chop newline by hand, for strcmp()'s sake. */
	     if (num_red > 0 && linebuf[num_red - 1] == '\n')
		 linebuf[num_red - 1] = '\0';

	     if (strcmp (linebuf, CVS_Username) == 0)
	     {
		 found_it = 1;
		 break;
	     }
	 }
	 if (num_red < 0 && !feof (fp))
	     error (0, errno, "cannot read %s", fname);

	 if (found_it)
	 {
	     if (fclose (fp) < 0)
		 error (0, errno, "cannot close %s", fname);
	     if (linebuf)
		 free (linebuf);
	     free (fname);
             return true;
         }
         else   /* writers file exists, but this user not listed in it */
         {
         handle_invalid:
             if (fclose (fp) < 0)
		 error (0, errno, "cannot close %s", fname);
	     if (linebuf)
		 free (linebuf);
	     free (fname);
	     return false;
	 }
    }
# endif /* AUTH_SERVER_SUPPORT */

    /* If ever reach end of this function, command must be valid. */
    return true;
}



/* Execute COMMAND in a subprocess with the approriate funky things done.  */

static struct fd_set_wrapper { fd_set fds; } command_fds_to_drain;
# ifdef SUNOS_KLUDGE
static int max_command_fd;
# endif

# ifdef SERVER_FLOWCONTROL
static int flowcontrol_pipe[2];
# endif /* SERVER_FLOWCONTROL */



/*
 * Set buffer FD to non-blocking I/O.  Returns 0 for success or errno
 * code.
 */
int
set_nonblock_fd (int fd)
{
# if defined (F_GETFL) && defined (O_NONBLOCK) && defined (F_SETFL)
    int flags;

    flags = fcntl (fd, F_GETFL, 0);
    if (flags < 0)
	return errno;
    if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) < 0)
	return errno;
# endif /* F_GETFL && O_NONBLOCK && F_SETFL */
    return 0;
}



static void
do_cvs_command (char *cmd_name, int (*command) (int, char **))
{
    /*
     * The following file descriptors are set to -1 if that file is not
     * currently open.
     */

    /* Data on these pipes is a series of '\n'-terminated lines.  */
    int stdout_pipe[2];
    int stderr_pipe[2];

    /*
     * Data on this pipe is a series of counted (see buf_send_counted)
     * packets.  Each packet must be processed atomically (i.e. not
     * interleaved with data from stdout_pipe or stderr_pipe).
     */
    int protocol_pipe[2];

    int dev_null_fd = -1;

    int errs;

    TRACE (TRACE_FUNCTION, "do_cvs_command (%s)", cmd_name);

    /* Write proxy logging is always terminated when a command is received.
     * Therefore, we wish to avoid reprocessing the command since that would
     * cause endless recursion.
     */
    if (isProxyServer())
    {
# ifdef PROXY_SUPPORT
	if (reprocessing)
	    /* This must be the second time we've reached this point.
	     * Done reprocessing.
	     */
	    reprocessing = false;
	else
	{
	    if (lookup_command_attribute (cmd_name)
		    & CVS_CMD_MODIFIES_REPOSITORY)
	    {
		become_proxy ();
		exit (EXIT_SUCCESS);
	    }
	    else if (/* serve_co may have called this already and missing logs
		      * should have generated an error in serve_root().
		      */
		     proxy_log)
	    {
		/* Set up the log for reprocessing.  */
		rewind_buf_from_net ();
		/* And return to the main loop in server(), where we will now
		 * find the logged secondary data and reread it.
		 */
		return;
	    }
	}
# else /* !PROXY_SUPPORT */
	if (lookup_command_attribute (cmd_name)
		    & CVS_CMD_MODIFIES_REPOSITORY
	    && alloc_pending (120))
	    sprintf (pending_error_text, 
"E You need a CVS client that supports the `Redirect' response for write requests to this server.");
	return;
# endif /* PROXY_SUPPORT */
    }

    command_pid = -1;
    stdout_pipe[0] = -1;
    stdout_pipe[1] = -1;
    stderr_pipe[0] = -1;
    stderr_pipe[1] = -1;
    protocol_pipe[0] = -1;
    protocol_pipe[1] = -1;

    server_write_entries ();

    if (print_pending_error ())
	goto free_args_and_return;

    /* Global `cvs_cmd_name' is probably "server" right now -- only
       serve_export() sets it to anything else.  So we will use local
       parameter `cmd_name' to determine if this command is valid for
       this user.  */
    if (!check_command_valid_p (cmd_name))
    {
	buf_output0 (buf_to_net, "E ");
	buf_output0 (buf_to_net, program_name);
	buf_output0 (buf_to_net, " [server aborted]: \"");
	buf_output0 (buf_to_net, cmd_name);
	buf_output0 (buf_to_net,
"\" requires write access to the repository\n\
error  \n");
	goto free_args_and_return;
    }
    cvs_cmd_name = cmd_name;

    (void) server_notify ();

    /*
     * We use a child process which actually does the operation.  This
     * is so we can intercept its standard output.  Even if all of CVS
     * were written to go to some special routine instead of writing
     * to stdout or stderr, we would still need to do the same thing
     * for the RCS commands.
     */

    if (pipe (stdout_pipe) < 0)
    {
	buf_output0 (buf_to_net, "E pipe failed\n");
	print_error (errno);
	goto error_exit;
    }
    if (pipe (stderr_pipe) < 0)
    {
	buf_output0 (buf_to_net, "E pipe failed\n");
	print_error (errno);
	goto error_exit;
    }
    if (pipe (protocol_pipe) < 0)
    {
	buf_output0 (buf_to_net, "E pipe failed\n");
	print_error (errno);
	goto error_exit;
    }
# ifdef SERVER_FLOWCONTROL
    if (pipe (flowcontrol_pipe) < 0)
    {
	buf_output0 (buf_to_net, "E pipe failed\n");
	print_error (errno);
	goto error_exit;
    }
    set_nonblock_fd (flowcontrol_pipe[0]);
    set_nonblock_fd (flowcontrol_pipe[1]);
# endif /* SERVER_FLOWCONTROL */

    dev_null_fd = CVS_OPEN (DEVNULL, O_RDONLY);
    if (dev_null_fd < 0)
    {
	buf_output0 (buf_to_net, "E open /dev/null failed\n");
	print_error (errno);
	goto error_exit;
    }

    /* We shouldn't have any partial lines from cvs_output and
       cvs_outerr, but we handle them here in case there is a bug.  */
    /* FIXME: appending a newline, rather than using "MT" as we
       do in the child process, is probably not really a very good
       way to "handle" them.  */
    if (! buf_empty_p (saved_output))
    {
	buf_append_char (saved_output, '\n');
	buf_copy_lines (buf_to_net, saved_output, 'M');
    }
    if (! buf_empty_p (saved_outerr))
    {
	buf_append_char (saved_outerr, '\n');
	buf_copy_lines (buf_to_net, saved_outerr, 'E');
    }

    /* Flush out any pending data.  */
    buf_flush (buf_to_net, 1);

    /* Don't use vfork; we're not going to exec().  */
    command_pid = fork ();
    if (command_pid < 0)
    {
	buf_output0 (buf_to_net, "E fork failed\n");
	print_error (errno);
	goto error_exit;
    }
    if (command_pid == 0)
    {
	int exitstatus;

	/* Since we're in the child, and the parent is going to take
	   care of packaging up our error messages, we can clear this
	   flag.  */
	error_use_protocol = 0;

	protocol = fd_buffer_initialize (protocol_pipe[1], 0, NULL, false,
					 protocol_memory_error);

	/* At this point we should no longer be using buf_to_net and
	   buf_from_net.  Instead, everything should go through
	   protocol.  */
	if (buf_to_net != NULL)
	{
	    buf_free (buf_to_net);
	    buf_to_net = NULL;
	}
	if (buf_from_net != NULL)
	{
	    buf_free (buf_from_net);
	    buf_from_net = NULL;
	}

	/* These were originally set up to use outbuf_memory_error.
	   Since we're now in the child, we should use the simpler
	   protocol_memory_error function.  */
	saved_output->memory_error = protocol_memory_error;
	saved_outerr->memory_error = protocol_memory_error;

	if (dup2 (dev_null_fd, STDIN_FILENO) < 0)
	    error (1, errno, "can't set up pipes");
	if (dup2 (stdout_pipe[1], STDOUT_FILENO) < 0)
	    error (1, errno, "can't set up pipes");
	if (dup2 (stderr_pipe[1], STDERR_FILENO) < 0)
	    error (1, errno, "can't set up pipes");
	close (dev_null_fd);
	close (stdout_pipe[0]);
	close (stdout_pipe[1]);
	close (stderr_pipe[0]);
	close (stderr_pipe[1]);
	close (protocol_pipe[0]);
	close_on_exec (protocol_pipe[1]);
# ifdef SERVER_FLOWCONTROL
	close_on_exec (flowcontrol_pipe[0]);
	close (flowcontrol_pipe[1]);
# endif /* SERVER_FLOWCONTROL */

	/*
	 * Set this in .bashrc if you want to give yourself time to attach
	 * to the subprocess with a debugger.
	 */
	if (getenv ("CVS_SERVER_SLEEP"))
	{
	    int secs = atoi (getenv ("CVS_SERVER_SLEEP"));
	    TRACE (TRACE_DATA, "Sleeping CVS_SERVER_SLEEP (%d) seconds", secs);
	    sleep (secs);
	}
	else
	    TRACE (TRACE_DATA, "CVS_SERVER_SLEEP not set.");

	exitstatus = (*command) (argument_count, argument_vector);

	/* Output any partial lines.  If the client doesn't support
	   "MT", we go ahead and just tack on a newline since the
	   protocol doesn't support anything better.  */
	if (! buf_empty_p (saved_output))
	{
	    buf_output0 (protocol, supported_response ("MT") ? "MT text " : "M ");
	    buf_append_buffer (protocol, saved_output);
	    buf_output (protocol, "\n", 1);
	    buf_send_counted (protocol);
	}
	/* For now we just discard partial lines on stderr.  I suspect
	   that CVS can't write such lines unless there is a bug.  */

	buf_free (protocol);

	/* Close the pipes explicitly in order to send an EOF to the parent,
	 * then wait for the parent to close the flow control pipe.  This
	 * avoids a race condition where a child which dumped more than the
	 * high water mark into the pipes could complete its job and exit,
	 * leaving the parent process to attempt to write a stop byte to the
	 * closed flow control pipe, which earned the parent a SIGPIPE, which
	 * it normally only expects on the network pipe and that causes it to
	 * exit with an error message, rather than the SIGCHILD that it knows
	 * how to handle correctly.
	 */
	/* Let exit() close STDIN - it's from /dev/null anyhow.  */
	fclose (stderr);
	fclose (stdout);
	close (protocol_pipe[1]);
# ifdef SERVER_FLOWCONTROL
	{
	    char junk;
	    ssize_t status;
	    while ((status = read (flowcontrol_pipe[0], &junk, 1)) > 0
	           || (status == -1 && errno == EAGAIN));
	}
	/* FIXME: No point in printing an error message with error(),
	 * as STDERR is already closed, but perhaps this could be syslogged?
	 */
# endif

	exit (exitstatus);
    }

    /* OK, sit around getting all the input from the child.  */
    {
	struct buffer *stdoutbuf;
	struct buffer *stderrbuf;
	struct buffer *protocol_inbuf;
	/* Number of file descriptors to check in select ().  */
	int num_to_check;
	int count_needed = 1;
# ifdef SERVER_FLOWCONTROL
	int have_flowcontrolled = 0;
# endif /* SERVER_FLOWCONTROL */

	FD_ZERO (&command_fds_to_drain.fds);
	num_to_check = stdout_pipe[0];
	FD_SET (stdout_pipe[0], &command_fds_to_drain.fds);
	num_to_check = MAX (num_to_check, stderr_pipe[0]);
	FD_SET (stderr_pipe[0], &command_fds_to_drain.fds);
	num_to_check = MAX (num_to_check, protocol_pipe[0]);
	FD_SET (protocol_pipe[0], &command_fds_to_drain.fds);
	num_to_check = MAX (num_to_check, STDOUT_FILENO);
# ifdef SUNOS_KLUDGE
	max_command_fd = num_to_check;
# endif
	/*
	 * File descriptors are numbered from 0, so num_to_check needs to
	 * be one larger than the largest descriptor.
	 */
	++num_to_check;
	if (num_to_check > FD_SETSIZE)
	{
	    buf_output0 (buf_to_net,
			 "E internal error: FD_SETSIZE not big enough.\n\
error  \n");
	    goto error_exit;
	}

	stdoutbuf = fd_buffer_initialize (stdout_pipe[0], 0, NULL, true,
					  input_memory_error);

	stderrbuf = fd_buffer_initialize (stderr_pipe[0], 0, NULL, true,
					  input_memory_error);

	protocol_inbuf = fd_buffer_initialize (protocol_pipe[0], 0, NULL, true,
					       input_memory_error);

	set_nonblock (buf_to_net);
	set_nonblock (stdoutbuf);
	set_nonblock (stderrbuf);
	set_nonblock (protocol_inbuf);

	if (close (stdout_pipe[1]) < 0)
	{
	    buf_output0 (buf_to_net, "E close failed\n");
	    print_error (errno);
	    goto error_exit;
	}
	stdout_pipe[1] = -1;

	if (close (stderr_pipe[1]) < 0)
	{
	    buf_output0 (buf_to_net, "E close failed\n");
	    print_error (errno);
	    goto error_exit;
	}
	stderr_pipe[1] = -1;

	if (close (protocol_pipe[1]) < 0)
	{
	    buf_output0 (buf_to_net, "E close failed\n");
	    print_error (errno);
	    goto error_exit;
	}
	protocol_pipe[1] = -1;

# ifdef SERVER_FLOWCONTROL
	if (close (flowcontrol_pipe[0]) < 0)
	{
	    buf_output0 (buf_to_net, "E close failed\n");
	    print_error (errno);
	    goto error_exit;
	}
	flowcontrol_pipe[0] = -1;
# endif /* SERVER_FLOWCONTROL */

	if (close (dev_null_fd) < 0)
	{
	    buf_output0 (buf_to_net, "E close failed\n");
	    print_error (errno);
	    goto error_exit;
	}
	dev_null_fd = -1;

	while (stdout_pipe[0] >= 0
	       || stderr_pipe[0] >= 0
	       || protocol_pipe[0] >= 0
	       || count_needed <= 0)
	{
	    fd_set readfds;
	    fd_set writefds;
	    int numfds;
	    struct timeval *timeout_ptr;
	    struct timeval timeout;
# ifdef SERVER_FLOWCONTROL
	    int bufmemsize;

	    /*
	     * See if we are swamping the remote client and filling our VM.
	     * Tell child to hold off if we do.
	     */
	    bufmemsize = buf_count_mem (buf_to_net);
	    if (!have_flowcontrolled && (bufmemsize > SERVER_HI_WATER))
	    {
		if (write(flowcontrol_pipe[1], "S", 1) == 1)
		    have_flowcontrolled = 1;
	    }
	    else if (have_flowcontrolled && (bufmemsize < SERVER_LO_WATER))
	    {
		if (write(flowcontrol_pipe[1], "G", 1) == 1)
		    have_flowcontrolled = 0;
	    }
# endif /* SERVER_FLOWCONTROL */

	    FD_ZERO (&readfds);
	    FD_ZERO (&writefds);

	    if (count_needed <= 0)
	    {
		/* there is data pending which was read from the protocol pipe
		 * so don't block if we don't find any data
		 */
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		timeout_ptr = &timeout;
	    }
	    else
	    {
		/* block indefinately */
		timeout_ptr = NULL;
	    }

	    if (! buf_empty_p (buf_to_net))
		FD_SET (STDOUT_FILENO, &writefds);

	    if (stdout_pipe[0] >= 0)
	    {
		FD_SET (stdout_pipe[0], &readfds);
	    }
	    if (stderr_pipe[0] >= 0)
	    {
		FD_SET (stderr_pipe[0], &readfds);
	    }
	    if (protocol_pipe[0] >= 0)
	    {
		FD_SET (protocol_pipe[0], &readfds);
	    }

	    /* This process of selecting on the three pipes means that
	     we might not get output in the same order in which it
	     was written, thus producing the well-known
	     "out-of-order" bug.  If the child process uses
	     cvs_output and cvs_outerr, it will send everything on
	     the protocol_pipe and avoid this problem, so the
	     solution is to use cvs_output and cvs_outerr in the
	     child process.  */
	    do {
		/* This used to select on exceptions too, but as far
		   as I know there was never any reason to do that and
		   SCO doesn't let you select on exceptions on pipes.  */
		numfds = select (num_to_check, &readfds, &writefds,
				 NULL, timeout_ptr);
		if (numfds < 0
			&& errno != EINTR)
		{
		    buf_output0 (buf_to_net, "E select failed\n");
		    print_error (errno);
		    goto error_exit;
		}
	    } while (numfds < 0);

	    if (numfds == 0)
	    {
		FD_ZERO (&readfds);
		FD_ZERO (&writefds);
	    }

	    if (FD_ISSET (STDOUT_FILENO, &writefds))
	    {
		/* What should we do with errors?  syslog() them?  */
		buf_send_output (buf_to_net);
	    }

	    if (protocol_pipe[0] >= 0
		&& (FD_ISSET (protocol_pipe[0], &readfds)))
	    {
		int status;
		size_t count_read;

		status = buf_input_data (protocol_inbuf, &count_read);

		if (status == -1)
		{
		    close (protocol_pipe[0]);
		    protocol_pipe[0] = -1;
		}
		else if (status > 0)
		{
		    buf_output0 (buf_to_net, "E buf_input_data failed\n");
		    print_error (status);
		    goto error_exit;
		}

		/*
		 * We only call buf_copy_counted if we have read
		 * enough bytes to make it worthwhile.  This saves us
		 * from continually recounting the amount of data we
		 * have.
		 */
		count_needed -= count_read;
	    }
	    /* this is still part of the protocol pipe procedure, but it is
	     * outside the above conditional so that unprocessed data can be
	     * left in the buffer and stderr/stdout can be read when a flush
	     * signal is received and control can return here without passing
	     * through the select code and maybe blocking
	     */
	    while (count_needed <= 0)
	    {
		int special = 0;

		count_needed = buf_copy_counted (buf_to_net,
						     protocol_inbuf,
						     &special);

		/* What should we do with errors?  syslog() them?  */
		buf_send_output (buf_to_net);

		/* If SPECIAL got set to <0, it means that the child
		 * wants us to flush the pipe & maybe stderr or stdout.
		 *
		 * After that we break to read stderr & stdout again before
		 * going back to the protocol pipe
		 *
		 * Upon breaking, count_needed = 0, so the next pass will only
		 * perform a non-blocking select before returning here to finish
		 * processing data we already read from the protocol buffer
		 */
		 if (special == -1)
		 {
		     cvs_flushout();
		     break;
		 }
		if (special == -2)
		{
		    /* If the client supports the 'F' command, we send it. */
		    if (supported_response ("F"))
		    {
			buf_append_char (buf_to_net, 'F');
			buf_append_char (buf_to_net, '\n');
		    }
		    cvs_flusherr ();
		    break;
		}
	    }

	    if (stdout_pipe[0] >= 0
		&& (FD_ISSET (stdout_pipe[0], &readfds)))
	    {
		int status;

		status = buf_input_data (stdoutbuf, NULL);

		buf_copy_lines (buf_to_net, stdoutbuf, 'M');

		if (status == -1)
		{
		    close (stdout_pipe[0]);
		    stdout_pipe[0] = -1;
		}
		else if (status > 0)
		{
		    buf_output0 (buf_to_net, "E buf_input_data failed\n");
		    print_error (status);
		    goto error_exit;
		}

		/* What should we do with errors?  syslog() them?  */
		buf_send_output (buf_to_net);
	    }

	    if (stderr_pipe[0] >= 0
		&& (FD_ISSET (stderr_pipe[0], &readfds)))
	    {
		int status;

		status = buf_input_data (stderrbuf, NULL);

		buf_copy_lines (buf_to_net, stderrbuf, 'E');

		if (status == -1)
		{
		    close (stderr_pipe[0]);
		    stderr_pipe[0] = -1;
		}
		else if (status > 0)
		{
		    buf_output0 (buf_to_net, "E buf_input_data failed\n");
		    print_error (status);
		    goto error_exit;
		}

		/* What should we do with errors?  syslog() them?  */
		buf_send_output (buf_to_net);
	    }
	}

	/*
	 * OK, we've gotten EOF on all the pipes.  If there is
	 * anything left on stdoutbuf or stderrbuf (this could only
	 * happen if there was no trailing newline), send it over.
	 */
	if (! buf_empty_p (stdoutbuf))
	{
	    buf_append_char (stdoutbuf, '\n');
	    buf_copy_lines (buf_to_net, stdoutbuf, 'M');
	}
	if (! buf_empty_p (stderrbuf))
	{
	    buf_append_char (stderrbuf, '\n');
	    buf_copy_lines (buf_to_net, stderrbuf, 'E');
	}
	if (! buf_empty_p (protocol_inbuf))
	    buf_output0 (buf_to_net,
			 "E Protocol error: uncounted data discarded\n");

# ifdef SERVER_FLOWCONTROL
	close (flowcontrol_pipe[1]);
	flowcontrol_pipe[1] = -1;
# endif /* SERVER_FLOWCONTROL */

	errs = 0;

	while (command_pid > 0)
	{
	    int status;
	    pid_t waited_pid;
	    waited_pid = waitpid (command_pid, &status, 0);
	    if (waited_pid < 0)
	    {
		/*
		 * Intentionally ignoring EINTR.  Other errors
		 * "can't happen".
		 */
		continue;
	    }

	    if (WIFEXITED (status))
		errs += WEXITSTATUS (status);
	    else
	    {
		int sig = WTERMSIG (status);
		char buf[50];
		/*
		 * This is really evil, because signals might be numbered
		 * differently on the two systems.  We should be using
		 * signal names (either of the "Terminated" or the "SIGTERM"
		 * variety).  But cvs doesn't currently use libiberty...we
		 * could roll our own....  FIXME.
		 */
		buf_output0 (buf_to_net, "E Terminated with fatal signal ");
		sprintf (buf, "%d\n", sig);
		buf_output0 (buf_to_net, buf);

		/* Test for a core dump.  */
		if (WCOREDUMP (status))
		{
		    buf_output0 (buf_to_net, "E Core dumped; preserving ");
		    buf_output0 (buf_to_net, orig_server_temp_dir);
		    buf_output0 (buf_to_net, " on server.\n\
E CVS locks may need cleaning up.\n");
		    dont_delete_temp = 1;
		}
		++errs;
	    }
	    if (waited_pid == command_pid)
		command_pid = -1;
	}

	/*
	 * OK, we've waited for the child.  By now all CVS locks are free
	 * and it's OK to block on the network.
	 */
	set_block (buf_to_net);
	buf_flush (buf_to_net, 1);
	buf_shutdown (protocol_inbuf);
	buf_free (protocol_inbuf);
	protocol_inbuf = NULL;
	buf_shutdown (stderrbuf);
	buf_free (stderrbuf);
	stderrbuf = NULL;
	buf_shutdown (stdoutbuf);
	buf_free (stdoutbuf);
	stdoutbuf = NULL;
    }

    if (errs)
	/* We will have printed an error message already.  */
	buf_output0 (buf_to_net, "error  \n");
    else
	buf_output0 (buf_to_net, "ok\n");
    goto free_args_and_return;

 error_exit:
    if (command_pid > 0)
	kill (command_pid, SIGTERM);

    while (command_pid > 0)
    {
	pid_t waited_pid;
	waited_pid = waitpid (command_pid, NULL, 0);
	if (waited_pid < 0 && errno == EINTR)
	    continue;
	if (waited_pid == command_pid)
	    command_pid = -1;
    }

    close (dev_null_fd);
    close (protocol_pipe[0]);
    close (protocol_pipe[1]);
    close (stderr_pipe[0]);
    close (stderr_pipe[1]);
    close (stdout_pipe[0]);
    close (stdout_pipe[1]);
# ifdef SERVER_FLOWCONTROL
    close (flowcontrol_pipe[0]);
    close (flowcontrol_pipe[1]);
# endif /* SERVER_FLOWCONTROL */

 free_args_and_return:
    /* Now free the arguments.  */
    {
	/* argument_vector[0] is a dummy argument, we don't mess with it.  */
	char **cp;
	for (cp = argument_vector + 1;
	     cp < argument_vector + argument_count;
	     ++cp)
	    free (*cp);

	argument_count = 1;
    }

    /* Flush out any data not yet sent.  */
    set_block (buf_to_net);
    buf_flush (buf_to_net, 1);

    return;
}



# ifdef SERVER_FLOWCONTROL
/*
 * Called by the child at convenient points in the server's execution for
 * the server child to block.. ie: when it has no locks active.
 */
void
server_pause_check(void)
{
    int paused = 0;
    char buf[1];

    while (read (flowcontrol_pipe[0], buf, 1) == 1)
    {
	if (*buf == 'S')	/* Stop */
	    paused = 1;
	else if (*buf == 'G')	/* Go */
	    paused = 0;
	else
	    return;		/* ??? */
    }
    while (paused) {
	int numfds, numtocheck;
	fd_set fds;

	FD_ZERO (&fds);
	FD_SET (flowcontrol_pipe[0], &fds);
	numtocheck = flowcontrol_pipe[0] + 1;

	do {
	    numfds = select (numtocheck, &fds, NULL, NULL, NULL);
	    if (numfds < 0
		&& errno != EINTR)
	    {
		buf_output0 (buf_to_net, "E select failed\n");
		print_error (errno);
		return;
	    }
	} while (numfds < 0);

	if (FD_ISSET (flowcontrol_pipe[0], &fds))
	{
	    int got;

	    while ((got = read (flowcontrol_pipe[0], buf, 1)) == 1)
	    {
		if (*buf == 'S')	/* Stop */
		    paused = 1;
		else if (*buf == 'G')	/* Go */
		    paused = 0;
		else
		    return;		/* ??? */
	    }

	    /* This assumes that we are using BSD or POSIX nonblocking
	       I/O.  System V nonblocking I/O returns zero if there is
	       nothing to read.  */
	    if (got == 0)
		error (1, 0, "flow control EOF");
	    if (got < 0 && ! blocking_error (errno))
	    {
		error (1, errno, "flow control read failed");
	    }
	}
    }
}
# endif /* SERVER_FLOWCONTROL */



/* This variable commented in server.h.  */
char *server_dir = NULL;



static void
output_dir (const char *update_dir, const char *repository)
{
    /* Set up SHORT_REPOS.  */
    const char *short_repos = Short_Repository (repository);

    /* Send the update_dir/repos.  */
    if (server_dir != NULL)
    {
	buf_output0 (protocol, server_dir);
	buf_output0 (protocol, "/");
    }
    if (update_dir[0] == '\0')
	buf_output0 (protocol, ".");
    else
	buf_output0 (protocol, update_dir);
    buf_output0 (protocol, "/\n");
    if (short_repos[0] == '\0')
	buf_output0 (protocol, ".");
    else
	buf_output0 (protocol, short_repos);
    buf_output0 (protocol, "/");
}



/*
 * Entries line that we are squirreling away to send to the client when
 * we are ready.
 */
static char *entries_line;

/*
 * File which has been Scratch_File'd, we are squirreling away that fact
 * to inform the client when we are ready.
 */
static char *scratched_file;

/*
 * The scratched_file will need to be removed as well as having its entry
 * removed.
 */
static int kill_scratched_file;



void
server_register (const char *name, const char *version, const char *timestamp,
                 const char *options, const char *tag, const char *date,
                 const char *conflict)
{
    int len;

    if (options == NULL)
	options = "";

    TRACE (TRACE_FUNCTION, "server_register(%s, %s, %s, %s, %s, %s, %s)",
	   name, version, timestamp ? timestamp : "", options,
	   tag ? tag : "", date ? date : "",
	   conflict ? conflict : "");

    if (entries_line != NULL)
    {
	/*
	 * If CVS decides to Register it more than once (which happens
	 * on "cvs update foo/foo.c" where foo and foo.c are already
	 * checked out), use the last of the entries lines Register'd.
	 */
	free (entries_line);
    }

    /*
     * I have reports of Scratch_Entry and Register both happening, in
     * two different cases.  Using the last one which happens is almost
     * surely correct; I haven't tracked down why they both happen (or
     * even verified that they are for the same file).
     */
    if (scratched_file != NULL)
    {
	free (scratched_file);
	scratched_file = NULL;
    }

    len = (strlen (name) + strlen (version) + strlen (options) + 80);
    if (tag)
	len += strlen (tag);
    if (date)
	len += strlen (date);

    entries_line = xmalloc (len);
    sprintf (entries_line, "/%s/%s/", name, version);
    if (conflict != NULL)
    {
	strcat (entries_line, "+=");
    }
    strcat (entries_line, "/");
    strcat (entries_line, options);
    strcat (entries_line, "/");
    if (tag != NULL)
    {
	strcat (entries_line, "T");
	strcat (entries_line, tag);
    }
    else if (date != NULL)
    {
	strcat (entries_line, "D");
	strcat (entries_line, date);
    }
}



void
server_scratch (const char *fname)
{
    /*
     * I have reports of Scratch_Entry and Register both happening, in
     * two different cases.  Using the last one which happens is almost
     * surely correct; I haven't tracked down why they both happen (or
     * even verified that they are for the same file).
     *
     * Don't know if this is what whoever wrote the above comment was
     * talking about, but this can happen in the case where a join
     * removes a file - the call to Register puts the '-vers' into the
     * Entries file after the file is removed
     */
    if (entries_line != NULL)
    {
	free (entries_line);
	entries_line = NULL;
    }

    if (scratched_file != NULL)
    {
	buf_output0 (protocol,
		     "E CVS server internal error: duplicate Scratch_Entry\n");
	buf_send_counted (protocol);
	return;
    }
    scratched_file = xstrdup (fname);
    kill_scratched_file = 1;
}



void
server_scratch_entry_only (void)
{
    kill_scratched_file = 0;
}



/* Print a new entries line, from a previous server_register.  */
static void
new_entries_line (void)
{
    if (entries_line)
    {
	buf_output0 (protocol, entries_line);
	buf_output (protocol, "\n", 1);
    }
    else
	/* Return the error message as the Entries line.  */
	buf_output0 (protocol,
		     "CVS server internal error: Register missing\n");
    free (entries_line);
    entries_line = NULL;
}



static void
serve_ci (char *arg)
{
    do_cvs_command ("commit", commit);
}



static void
checked_in_response (const char *file, const char *update_dir,
                     const char *repository)
{
    if (supported_response ("Mode"))
    {
	struct stat sb;
	char *mode_string;

	if (stat (file, &sb) < 0)
	{
	    /* Not clear to me why the file would fail to exist, but it
	       was happening somewhere in the testsuite.  */
	    if (!existence_error (errno))
		error (0, errno, "cannot stat %s", file);
	}
	else
	{
	    buf_output0 (protocol, "Mode ");
	    mode_string = mode_to_string (sb.st_mode);
	    buf_output0 (protocol, mode_string);
	    buf_output0 (protocol, "\n");
	    free (mode_string);
	}
    }

    buf_output0 (protocol, "Checked-in ");
    output_dir (update_dir, repository);
    buf_output0 (protocol, file);
    buf_output (protocol, "\n", 1);
    new_entries_line ();
}



void
server_checked_in (const char *file, const char *update_dir,
                   const char *repository)
{
    if (noexec)
	return;
    if (scratched_file != NULL && entries_line == NULL)
    {
	/*
	 * This happens if we are now doing a "cvs remove" after a previous
	 * "cvs add" (without a "cvs ci" in between).
	 */
	buf_output0 (protocol, "Remove-entry ");
	output_dir (update_dir, repository);
	buf_output0 (protocol, file);
	buf_output (protocol, "\n", 1);
	free (scratched_file);
	scratched_file = NULL;
    }
    else
    {
	checked_in_response (file, update_dir, repository);
    }
    buf_send_counted (protocol);
}



void
server_update_entries (const char *file, const char *update_dir,
                       const char *repository,
                       enum server_updated_arg4 updated)
{
    if (noexec)
	return;
    if (updated == SERVER_UPDATED)
	checked_in_response (file, update_dir, repository);
    else
    {
	if (!supported_response ("New-entry"))
	    return;
	buf_output0 (protocol, "New-entry ");
	output_dir (update_dir, repository);
	buf_output0 (protocol, file);
	buf_output (protocol, "\n", 1);
	new_entries_line ();
    }

    buf_send_counted (protocol);
}



static void
serve_update (char *arg)
{
    do_cvs_command ("update", update);
}



static void
serve_diff (char *arg)
{
    do_cvs_command ("diff", diff);
}



static void
serve_log (char *arg)
{
    do_cvs_command ("log", cvslog);
}



static void
serve_rlog (char *arg)
{
    do_cvs_command ("rlog", cvslog);
}



static void
serve_ls (char *arg)
{
  do_cvs_command ("ls", ls);
}



static void
serve_rls (char *arg)
{
  do_cvs_command ("rls", ls);
}



static void
serve_add (char *arg)
{
    do_cvs_command ("add", add);
}



static void
serve_remove (char *arg)
{
    do_cvs_command ("remove", cvsremove);
}



static void
serve_status (char *arg)
{
    do_cvs_command ("status", cvsstatus);
}



static void
serve_rdiff (char *arg)
{
    do_cvs_command ("rdiff", patch);
}



static void
serve_tag (char *arg)
{
    do_cvs_command ("tag", cvstag);
}



static void
serve_rtag (char *arg)
{
    do_cvs_command ("rtag", cvstag);
}



static void
serve_import (char *arg)
{
    do_cvs_command ("import", import);
}



static void
serve_admin (char *arg)
{
    do_cvs_command ("admin", admin);
}



static void
serve_history (char *arg)
{
    do_cvs_command ("history", history);
}



static void
serve_release (char *arg)
{
    do_cvs_command ("release", release);
}



static void
serve_watch_on (char *arg)
{
    do_cvs_command ("watch", watch_on);
}



static void
serve_watch_off (char *arg)
{
    do_cvs_command ("watch", watch_off);
}



static void
serve_watch_add (char *arg)
{
    do_cvs_command ("watch", watch_add);
}



static void
serve_watch_remove (char *arg)
{
    do_cvs_command ("watch", watch_remove);
}



static void
serve_watchers (char *arg)
{
    do_cvs_command ("watchers", watchers);
}



static void
serve_editors (char *arg)
{
    do_cvs_command ("editors", editors);
}



static void
serve_edit (char *arg)
{
    do_cvs_command ("edit", edit);
}



# ifdef PROXY_SUPPORT
/* We need to handle some of this before reprocessing since it is defined to
 * send a response and print errors before a Root request is received.
 */
# endif /* PROXY_SUPPORT */
static void
serve_noop (char *arg)
{
    /* Errors could be encountered in the first or second passes, so always
     * send them to the client.
     */
    bool pe = print_pending_error();

# ifdef PROXY_SUPPORT
    /* The portions below need not be handled until reprocessing anyhow since
     * there should be no entries or notifications prior to that.  */
    if (!proxy_log)
# endif /* PROXY_SUPPORT */
    {
	server_write_entries ();
	if (!pe)
	    (void) server_notify ();
    }

    if (!pe
# ifdef PROXY_SUPPORT
        /* "ok" only goes across in the first pass.  */
        && !reprocessing
# endif /* PROXY_SUPPORT */
       )
	buf_output0 (buf_to_net, "ok\n");
    buf_flush (buf_to_net, 1);
}



static void
serve_version (char *arg)
{
    do_cvs_command ("version", version);
}



static void
serve_init (char *arg)
{
    cvsroot_t *saved_parsed_root;

    if (!ISABSOLUTE (arg))
    {
	if (alloc_pending (80 + strlen (arg)))
	    sprintf (pending_error_text,
		     "E init %s must be an absolute pathname", arg);
    }
# ifdef AUTH_SERVER_SUPPORT
    else if (Pserver_Repos != NULL)
    {
	if (strcmp (Pserver_Repos, arg) != 0)
	{
	    if (alloc_pending (80 + strlen (Pserver_Repos) + strlen (arg)))
		/* The explicitness is to aid people who are writing clients.
		   I don't see how this information could help an
		   attacker.  */
		sprintf (pending_error_text, "\
E Protocol error: init says \"%s\" but pserver says \"%s\"",
			 arg, Pserver_Repos);
	}
    }
# endif

    if (print_pending_error ())
	return;

    saved_parsed_root = current_parsed_root;
    current_parsed_root = local_cvsroot (arg);

    do_cvs_command ("init", init);

    /* Do not free CURRENT_PARSED_ROOT since it is still in the cache.  */
    current_parsed_root = saved_parsed_root;
}



static void
serve_annotate (char *arg)
{
    do_cvs_command ("annotate", annotate);
}



static void
serve_rannotate (char *arg)
{
    do_cvs_command ("rannotate", annotate);
}



static void
serve_co (char *arg)
{
    if (print_pending_error ())
	return;

# ifdef PROXY_SUPPORT
    /* If we are not a secondary server, the write proxy log will already have
     * been processed.
     */
    if (isProxyServer ())
    {
	if (reprocessing)
	    reprocessing = false;
	else if (/* The proxy log may be closed if the client sent a
		  * `Command-prep' request.
		  */
		 proxy_log)
	{
	    /* Set up the log for reprocessing.  */
	    rewind_buf_from_net ();
	    /* And return to the main loop in server(), where we will now find
	     * the logged secondary data and reread it.
	     */
	    return;
	}
    }
# endif /* PROXY_SUPPORT */

    /* Compensate for server_export()'s setting of cvs_cmd_name.
     *
     * [It probably doesn't matter if do_cvs_command() gets "export"
     *  or "checkout", but we ought to be accurate where possible.]
     */
    do_cvs_command (!strcmp (cvs_cmd_name, "export") ? "export" : "checkout",
                   checkout);
}



static void
serve_export (char *arg)
{
    /* Tell checkout() to behave like export not checkout.  */
    cvs_cmd_name = "export";
    serve_co (arg);
}



void
server_copy_file (const char *file, const char *update_dir,
                  const char *repository, const char *newfile)
{
    /* At least for now, our practice is to have the server enforce
       noexec for the repository and the client enforce it for the
       working directory.  This might want more thought, and/or
       documentation in cvsclient.texi (other responses do it
       differently).  */

    if (!supported_response ("Copy-file"))
	return;
    buf_output0 (protocol, "Copy-file ");
    output_dir (update_dir, repository);
    buf_output0 (protocol, file);
    buf_output0 (protocol, "\n");
    buf_output0 (protocol, newfile);
    buf_output0 (protocol, "\n");
}



/* See server.h for description.  */
void
server_modtime (struct file_info *finfo, Vers_TS *vers_ts)
{
    char date[MAXDATELEN];
    char outdate[MAXDATELEN];

    assert (vers_ts->vn_rcs != NULL);

    if (!supported_response ("Mod-time"))
	return;

    if (RCS_getrevtime (finfo->rcs, vers_ts->vn_rcs, date, 0) == (time_t) -1)
	/* FIXME? should we be printing some kind of warning?  For one
	   thing I'm not 100% sure whether this happens in non-error
	   circumstances.  */
	return;
    date_to_internet (outdate, date);
    buf_output0 (protocol, "Mod-time ");
    buf_output0 (protocol, outdate);
    buf_output0 (protocol, "\n");
}



/* See server.h for description.  */
void
server_updated (
    struct file_info *finfo,
    Vers_TS *vers,
    enum server_updated_arg4 updated,
    mode_t mode,
    unsigned char *checksum,
    struct buffer *filebuf)
{
    if (noexec)
    {
	/* Hmm, maybe if we did the same thing for entries_file, we
	   could get rid of the kludges in server_register and
	   server_scratch which refrain from warning if both
	   Scratch_Entry and Register get called.  Maybe.  */
	if (scratched_file)
	{
	    free (scratched_file);
	    scratched_file = NULL;
	}
	buf_send_counted (protocol);
	return;
    }

    if (entries_line != NULL && scratched_file == NULL)
    {
	FILE *f;
	struct buffer_data *list, *last;
	unsigned long size;
	char size_text[80];

	/* The contents of the file will be in one of filebuf,
	   list/last, or here.  */
	unsigned char *file;
	size_t file_allocated;
	size_t file_used;

	if (filebuf != NULL)
	{
	    size = buf_length (filebuf);
	    if (mode == (mode_t) -1)
		error (1, 0, "\
CVS server internal error: no mode in server_updated");
	}
	else
	{
	    struct stat sb;

	    if (stat (finfo->file, &sb) < 0)
	    {
		if (existence_error (errno))
		{
		    /* If we have a sticky tag for a branch on which
		       the file is dead, and cvs update the directory,
		       it gets a T_CHECKOUT but no file.  So in this
		       case just forget the whole thing.  */
		    free (entries_line);
		    entries_line = NULL;
		    goto done;
		}
		error (1, errno, "reading %s", finfo->fullname);
	    }
	    size = sb.st_size;
	    if (mode == (mode_t) -1)
	    {
		/* FIXME: When we check out files the umask of the
		   server (set in .bashrc if rsh is in use) affects
		   what mode we send, and it shouldn't.  */
		mode = sb.st_mode;
	    }
	}

	if (checksum != NULL)
	{
	    static int checksum_supported = -1;

	    if (checksum_supported == -1)
	    {
		checksum_supported = supported_response ("Checksum");
	    }

	    if (checksum_supported)
	    {
		int i;
		char buf[3];

		buf_output0 (protocol, "Checksum ");
		for (i = 0; i < 16; i++)
		{
		    sprintf (buf, "%02x", (unsigned int) checksum[i]);
		    buf_output0 (protocol, buf);
		}
		buf_append_char (protocol, '\n');
	    }
	}

	if (updated == SERVER_UPDATED)
	{
	    Node *node;
	    Entnode *entnode;

	    if (!(supported_response ("Created")
		  && supported_response ("Update-existing")))
		buf_output0 (protocol, "Updated ");
	    else
	    {
		assert (vers != NULL);
		if (vers->ts_user == NULL)
		    buf_output0 (protocol, "Created ");
		else
		    buf_output0 (protocol, "Update-existing ");
	    }

	    /* Now munge the entries to say that the file is unmodified,
	       in case we end up processing it again (e.g. modules3-6
	       in the testsuite).  */
	    node = findnode_fn (finfo->entries, finfo->file);
	    entnode = node->data;
	    free (entnode->timestamp);
	    entnode->timestamp = xstrdup ("=");
	}
	else if (updated == SERVER_MERGED)
	    buf_output0 (protocol, "Merged ");
	else if (updated == SERVER_PATCHED)
	    buf_output0 (protocol, "Patched ");
	else if (updated == SERVER_RCS_DIFF)
	    buf_output0 (protocol, "Rcs-diff ");
	else
	    abort ();
	output_dir (finfo->update_dir, finfo->repository);
	buf_output0 (protocol, finfo->file);
	buf_output (protocol, "\n", 1);

	new_entries_line ();

	{
	    char *mode_string;

	    mode_string = mode_to_string (mode);
	    buf_output0 (protocol, mode_string);
	    buf_output0 (protocol, "\n");
	    free (mode_string);
	}

	list = last = NULL;

	file = NULL;
	file_allocated = 0;
	file_used = 0;

	if (size > 0)
	{
	    /* Throughout this section we use binary mode to read the
	       file we are sending.  The client handles any line ending
	       translation if necessary.  */

	    if (file_gzip_level
		/*
		 * For really tiny files, the gzip process startup
		 * time will outweigh the compression savings.  This
		 * might be computable somehow; using 100 here is just
		 * a first approximation.
		 */
		&& size > 100)
	    {
		/* Basing this routine on read_and_gzip is not a
		   high-performance approach.  But it seems easier
		   to code than the alternative (and less
		   vulnerable to subtle bugs).  Given that this feature
		   is mainly for compatibility, that is the better
		   tradeoff.  */

		int fd;

		/* Callers must avoid passing us a buffer if
		   file_gzip_level is set.  We could handle this case,
		   but it's not worth it since this case never arises
		   with a current client and server.  */
		if (filebuf != NULL)
		    error (1, 0, "\
CVS server internal error: unhandled case in server_updated");

		fd = CVS_OPEN (finfo->file, O_RDONLY | OPEN_BINARY, 0);
		if (fd < 0)
		    error (1, errno, "reading %s", finfo->fullname);
		if (read_and_gzip (fd, finfo->fullname, &file,
				   &file_allocated, &file_used,
				   file_gzip_level))
		    error (1, 0, "aborting due to compression error");
		size = file_used;
		if (close (fd) < 0)
		    error (1, errno, "reading %s", finfo->fullname);
		/* Prepending length with "z" is flag for using gzip here.  */
		buf_output0 (protocol, "z");
	    }
	    else if (filebuf == NULL)
	    {
		long status;

		f = CVS_FOPEN (finfo->file, "rb");
		if (f == NULL)
		    error (1, errno, "reading %s", finfo->fullname);
		status = buf_read_file (f, size, &list, &last);
		if (status == -2)
		    (*protocol->memory_error) (protocol);
		else if (status != 0)
		    error (1, ferror (f) ? errno : 0, "reading %s",
			   finfo->fullname);
		if (fclose (f) == EOF)
		    error (1, errno, "reading %s", finfo->fullname);
	    }
	}

	sprintf (size_text, "%lu\n", size);
	buf_output0 (protocol, size_text);

	if (file != NULL)
	{
	    buf_output (protocol, (char *) file, file_used);
	    free (file);
	    file = NULL;
	}
	else if (filebuf == NULL)
	    buf_append_data (protocol, list, last);
	else
	    buf_append_buffer (protocol, filebuf);
	/* Note we only send a newline here if the file ended with one.  */

	/*
	 * Avoid using up too much disk space for temporary files.
	 * A file which does not exist indicates that the file is up-to-date,
	 * which is now the case.  If this is SERVER_MERGED, the file is
	 * not up-to-date, and we indicate that by leaving the file there.
	 * I'm thinking of cases like "cvs update foo/foo.c foo".
	 */
	if ((updated == SERVER_UPDATED
	     || updated == SERVER_PATCHED
	     || updated == SERVER_RCS_DIFF)
	    && filebuf == NULL
	    /* But if we are joining, we'll need the file when we call
	       join_file.  */
	    && !joining ())
	{
	    if (CVS_UNLINK (finfo->file) < 0)
		error (0, errno, "cannot remove temp file for %s",
		       finfo->fullname);
	}
    }
    else if (scratched_file != NULL && entries_line == NULL)
    {
	if (strcmp (scratched_file, finfo->file) != 0)
	    error (1, 0,
		   "CVS server internal error: `%s' vs. `%s' scratched",
		   scratched_file,
		   finfo->file);
	free (scratched_file);
	scratched_file = NULL;

	if (kill_scratched_file)
	    buf_output0 (protocol, "Removed ");
	else
	    buf_output0 (protocol, "Remove-entry ");
	output_dir (finfo->update_dir, finfo->repository);
	buf_output0 (protocol, finfo->file);
	buf_output (protocol, "\n", 1);
	/* keep the vers structure up to date in case we do a join
	 * - if there isn't a file, it can't very well have a version number,
	 *   can it?
	 *
	 * we do it here on the assumption that since we just told the client
	 * to remove the file/entry, it will, and we want to remember that.
	 * If it fails, that's the client's problem, not ours
	 */
	if (vers && vers->vn_user != NULL)
	{
	    free (vers->vn_user);
	    vers->vn_user = NULL;
	}
	if (vers && vers->ts_user != NULL)
	{
	    free (vers->ts_user);
	    vers->ts_user = NULL;
	}
    }
    else if (scratched_file == NULL && entries_line == NULL)
    {
	/*
	 * This can happen with death support if we were processing
	 * a dead file in a checkout.
	 */
    }
    else
	error (1, 0,
	       "CVS server internal error: Register *and* Scratch_Entry.\n");
    buf_send_counted (protocol);
  done:;
}



/* Return whether we should send patches in RCS format.  */
int
server_use_rcs_diff (void)
{
    return supported_response ("Rcs-diff");
}



void
server_set_entstat (const char *update_dir, const char *repository)
{
    static int set_static_supported = -1;
    if (set_static_supported == -1)
	set_static_supported = supported_response ("Set-static-directory");
    if (!set_static_supported) return;

    buf_output0 (protocol, "Set-static-directory ");
    output_dir (update_dir, repository);
    buf_output0 (protocol, "\n");
    buf_send_counted (protocol);
}



void
server_clear_entstat (const char *update_dir, const char *repository)
{
    static int clear_static_supported = -1;
    if (clear_static_supported == -1)
	clear_static_supported = supported_response ("Clear-static-directory");
    if (!clear_static_supported) return;

    if (noexec)
	return;

    buf_output0 (protocol, "Clear-static-directory ");
    output_dir (update_dir, repository);
    buf_output0 (protocol, "\n");
    buf_send_counted (protocol);
}



void
server_set_sticky (const char *update_dir, const char *repository,
                   const char *tag, const char *date, int nonbranch)
{
    static int set_sticky_supported = -1;

    assert (update_dir != NULL);

    if (set_sticky_supported == -1)
	set_sticky_supported = supported_response ("Set-sticky");
    if (!set_sticky_supported) return;

    if (noexec)
	return;

    if (tag == NULL && date == NULL)
    {
	buf_output0 (protocol, "Clear-sticky ");
	output_dir (update_dir, repository);
	buf_output0 (protocol, "\n");
    }
    else
    {
	buf_output0 (protocol, "Set-sticky ");
	output_dir (update_dir, repository);
	buf_output0 (protocol, "\n");
	if (tag != NULL)
	{
	    if (nonbranch)
		buf_output0 (protocol, "N");
	    else
		buf_output0 (protocol, "T");
	    buf_output0 (protocol, tag);
	}
	else
	{
	    buf_output0 (protocol, "D");
	    buf_output0 (protocol, date);
	}
	buf_output0 (protocol, "\n");
    }
    buf_send_counted (protocol);
}



void
server_edit_file (struct file_info *finfo)
{
    buf_output (protocol, "Edit-file ", 10);
    output_dir (finfo->update_dir, finfo->repository);
    buf_output0 (protocol, finfo->file);
    buf_output (protocol, "\n", 1);
    buf_send_counted (protocol);
}



struct template_proc_data
{
    const char *update_dir;
    const char *repository;
};

static int
template_proc (const char *repository, const char *template, void *closure)
{
    FILE *fp;
    char buf[1024];
    size_t n;
    struct stat sb;
    struct template_proc_data *data = (struct template_proc_data *)closure;

    if (!supported_response ("Template"))
	/* Might want to warn the user that the rcsinfo feature won't work.  */
	return 0;
    buf_output0 (protocol, "Template ");
    output_dir (data->update_dir, data->repository);
    buf_output0 (protocol, "\n");

    fp = CVS_FOPEN (template, "rb");
    if (fp == NULL)
    {
	error (0, errno, "Couldn't open rcsinfo template file %s", template);
	return 1;
    }
    if (fstat (fileno (fp), &sb) < 0)
    {
	error (0, errno, "cannot stat rcsinfo template file %s", template);
	return 1;
    }
    sprintf (buf, "%ld\n", (long) sb.st_size);
    buf_output0 (protocol, buf);
    while (!feof (fp))
    {
	n = fread (buf, 1, sizeof buf, fp);
	buf_output (protocol, buf, n);
	if (ferror (fp))
	{
	    error (0, errno, "cannot read rcsinfo template file %s", template);
	    (void) fclose (fp);
	    return 1;
	}
    }
    buf_send_counted (protocol);
    if (fclose (fp) < 0)
	error (0, errno, "cannot close rcsinfo template file %s", template);
    return 0;
}



void
server_clear_template (const char *update_dir, const char *repository)
{
    assert (update_dir != NULL);

    if (noexec)
	return;

    if (!supported_response ("Clear-template") &&
	!supported_response ("Template"))
	/* Might want to warn the user that the rcsinfo feature won't work.  */
	return;

    if (supported_response ("Clear-template"))
    {
	buf_output0 (protocol, "Clear-template ");
	output_dir (update_dir, repository);
	buf_output0 (protocol, "\n");
	buf_send_counted (protocol);
    }
    else
    {
	buf_output0 (protocol, "Template ");
	output_dir (update_dir, repository);
	buf_output0 (protocol, "\n");
	buf_output0 (protocol, "0\n");
	buf_send_counted (protocol);
    }
}



void
server_template (const char *update_dir, const char *repository)
{
    struct template_proc_data data;
    data.update_dir = update_dir;
    data.repository = repository;
    (void) Parse_Info (CVSROOTADM_RCSINFO, repository, template_proc,
		       PIOPT_ALL, &data);
}



static void
serve_gzip_contents (char *arg)
{
    int level;
    bool forced = false;

# ifdef PROXY_SUPPORT
    assert (!proxy_log);
# endif /* PROXY_SUPPORT */

    level = atoi (arg);
    if (level == 0)
	level = 6;

    if (config && level < config->MinCompressionLevel)
    {
	level = config->MinCompressionLevel;
	forced = true;
    }
    if (config && level > config->MaxCompressionLevel)
    {
	level = config->MaxCompressionLevel;
	forced = true;
    }

    if (forced && !quiet
	&& alloc_pending_warning (120 + strlen (program_name)))
	sprintf (pending_warning_text,
"E %s server: Forcing compression level %d (allowed: %d <= z <= %d).",
		 program_name, level, config->MinCompressionLevel,
		 config->MaxCompressionLevel);

    gzip_level = file_gzip_level = level;
}



static void
serve_gzip_stream (char *arg)
{
    int level;
    bool forced = false;

    level = atoi (arg);

    if (config && level < config->MinCompressionLevel)
    {
	level = config->MinCompressionLevel;
	forced = true;
    }
    if (config && level > config->MaxCompressionLevel)
    {
	level = config->MaxCompressionLevel;
	forced = true;
    }

    if (forced && !quiet
	&& alloc_pending_warning (120 + strlen (program_name)))
	sprintf (pending_warning_text,
"E %s server: Forcing compression level %d (allowed: %d <= z <= %d).",
		 program_name, level, config->MinCompressionLevel,
		 config->MaxCompressionLevel);
	
    gzip_level = level;

    /* All further communication with the client will be compressed.
     *
     * The deflate buffers need to be initialized even for compression level
     * 0, or the client will no longer be able to understand us.  At
     * compression level 0, the correct compression headers will be created and
     * sent, but data will thereafter simply be copied to the network buffers.
     */

    /* This needs to be processed in both passes so that we may continue to
     * understand client requests on both the socket and from the log.
     */
    buf_from_net = compress_buffer_initialize (buf_from_net, 1,
					       0 /* Not used. */,
					       buf_from_net->memory_error);

    /* This needs to be skipped in subsequent passes to avoid compressing data
     * to the client twice.
     */
# ifdef PROXY_SUPPORT
    if (reprocessing) return;
# endif /* PROXY_SUPPORT */
    buf_to_net = compress_buffer_initialize (buf_to_net, 0, level,
					     buf_to_net->memory_error);
}



/* Tell the client about RCS options set in CVSROOT/cvswrappers. */
static void
serve_wrapper_sendme_rcs_options (char *arg)
{
    /* Actually, this is kind of sdrawkcab-ssa: the client wants
     * verbatim lines from a cvswrappers file, but the server has
     * already parsed the cvswrappers file into the wrap_list struct.
     * Therefore, the server loops over wrap_list, unparsing each
     * entry before sending it.
     */
    char *wrapper_line = NULL;

# ifdef PROXY_SUPPORT
    if (reprocessing) return;
# endif /* PROXY_SUPPORT */

    wrap_setup ();

    for (wrap_unparse_rcs_options (&wrapper_line, 1);
	 wrapper_line;
	 wrap_unparse_rcs_options (&wrapper_line, 0))
    {
	buf_output0 (buf_to_net, "Wrapper-rcsOption ");
	buf_output0 (buf_to_net, wrapper_line);
	buf_output0 (buf_to_net, "\012");;
	free (wrapper_line);
    }

    buf_output0 (buf_to_net, "ok\012");

    /* The client is waiting for us, so we better send the data now.  */
    buf_flush (buf_to_net, 1);
}



static void
serve_ignore (char *arg)
{
    /*
     * Just ignore this command.  This is used to support the
     * update-patches command, which is not a real command, but a signal
     * to the client that update will accept the -u argument.
     */
# ifdef PROXY_SUPPORT
    assert (!proxy_log);
# endif /* PROXY_SUPPORT */
}



static int
expand_proc (int argc, char **argv, char *where, char *mwhere, char *mfile, int shorten, int local_specified, char *omodule, char *msg)
{
    int i;
    char *dir = argv[0];

    /* If mwhere has been specified, the thing we're expanding is a
       module -- just return its name so the client will ask for the
       right thing later.  If it is an alias or a real directory,
       mwhere will not be set, so send out the appropriate
       expansion. */

    if (mwhere != NULL)
    {
	buf_output0 (buf_to_net, "Module-expansion ");
	if (server_dir != NULL)
	{
	    buf_output0 (buf_to_net, server_dir);
	    buf_output0 (buf_to_net, "/");
	}
	buf_output0 (buf_to_net, mwhere);
	if (mfile != NULL)
	{
	    buf_append_char (buf_to_net, '/');
	    buf_output0 (buf_to_net, mfile);
	}
	buf_append_char (buf_to_net, '\n');
    }
    else
    {
	/* We may not need to do this anymore -- check the definition
	   of aliases before removing */
	if (argc == 1)
	{
	    buf_output0 (buf_to_net, "Module-expansion ");
	    if (server_dir != NULL)
	    {
		buf_output0 (buf_to_net, server_dir);
		buf_output0 (buf_to_net, "/");
	    }
	    buf_output0 (buf_to_net, dir);
	    buf_append_char (buf_to_net, '\n');
	}
	else
	{
	    for (i = 1; i < argc; ++i)
	    {
		buf_output0 (buf_to_net, "Module-expansion ");
		if (server_dir != NULL)
		{
		    buf_output0 (buf_to_net, server_dir);
		    buf_output0 (buf_to_net, "/");
		}
		buf_output0 (buf_to_net, dir);
		buf_append_char (buf_to_net, '/');
		buf_output0 (buf_to_net, argv[i]);
		buf_append_char (buf_to_net, '\n');
	    }
	}
    }
    return 0;
}



static void
serve_expand_modules (char *arg)
{
    int i;
    int err = 0;
    DBM *db;

# ifdef PROXY_SUPPORT
    /* This needs to be processed in the first pass since the client expects a
     * response but we may not yet know if we are a secondary.
     *
     * On the second pass, we still must make sure to ignore the arguments.
     */
    if (!reprocessing)
# endif /* PROXY_SUPPORT */
    {
	err = 0;

	db = open_module ();
	for (i = 1; i < argument_count; i++)
	    err += do_module (db, argument_vector[i],
			      CHECKOUT, "Updating", expand_proc,
			      NULL, 0, 0, 0, 0, NULL);
	close_module (db);
    }

    {
	/* argument_vector[0] is a dummy argument, we don't mess with it.  */
	char **cp;
	for (cp = argument_vector + 1;
	     cp < argument_vector + argument_count;
	     ++cp)
	    free (*cp);

	argument_count = 1;
    }

# ifdef PROXY_SUPPORT
    if (!reprocessing)
# endif /* PROXY_SUPPORT */
    {
	if (err)
	    /* We will have printed an error message already.  */
	    buf_output0 (buf_to_net, "error  \n");
	else
	    buf_output0 (buf_to_net, "ok\n");

	/* The client is waiting for the module expansions, so we must
	   send the output now.  */
	buf_flush (buf_to_net, 1);
    }
}



/* Decide if we should redirect the client to another server.
 *
 * GLOBALS
 *   config->PrimaryServer	The server to redirect write requests to, if
 *				any.
 *
 * ASSUMPTIONS
 *   The `Root' request has already been processed.
 *
 * RETURNS
 *   Nothing.
 */
static void
serve_command_prep (char *arg)
{
    bool redirect_supported;
# ifdef PROXY_SUPPORT
    bool ditch_log;
# endif /* PROXY_SUPPORT */

    if (print_pending_error ()) return;

    redirect_supported = supported_response ("Redirect");
    if (redirect_supported
	&& lookup_command_attribute (arg) & CVS_CMD_MODIFIES_REPOSITORY
	/* I call isProxyServer() last because it can probably be the slowest
	 * call due to the call to gethostbyname().
	 */
	&& isProxyServer ())
    {
	/* Before sending a redirect, send a "Referrer" line to the client,
	 * if possible, to give admins more control over canonicalizing roots
	 * sent from the client.
	 */
	if (supported_response ("Referrer"))
	{
	    /* assume :ext:, since that is all we currently support for
	     * proxies and redirection.
	     */
	    char *referrer = Xasprintf (":ext:%s@%s%s", getcaller(),
					server_hostname,
					current_parsed_root->directory);

	    buf_output0 (buf_to_net, "Referrer ");
	    buf_output0 (buf_to_net, referrer);
	    buf_output0 (buf_to_net, "\n");

	    free (referrer);
	}

	/* Send `Redirect' to redirect client requests to the primary.  */
	buf_output0 (buf_to_net, "Redirect ");
	buf_output0 (buf_to_net, config->PrimaryServer->original);
	buf_output0 (buf_to_net, "\n");
	buf_flush (buf_to_net, 1);
# ifdef PROXY_SUPPORT
	ditch_log = true;
# endif /* PROXY_SUPPORT */
    }
    else
    {
	/* Send `ok' so the client can proceed.  */
	buf_output0 (buf_to_net, "ok\n");
	buf_flush (buf_to_net, 1);
# ifdef PROXY_SUPPORT
	if (lookup_command_attribute (arg) & CVS_CMD_MODIFIES_REPOSITORY
            && isProxyServer ())
	    /* Don't ditch the log for write commands on a proxy server.  We
	     * we got here because the `Redirect' response was not supported.
	     */
	    ditch_log = false;
	else
	    ditch_log = true;
# endif /* PROXY_SUPPORT */
    }
# ifdef PROXY_SUPPORT
    if (proxy_log && ditch_log)
    {
	/* If the client supported the redirect response, then they will always
	 * be redirected if they are preparing for a write request.  It is
	 * therefore safe to close the proxy logs.
	 *
	 * If the client is broken and ignores the redirect, this will be
	 * detected later, in rewind_buf_from_net().
	 *
	 * Since a `Command-prep' response is only acceptable immediately
	 * following the `Root' request according to the specification, there
	 * is no need to rewind the log and reprocess.
	 */
	log_buffer_closelog (proxy_log);
	log_buffer_closelog (proxy_log_out);
	proxy_log = NULL;
    }
# endif /* PROXY_SUPPORT */
}



/* Save a referrer, potentially for passing to hook scripts later.
 *
 * GLOBALS
 *   referrer	Where we save the parsed referrer.
 *
 * ASSUMPTIONS
 *   The `Root' request has already been processed.
 *   There is no need to dispose of REFERRER if it is set.  It's memory is
 *   tracked by parse_root().
 *
 * RETURNS
 *   Nothing.
 */
static void
serve_referrer (char *arg)
{
    if (error_pending ()) return;

    referrer = parse_cvsroot (arg);

    if (!referrer
	&& alloc_pending (80 + strlen (arg)))
	sprintf (pending_error_text,
		 "E Protocol error: Invalid Referrer: `%s'",
		 arg);
}



static void serve_valid_requests (char *arg);

#endif /* SERVER_SUPPORT */
/*
 * Comment to move position of the following #if line which works
 * around an apparent bug in Microsoft Visual C++ 6.0 compiler.
 */
#if defined(SERVER_SUPPORT) || defined(CLIENT_SUPPORT)
/*
 * Parts of this table are shared with the client code,
 * but the client doesn't need to know about the handler
 * functions.
 */

struct request requests[] =
{
#ifdef SERVER_SUPPORT
#define REQ_LINE(n, f, s) {n, f, s}
#else
#define REQ_LINE(n, f, s) {n, s}
#endif

  REQ_LINE("Root", serve_root, RQ_ESSENTIAL | RQ_ROOTLESS),
  REQ_LINE("Valid-responses", serve_valid_responses,
	   RQ_ESSENTIAL | RQ_ROOTLESS),
  REQ_LINE("valid-requests", serve_valid_requests,
	   RQ_ESSENTIAL | RQ_ROOTLESS),
  REQ_LINE("Command-prep", serve_command_prep, 0),
  REQ_LINE("Referrer", serve_referrer, 0),
  REQ_LINE("Repository", serve_repository, 0),
  REQ_LINE("Directory", serve_directory, RQ_ESSENTIAL),
  REQ_LINE("Relative-directory", serve_directory, 0),
  REQ_LINE("Max-dotdot", serve_max_dotdot, 0),
  REQ_LINE("Static-directory", serve_static_directory, 0),
  REQ_LINE("Sticky", serve_sticky, 0),
  REQ_LINE("Checkin-prog", serve_noop, 0),
  REQ_LINE("Update-prog", serve_noop, 0),
  REQ_LINE("Entry", serve_entry, RQ_ESSENTIAL),
  REQ_LINE("Kopt", serve_kopt, 0),
  REQ_LINE("Checkin-time", serve_checkin_time, 0),
  REQ_LINE("Modified", serve_modified, RQ_ESSENTIAL),
  REQ_LINE("Is-modified", serve_is_modified, 0),

  /* The client must send this request to interoperate with CVS 1.5
     through 1.9 servers.  The server must support it (although it can
     be and is a noop) to interoperate with CVS 1.5 to 1.9 clients.  */
  REQ_LINE("UseUnchanged", serve_enable_unchanged, RQ_ENABLEME | RQ_ROOTLESS),

  REQ_LINE("Unchanged", serve_unchanged, RQ_ESSENTIAL),
  REQ_LINE("Notify", serve_notify, 0),
  REQ_LINE("Hostname", serve_hostname, 0),
  REQ_LINE("LocalDir", serve_localdir, 0),
  REQ_LINE("Questionable", serve_questionable, 0),
  REQ_LINE("Argument", serve_argument, RQ_ESSENTIAL),
  REQ_LINE("Argumentx", serve_argumentx, RQ_ESSENTIAL),
  REQ_LINE("Global_option", serve_global_option, RQ_ROOTLESS),
  /* This is rootless, even though the client/server spec does not specify
   * such, to allow error messages to be understood by the client when they are
   * sent.
   */
  REQ_LINE("Gzip-stream", serve_gzip_stream, RQ_ROOTLESS),
  REQ_LINE("wrapper-sendme-rcsOptions",
	   serve_wrapper_sendme_rcs_options,
	   0),
  REQ_LINE("Set", serve_set, RQ_ROOTLESS),
#ifdef ENCRYPTION
  /* These are rootless despite what the client/server spec says for the same
   * reasons as Gzip-stream.
   */
#  ifdef HAVE_KERBEROS
  REQ_LINE("Kerberos-encrypt", serve_kerberos_encrypt, RQ_ROOTLESS),
#  endif
#  ifdef HAVE_GSSAPI
  REQ_LINE("Gssapi-encrypt", serve_gssapi_encrypt, RQ_ROOTLESS),
#  endif
#endif
#ifdef HAVE_GSSAPI
  REQ_LINE("Gssapi-authenticate", serve_gssapi_authenticate, RQ_ROOTLESS),
#endif
  REQ_LINE("expand-modules", serve_expand_modules, 0),
  REQ_LINE("ci", serve_ci, RQ_ESSENTIAL),
  REQ_LINE("co", serve_co, RQ_ESSENTIAL),
  REQ_LINE("update", serve_update, RQ_ESSENTIAL),
  REQ_LINE("diff", serve_diff, 0),
  REQ_LINE("log", serve_log, 0),
  REQ_LINE("rlog", serve_rlog, 0),
  REQ_LINE("list", serve_ls, 0),
  REQ_LINE("rlist", serve_rls, 0),
  /* This allows us to avoid sending `-q' as a command argument to `cvs ls',
   * or more accurately, allows us to send `-q' to backwards CVSNT servers.
   */
  REQ_LINE("global-list-quiet", serve_noop, RQ_ROOTLESS),
  /* Deprecated synonym for rlist, for compatibility with CVSNT. */
  REQ_LINE("ls", serve_rls, 0),
  REQ_LINE("add", serve_add, 0),
  REQ_LINE("remove", serve_remove, 0),
  REQ_LINE("update-patches", serve_ignore, 0),
  REQ_LINE("gzip-file-contents", serve_gzip_contents, RQ_ROOTLESS),
  REQ_LINE("status", serve_status, 0),
  REQ_LINE("rdiff", serve_rdiff, 0),
  REQ_LINE("tag", serve_tag, 0),
  REQ_LINE("rtag", serve_rtag, 0),
  REQ_LINE("import", serve_import, 0),
  REQ_LINE("admin", serve_admin, 0),
  REQ_LINE("export", serve_export, 0),
  REQ_LINE("history", serve_history, 0),
  REQ_LINE("release", serve_release, 0),
  REQ_LINE("watch-on", serve_watch_on, 0),
  REQ_LINE("watch-off", serve_watch_off, 0),
  REQ_LINE("watch-add", serve_watch_add, 0),
  REQ_LINE("watch-remove", serve_watch_remove, 0),
  REQ_LINE("watchers", serve_watchers, 0),
  REQ_LINE("editors", serve_editors, 0),
  REQ_LINE("edit", serve_edit, 0),
  REQ_LINE("init", serve_init, RQ_ROOTLESS),
  REQ_LINE("annotate", serve_annotate, 0),
  REQ_LINE("rannotate", serve_rannotate, 0),
  REQ_LINE("noop", serve_noop, RQ_ROOTLESS),
  REQ_LINE("version", serve_version, RQ_ROOTLESS),
  REQ_LINE(NULL, NULL, 0)

#undef REQ_LINE
};
#endif /* SERVER_SUPPORT or CLIENT_SUPPORT */



#ifdef SERVER_SUPPORT
/*
 * This server request is not ignored by the secondary.
 */
static void
serve_valid_requests (char *arg)
{
    struct request *rq;

    /* Since this is processed in the first pass, don't reprocess it in the
     * second.
     *
     * We still print errors since new errors could have been generated in the
     * second pass.
     */
    if (print_pending_error ()
#ifdef PROXY_SUPPORT
	|| reprocessing
#endif /* PROXY_SUPPORT */
       )
	return;

    buf_output0 (buf_to_net, "Valid-requests");
    for (rq = requests; rq->name != NULL; rq++)
    {
	if (rq->func != NULL)
	{
	    buf_append_char (buf_to_net, ' ');
	    buf_output0 (buf_to_net, rq->name);
	}
    }

    if (config && config->MinCompressionLevel
	&& supported_response ("Force-gzip"))
    {
	    buf_output0 (buf_to_net, "\n");
	    buf_output0 (buf_to_net, "Force-gzip");
    }

    buf_output0 (buf_to_net, "\nok\n");

    /* The client is waiting for the list of valid requests, so we
       must send the output now.  */
    buf_flush (buf_to_net, 1);
}



#ifdef SUNOS_KLUDGE
/*
 * Delete temporary files.  SIG is the signal making this happen, or
 * 0 if not called as a result of a signal.
 */
static int command_pid_is_dead;
static void wait_sig (int sig)
{
    int status;
    pid_t r = wait (&status);
    if (r == command_pid)
	command_pid_is_dead++;
}
#endif /* SUNOS_KLUDGE */



/*
 * This function cleans up after the server.  Specifically, it:
 *
 * <ol>
 * <li>Sets BUF_TO_NET to blocking and fluxhes it.</li>
 * <li>With SUNOS_KLUDGE enabled:
 *   <ol>
 *   <li>Terminates the command process.</li>
 *   <li>Waits on the command process, draining output as necessary.</li>
 *   </ol>
 * </li>
 * <li>Removes the temporary directory.</li>
 * <li>Flush and shutdown the buffers.</li>
 * <li>Set ERROR_USE_PROTOCOL and SERVER_ACTIVE to false.</li>
 * </ol>
 *
 * NOTES
 *   This function needs to be reentrant since a call to exit() can cause a
 *   call to this function, which can then be interrupted by a signal, which
 *   can cause a second call to this function.
 *
 * GLOBALS
 *   buf_from_net		The input buffer which brings data from the
 *   				CVS client.
 *   buf_to_net			The output buffer which moves data to the CVS
 *   				client.
 *   error_use_protocol		Set when the server parent process is active.
 *   				Cleared for the server child processes.
 *   dont_delete_temp		Set when a core dump of a child process is
 *   				detected so that the core and related data may
 *   				be preserved.
 *   noexec			Whether we are supposed to change the disk.
 *   orig_server_temp_dir	The temporary directory we created within
 *   				Tmpdir for our duplicate of the client
 *   				workspace.
 *
 * INPUTS
 *   None.
 *
 * ERRORS
 *   Problems encountered during the cleanup, for instance low memory or
 *   problems deleting the temp files and directories, can cause the error
 *   function to be called, which might call exit.  If exit gets called in this
 *   manner. this routine will not complete, but the other exit handlers
 *   registered via atexit() will still run.
 *
 * RETURNS
 *   Nothing.
 */
void
server_cleanup (void)
{
    TRACE (TRACE_FUNCTION, "server_cleanup()");

    assert (server_active);

    /* FIXME: Do not perform buffered I/O from an interrupt handler like
     * this (via error).  However, I'm leaving the error-calling code there
     * in the hope that on the rare occasion the error call is actually made
     * (e.g., a fluky I/O error or permissions problem prevents the deletion
     * of a just-created file) reentrancy won't be an issue.
     */

    /* We don't want to be interrupted during calls which set globals to NULL,
     * but we know that by the time we reach this function, interrupts have
     * already been blocked.
     */

    /* Since we install this function in an atexit() handler before forking,
     * reuse the ERROR_USE_PROTOCOL flag, which we know is only set in the
     * parent server process, to avoid cleaning up the temp space multiple
     * times.  Skip the buf_to_net checks too as an optimization since we know
     * they will be set to NULL in the child process anyhow.
     */
    if (error_use_protocol)
    {
	if (buf_to_net != NULL)
	{
	    int status;

	    /* Since we're done, go ahead and put BUF_TO_NET back into blocking
	     * mode and send any pending output.  In the usual case there won't
	     * won't be any, but there might be if an error occured.
	     */

	    set_block (buf_to_net);
	    buf_flush (buf_to_net, 1);

	    /* Next we shut down BUF_FROM_NET.  That will pick up the checksum
	     * generated when the client shuts down its buffer.  Then, after we
	     * have generated any final output, we shut down BUF_TO_NET.
	     */

	    /* SIG_beginCrSect(); */
	    if (buf_from_net)
	    {
		status = buf_shutdown (buf_from_net);
		if (status != 0)
		    error (0, status, "shutting down buffer from client");
		buf_free (buf_from_net);
		buf_from_net = NULL;
	    }
	    /* SIG_endCrSect(); */
	}

	if (!dont_delete_temp)
	{
	    int save_noexec;

	    /* What a bogus kludge.  This disgusting code makes all kinds of
	       assumptions about SunOS, and is only for a bug in that system.
	       So only enable it on Suns.  */
#ifdef SUNOS_KLUDGE
	    if (command_pid > 0)
	    {
		/* To avoid crashes on SunOS due to bugs in SunOS tmpfs
		 * triggered by the use of rename() in RCS, wait for the
		 * subprocess to die.  Unfortunately, this means draining
		 * output while waiting for it to unblock the signal we sent
		 * it.  Yuck!
		 */
		int status;
		pid_t r;

		signal (SIGCHLD, wait_sig);
		/* Perhaps SIGTERM would be more correct.  But the child
		   process will delay the SIGINT delivery until its own
		   children have exited.  */
		kill (command_pid, SIGINT);
		/* The caller may also have sent a signal to command_pid, so
		 * always try waiting.  First, though, check and see if it's
		 * still there....
		 */
	    do_waitpid:
		r = waitpid (command_pid, &status, WNOHANG);
		if (r == 0)
		    ;
		else if (r == command_pid)
		    command_pid_is_dead++;
		else if (r == -1)
		    switch (errno)
		    {
			case ECHILD:
			    command_pid_is_dead++;
			    break;
			case EINTR:
			    goto do_waitpid;
		    }
		else
		    /* waitpid should always return one of the above values */
		    abort ();
		while (!command_pid_is_dead)
		{
		    struct timeval timeout;
		    struct fd_set_wrapper readfds;
		    char buf[100];
		    int i;

		    /* Use a non-zero timeout to avoid eating up CPU cycles.  */
		    timeout.tv_sec = 2;
		    timeout.tv_usec = 0;
		    readfds = command_fds_to_drain;
		    switch (select (max_command_fd + 1, &readfds.fds,
				    NULL, NULL &timeout))
		    {
			case -1:
			    if (errno != EINTR)
				abort ();
			case 0:
			    /* timeout */
			    break;
			case 1:
			    for (i = 0; i <= max_command_fd; i++)
			    {
				if (!FD_ISSET (i, &readfds.fds))
				    continue;
				/* this fd is non-blocking */
				while (read (i, buf, sizeof (buf)) >= 1)
				    ;
			    }
			    break;
			default:
			    abort ();
		    }
		}
	    }
#endif /* SUNOS_KLUDGE */

	    /* Make sure our working directory isn't inside the tree we're
	       going to delete.  */
	    CVS_CHDIR (get_cvs_tmp_dir ());

	    /* Temporarily clear noexec, so that we clean up our temp directory
	       regardless of it (this could more cleanly be handled by moving
	       the noexec check to all the unlink_file_dir callers from
	       unlink_file_dir itself).  */
	    save_noexec = noexec;

	    /* SIG_beginCrSect(); */
	    noexec = 0;
	    unlink_file_dir (orig_server_temp_dir);
	    noexec = save_noexec;
	    /* SIG_endCrSect(); */
	} /* !dont_delete_temp */

	/* SIG_beginCrSect(); */
	if (buf_to_net != NULL)
	{
	    /* Save BUF_TO_NET and set the global pointer to NULL so that any
	     * error messages generated during shutdown go to the syslog rather
	     * than getting lost.
	     */
	    struct buffer *buf_to_net_save = buf_to_net;
	    buf_to_net = NULL;

	    (void) buf_flush (buf_to_net_save, 1);
	    (void) buf_shutdown (buf_to_net_save);
	    buf_free (buf_to_net_save);
	    error_use_protocol = 0;
	}
	/* SIG_endCrSect(); */
    }

    server_active = 0;
}



#ifdef PROXY_SUPPORT
size_t MaxProxyBufferSize = (size_t)(8 * 1024 * 1024); /* 8 megabytes,
                                                        * by default.
                                                        */
#endif /* PROXY_SUPPORT */

static const char *const server_usage[] =
{
    "Usage: %s %s [-c config-file]\n",
    "\t-c config-file\tPath to an alternative CVS config file.\n",
    "Normally invoked by a cvs client on a remote machine.\n",
    NULL
};



void
parseServerOptions (int argc, char **argv)
{
    int c;

    getoptreset ();
    while ((c = getopt (argc, argv, "+c:")) != -1)
    {
	switch (c)
	{
#ifdef ALLOW_CONFIG_OVERRIDE
	    case 'c':
		if (gConfigPath) free (gConfigPath);
		gConfigPath = xstrdup (optarg);
		break;
#endif
	    case '?':
	    default:
		usage (server_usage);
		break;
	}
    }
}



int
server (int argc, char **argv)
{
    char *error_prog_name;		/* Used in error messages */

    if (argc == -1)
	usage (server_usage);

    /* Options were pre-parsed in main.c.  */

    /*
     * Set this in .bashrc if you want to give yourself time to attach
     * to the subprocess with a debugger.
     */
    if (getenv ("CVS_PARENT_SERVER_SLEEP"))
    {
	int secs = atoi (getenv ("CVS_PARENT_SERVER_SLEEP"));
	TRACE (TRACE_DATA, "Sleeping CVS_PARENT_SERVER_SLEEP (%d) seconds",
	       secs);
	sleep (secs);
    }
    else
	TRACE (TRACE_DATA, "CVS_PARENT_SERVER_SLEEP not set.");

    /* pserver_authenticate_connection () (called from main ()) can initialize
     * these.
     */
    if (!buf_to_net)
    {
	buf_to_net = fd_buffer_initialize (STDOUT_FILENO, 0, NULL, false,
					   outbuf_memory_error);
	buf_from_net = fd_buffer_initialize (STDIN_FILENO, 0, NULL, true,
					     outbuf_memory_error);
    }

    setup_logfiles ("CVS_SERVER_LOG", &buf_to_net, &buf_from_net);

#ifdef PROXY_SUPPORT
    /* We have to set up the recording for all servers.  Until we receive the
     * `Root' request and load CVSROOT/config, we can't tell if we are a
     * secondary or primary.
     */
    {
	/* Open the secondary log.  */
	buf_from_net = log_buffer_initialize (buf_from_net, NULL,
# ifdef PROXY_SUPPORT
					      true,
					      config
						? config->MaxProxyBufferSize
						: MaxProxyBufferSize,
# endif /* PROXY_SUPPORT */
					      true, outbuf_memory_error);
	proxy_log = buf_from_net;

	/* And again for the out log.  */
	buf_to_net = log_buffer_initialize (buf_to_net, NULL,
# ifdef PROXY_SUPPORT
					    true,
					    config
					      ? config->MaxProxyBufferSize
					      : MaxProxyBufferSize,
# endif /* PROXY_SUPPORT */
					    false, outbuf_memory_error);
	proxy_log_out = buf_to_net;
    }
#endif /* PROXY_SUPPORT */

    saved_output = buf_nonio_initialize (outbuf_memory_error);
    saved_outerr = buf_nonio_initialize (outbuf_memory_error);

    /* Since we're in the server parent process, error should use the
       protocol to report error messages.  */
    error_use_protocol = 1;

    /* Now initialize our argument vector (for arguments from the client).  */

    /* Small for testing.  */
    argument_vector_size = 1;
    argument_vector = xmalloc (argument_vector_size * sizeof (char *));
    argument_count = 1;
    /* This gets printed if the client supports an option which the
       server doesn't, causing the server to print a usage message.
       FIXME: just a nit, I suppose, but the usage message the server
       prints isn't literally true--it suggests "cvs server" followed
       by options which are for a particular command.  Might be nice to
       say something like "client apparently supports an option not supported
       by this server" or something like that instead of usage message.  */
    error_prog_name = xmalloc (strlen (program_name) + 8);
    sprintf(error_prog_name, "%s server", program_name);
    argument_vector[0] = error_prog_name;

    while (1)
    {
	char *cmd, *orig_cmd;
	struct request *rq;
	int status;

	status = buf_read_line (buf_from_net, &cmd, NULL);
	if (status == -2)
	{
	    buf_output0 (buf_to_net, "E Fatal server error, aborting.\n\
error ENOMEM Virtual memory exhausted.\n");
	    break;
	}
	if (status != 0)
	    break;

	orig_cmd = cmd;
	for (rq = requests; rq->name != NULL; ++rq)
	    if (strncmp (cmd, rq->name, strlen (rq->name)) == 0)
	    {
		int len = strlen (rq->name);
		if (cmd[len] == '\0')
		    cmd += len;
		else if (cmd[len] == ' ')
		    cmd += len + 1;
		else
		    /*
		     * The first len characters match, but it's a different
		     * command.  e.g. the command is "cooperate" but we matched
		     * "co".
		     */
		    continue;

		if (!(rq->flags & RQ_ROOTLESS)
		    && current_parsed_root == NULL)
		{
		    if (alloc_pending (80))
			sprintf (pending_error_text,
				 "E Protocol error: Root request missing");
		}
		else
		{
		    if (config && config->MinCompressionLevel && !gzip_level
			&& !(rq->flags & RQ_ROOTLESS))
		    {
			/* This is a rootless request, a minimum compression
			 * level has been configured, and no compression has
			 * been requested by the client.
			 */
			if (alloc_pending (80 + strlen (program_name)))
			    sprintf (pending_error_text,
"E %s [server aborted]: Compression must be used with this server.",
				     program_name);
		    }
		    (*rq->func) (cmd);
		}
		break;
	    }
	if (rq->name == NULL)
	{
	    if (!print_pending_error ())
	    {
		buf_output0 (buf_to_net, "error  unrecognized request `");
		buf_output0 (buf_to_net, cmd);
		buf_append_char (buf_to_net, '\'');
		buf_append_char (buf_to_net, '\n');
	    }
	}
	free (orig_cmd);
    }

    free (error_prog_name);

    /* We expect the client is done talking to us at this point.  If there is
     * any data in the buffer or on the network pipe, then something we didn't
     * prepare for is happening.
     */
    if (!buf_empty (buf_from_net))
    {
	/* Try to send the error message to the client, but also syslog it, in
	 * case the client isn't listening anymore.
	 */
#ifdef HAVE_SYSLOG_H
	/* FIXME: Can the IP address of the connecting client be retrieved
	 * and printed here?
	 */
	syslog (LOG_DAEMON | LOG_ERR, "Dying gasps received from client.");
#endif /* HAVE_SYSLOG_H */
	error (0, 0, "Dying gasps received from client.");
    }

#ifdef HAVE_PAM
    if (pamh)
    {
        int retval;

        retval = pam_close_session (pamh, 0);
# ifdef HAVE_SYSLOG_H
        if (retval != PAM_SUCCESS)
            syslog (LOG_DAEMON | LOG_ERR, 
                    "PAM close session error: %s",
                    pam_strerror (pamh, retval));
# endif /* HAVE_SYSLOG_H */

        retval = pam_end (pamh, retval);
# ifdef HAVE_SYSLOG_H
        if (retval != PAM_SUCCESS)
            syslog (LOG_DAEMON | LOG_ERR, 
                    "PAM failed to release authenticator, error: %s",
                    pam_strerror (pamh, retval));
# endif /* HAVE_SYSLOG_H */
    }
#endif /* HAVE_PAM */

    /* server_cleanup() will be called on a normal exit and close the buffers
     * explicitly.
     */
    return 0;
}



#if defined (HAVE_KERBEROS) || defined (AUTH_SERVER_SUPPORT) || defined (HAVE_GSSAPI)
static void
switch_to_user (const char *cvs_username, const char *username)
{
    struct passwd *pw;
    int rc;
#ifdef HAVE_PAM
    int retval;
    char *pam_stage = "open session";

    if (pamh)
    {
        retval = pam_open_session (pamh, 0);
        if (retval == PAM_SUCCESS)
        {
            pam_stage = "get pam user";
            retval = pam_get_item (pamh, PAM_USER, (const void **)&username);
        }

        if (retval != PAM_SUCCESS)
        {
            printf("E PAM %s error: %s\n", pam_stage,
                    pam_strerror (pamh, retval));
            exit (EXIT_FAILURE);
        }
    }
#endif

    pw = getpwnam (username);
    if (pw == NULL)
    {
	/* check_password contains a similar check, so this usually won't be
	   reached unless the CVS user is mapped to an invalid system user.  */

	printf ("E Fatal error, aborting.\n\
error 0 %s: no such system user\n", username);
	exit (EXIT_FAILURE);
    }

    if (pw->pw_uid == 0)
    {
#ifdef HAVE_SYSLOG_H
	    /* FIXME: Can the IP address of the connecting client be retrieved
	     * and printed here?
	     */
	    syslog (LOG_DAEMON | LOG_ALERT,
		    "attempt to root from account: %s", cvs_username
		   );
#endif /* HAVE_SYSLOG_H */
        printf("error 0: root not allowed\n");
	exit (EXIT_FAILURE);
    }

#if HAVE_INITGROUPS
    if (initgroups (pw->pw_name, pw->pw_gid) < 0
#  ifdef EPERM
	/* At least on the system I tried, initgroups() only works as root.
	   But we do still want to report ENOMEM and whatever other
	   errors initgroups() might dish up.  */
	&& errno != EPERM
#  endif
	)
    {
	/* This could be a warning, but I'm not sure I see the point
	   in doing that instead of an error given that it would happen
	   on every connection.  We could log it somewhere and not tell
	   the user.  But at least for now make it an error.  */
	printf ("error 0 initgroups failed: %s\n", strerror (errno));
	exit (EXIT_FAILURE);
    }
#endif /* HAVE_INITGROUPS */

#ifdef HAVE_PAM
    if (pamh)
    {
        retval = pam_setcred (pamh, PAM_ESTABLISH_CRED);
        if (retval != PAM_SUCCESS)
        {
            printf("E PAM reestablish credentials error: %s\n", 
                    pam_strerror (pamh, retval));
            exit (EXIT_FAILURE);
        }
    }
#endif

#ifdef SETXID_SUPPORT
    /* honor the setgid bit iff set*/
    if (getgid() != getegid())
    {
	if (setgid (getegid ()) < 0)
	{
	    /* See comments at setuid call below for more discussion.  */
	    printf ("error 0 setgid failed: %s\n", strerror (errno));
	    exit (EXIT_FAILURE);
	}
    }
    else
#endif
    {
	if (setgid (pw->pw_gid) < 0)
	{
	    /* See comments at setuid call below for more discussion.  */
	    printf ("error 0 setgid failed: %s\n", strerror (errno));
#ifdef HAVE_SYSLOG_H
	    syslog (LOG_DAEMON | LOG_ERR,
		    "setgid to %d failed (%m): real %d/%d, effective %d/%d ",
		    pw->pw_gid, getuid(), getgid(), geteuid(), getegid());
#endif /* HAVE_SYSLOG_H */
	    exit (EXIT_FAILURE);
	}
    }

#ifdef SETXID_SUPPORT
    /* Honor the setuid bit iff set. */
    if (getuid() != geteuid())
	rc = setuid (geteuid ());
    else
#endif
	rc = setuid (pw->pw_uid);
    if (rc < 0)
    {
	/* Note that this means that if run as a non-root user,
	   CVSROOT/passwd must contain the user we are running as
	   (e.g. "joe:FsEfVcu:cvs" if run as "cvs" user).  This seems
	   cleaner than ignoring the error like CVS 1.10 and older but
	   it does mean that some people might need to update their
	   CVSROOT/passwd file.  */
	printf ("error 0 setuid failed: %s\n", strerror (errno));
#ifdef HAVE_SYSLOG_H
	    syslog (LOG_DAEMON | LOG_ERR,
		    "setuid to %d failed (%m): real %d/%d, effective %d/%d ",
		    pw->pw_uid, getuid(), getgid(), geteuid(), getegid());
#endif /* HAVE_SYSLOG_H */
	exit (EXIT_FAILURE);
    }

    /* We don't want our umask to change file modes.  The modes should
       be set by the modes used in the repository, and by the umask of
       the client.  */
    umask (0);

#ifdef AUTH_SERVER_SUPPORT
    /* Make sure our CVS_Username has been set. */
    if (CVS_Username == NULL)
	CVS_Username = xstrdup (username);
#endif

    /* Set LOGNAME, USER and CVS_USER in the environment, in case they
       are already set to something else.  */
    setenv ("LOGNAME", username, 1);
    setenv ("USER", username, 1);
# ifdef AUTH_SERVER_SUPPORT
    setenv ("CVS_USER", CVS_Username, 1);
# endif
}
#endif

#ifdef AUTH_SERVER_SUPPORT

extern char *crypt (const char *, const char *);


/*
 * 0 means no entry found for this user.
 * 1 means entry found and password matches (or found password is empty)
 * 2 means entry found, but password does not match.
 *
 * If 1, host_user_ptr will be set to point at the system
 * username (i.e., the "real" identity, which may or may not be the
 * CVS username) of this user; caller may free this.  Global
 * CVS_Username will point at an allocated copy of cvs username (i.e.,
 * the username argument below).
 * kff todo: FIXME: last sentence is not true, it applies to caller.
 */
static int
check_repository_password (char *username, char *password, char *repository, char **host_user_ptr)
{
    int retval = 0;
    FILE *fp;
    char *filename;
    char *linebuf = NULL;
    size_t linebuf_len;
    int found_it = 0;
    int namelen;

    /* We don't use current_parsed_root->directory because it hasn't been
     * set yet -- our `repository' argument came from the authentication
     * protocol, not the regular CVS protocol.
     */

    filename = xmalloc (strlen (repository)
			+ 1
			+ strlen (CVSROOTADM)
			+ 1
			+ strlen (CVSROOTADM_PASSWD)
			+ 1);

    (void) sprintf (filename, "%s/%s/%s", repository,
		    CVSROOTADM, CVSROOTADM_PASSWD);

    fp = CVS_FOPEN (filename, "r");
    if (fp == NULL)
    {
	if (!existence_error (errno))
	    error (0, errno, "cannot open %s", filename);
	free (filename);
	return 0;
    }

    /* Look for a relevant line -- one with this user's name. */
    namelen = strlen (username);
    while (getline (&linebuf, &linebuf_len, fp) >= 0)
    {
	if ((strncmp (linebuf, username, namelen) == 0)
	    && (linebuf[namelen] == ':'))
	{
	    found_it = 1;
	    break;
	}
    }
    if (ferror (fp))
	error (0, errno, "cannot read %s", filename);
    if (fclose (fp) < 0)
	error (0, errno, "cannot close %s", filename);

    /* If found_it, then linebuf contains the information we need. */
    if (found_it)
    {
	char *found_password, *host_user_tmp;
	char *non_cvsuser_portion;

	/* We need to make sure lines such as
	 *
	 *    "username::sysuser\n"
	 *    "username:\n"
	 *    "username:  \n"
	 *
	 * all result in a found_password of NULL, but we also need to
	 * make sure that
	 *
	 *    "username:   :sysuser\n"
	 *    "username: <whatever>:sysuser\n"
	 *
	 * continues to result in an impossible password.  That way,
	 * an admin would be on safe ground by going in and tacking a
	 * space onto the front of a password to disable the account
	 * (a technique some people use to close accounts
	 * temporarily).
	 */

	/* Make `non_cvsuser_portion' contain everything after the CVS
	   username, but null out any final newline. */
	non_cvsuser_portion = linebuf + namelen;
	strtok (non_cvsuser_portion, "\n");

	/* If there's a colon now, we just want to inch past it. */
	if (strchr (non_cvsuser_portion, ':') == non_cvsuser_portion)
	    non_cvsuser_portion++;

	/* Okay, after this conditional chain, found_password and
	   host_user_tmp will have useful values: */

	if ((non_cvsuser_portion == NULL)
	    || (strlen (non_cvsuser_portion) == 0)
	    || ((strspn (non_cvsuser_portion, " \t"))
		== strlen (non_cvsuser_portion)))
	{
	    found_password = NULL;
	    host_user_tmp = NULL;
	}
	else if (strncmp (non_cvsuser_portion, ":", 1) == 0)
	{
	    found_password = NULL;
	    host_user_tmp = non_cvsuser_portion + 1;
	    if (strlen (host_user_tmp) == 0)
		host_user_tmp = NULL;
	}
	else
	{
	    found_password = strtok (non_cvsuser_portion, ":");
	    host_user_tmp = strtok (NULL, ":");
	}

	/* Of course, maybe there was no system user portion... */
	if (host_user_tmp == NULL)
	    host_user_tmp = username;

	/* Verify blank passwords directly, otherwise use crypt(). */
	if ((found_password == NULL)
	    || ((strcmp (found_password, crypt (password, found_password))
		 == 0)))
	{
	    /* Give host_user_ptr permanent storage. */
	    *host_user_ptr = xstrdup (host_user_tmp);
	    retval = 1;
	}
	else
	{
#ifdef LOG_AUTHPRIV
	syslog (LOG_AUTHPRIV | LOG_NOTICE,
		"password mismatch for %s in %s: %s vs. %s", username,
		repository, crypt(password, found_password), found_password);
#endif
	    *host_user_ptr = NULL;
	    retval	 = 2;
	}
    }
    else     /* Didn't find this user, so deny access. */
    {
	*host_user_ptr = NULL;
	retval = 0;
    }

    free (filename);
    if (linebuf)
	free (linebuf);

    return retval;
}

#ifdef HAVE_PAM

static int
cvs_pam_conv (int num_msg, const struct pam_message **msg,
              struct pam_response **resp, void *appdata_ptr)
{
    int i;
    struct pam_response *response;

    assert (msg && resp);

    response = xnmalloc (num_msg, sizeof (struct pam_response));
    memset (response, 0, num_msg * sizeof (struct pam_response));

    for (i = 0; i < num_msg; i++)
    {
	switch (msg[i]->msg_style) 
	{
	    /* PAM wants a username */
	    case PAM_PROMPT_ECHO_ON:
                assert (pam_username != 0);
		response[i].resp = xstrdup (pam_username);
		break;
	    /* PAM wants a password */
	    case PAM_PROMPT_ECHO_OFF:
                assert (pam_password != 0);
		response[i].resp = xstrdup (pam_password);
		break;
	    case PAM_ERROR_MSG:
	    case PAM_TEXT_INFO:
		printf ("E %s\n", msg[i]->msg);
		break;
	    /* PAM wants something we don't understand - bail out */
	    default:
		goto cleanup;
	}
    }

    *resp = response;
    return PAM_SUCCESS;

cleanup:
    for (i = 0; i < num_msg; i++)
    {
	if (response[i].resp)
	{
	    free (response[i].resp);
	    response[i].resp = 0;
	}
    }
    free (response);
    return PAM_CONV_ERR;
}

static int
check_pam_password (char **username, char *password)
{
    int retval, err;
    struct pam_conv conv = { cvs_pam_conv, 0 };
    char *pam_stage = "start";

    pam_username = *username;
    pam_password = password;

    retval = pam_start (PAM_SERVICE_NAME, *username, &conv, &pamh);

    /* sets a dummy tty name which pam modules can check for */
    if (retval == PAM_SUCCESS)
    {
        pam_stage = "set dummy tty";
        retval = pam_set_item (pamh, PAM_TTY, PAM_SERVICE_NAME);
    }

    if (retval == PAM_SUCCESS)
    {
	pam_stage = "authenticate";
	retval = pam_authenticate (pamh, 0);
    }

    if (retval == PAM_SUCCESS)
    {
	pam_stage = "account";
	retval = pam_acct_mgmt (pamh, 0);
    }

    if (retval == PAM_SUCCESS)
    {
        pam_stage = "get pam user";
        retval = pam_get_item (pamh, PAM_USER, (const void **)username);
    }

    if (retval != PAM_SUCCESS)
	printf ("E PAM %s error: %s\n", pam_stage, pam_strerror (pamh, retval));

    /* clear the pointers to make sure we don't use these references again */
    pam_username = 0;
    pam_password = 0; 

    return retval == PAM_SUCCESS;       /* indicate success */
}
#endif

static int
check_system_password (char *username, char *password)
{
    char *found_passwd = NULL;
    struct passwd *pw;
#ifdef HAVE_GETSPNAM
    {
	struct spwd *spw;

	spw = getspnam (username);
	if (spw != NULL)
	    found_passwd = spw->sp_pwdp;
    }
#endif

    if (found_passwd == NULL && (pw = getpwnam (username)) != NULL)
	found_passwd = pw->pw_passwd;

    if (found_passwd == NULL)
    {
	printf ("E Fatal error, aborting.\n\
error 0 %s: no such user\n", username);

	exit (EXIT_FAILURE);
    }

    /* Allow for dain bramaged HPUX passwd aging
     *  - Basically, HPUX adds a comma and some data
     *    about whether the passwd has expired or not
     *    on the end of the passwd field.
     *  - This code replaces the ',' with '\0'.
     *
     * FIXME - our workaround is brain damaged too.  I'm
     * guessing that HPUX WANTED other systems to think the
     * password was wrong so logins would fail if the
     * system didn't handle expired passwds and the passwd
     * might be expired.  I think the way to go here
     * is with PAM.
     */
    strtok (found_passwd, ",");

    if (*found_passwd)
    {
	/* user exists and has a password */
	if (strcmp (found_passwd, crypt (password, found_passwd)) == 0)
	    return 1;
	else
	{
#ifdef LOG_AUTHPRIV
	    syslog (LOG_AUTHPRIV | LOG_NOTICE,
		    "password mismatch for %s: %s vs. %s", username,
		    crypt(password, found_passwd), found_passwd);
#endif
	    return 0;
	}
    }

#ifdef LOG_AUTHPRIV
    syslog (LOG_AUTHPRIV | LOG_NOTICE,
	    "user %s authenticated because of blank system password",
	    username);
#endif
    return 1;
}



/* Return a hosting username if password matches, else NULL. */
static char *
check_password (char *username, char *password, char *repository)
{
    int rc;
    char *host_user = NULL;

    /* First we see if this user has a password in the CVS-specific
       password file.  If so, that's enough to authenticate with.  If
       not, we'll check /etc/passwd or maybe whatever is configured via PAM. */

    rc = check_repository_password (username, password, repository,
				    &host_user);

    if (rc == 2)
	return NULL;

    if (rc == 1)
	/* host_user already set by reference, so just return. */
	goto handle_return;

    assert (rc == 0);

    if (!config->system_auth)
    {
	/* Note that the message _does_ distinguish between the case in
	   which we check for a system password and the case in which
	   we do not.  It is a real pain to track down why it isn't
	   letting you in if it won't say why, and I am not convinced
	   that the potential information disclosure to an attacker
	   outweighs this.  */
	printf ("error 0 no such user %s in CVSROOT/passwd\n", username);

	exit (EXIT_FAILURE);
    }

    /* No cvs password found, so try /etc/passwd. */
#ifdef HAVE_PAM
    if (check_pam_password (&username, password))
#else /* !HAVE_PAM */
    if (check_system_password (username, password))
#endif /* HAVE_PAM */
	host_user = xstrdup (username);
    else
	host_user = NULL;

#ifdef LOG_AUTHPRIV
    if (!host_user)
	syslog (LOG_AUTHPRIV | LOG_NOTICE,
		"login refused for %s: user has no password", username);
#endif

handle_return:
    if (host_user)
    {
	/* Set CVS_Username here, in allocated space.
	   It might or might not be the same as host_user. */
	CVS_Username = xmalloc (strlen (username) + 1);
	strcpy (CVS_Username, username);
    }

    return host_user;
}

#endif /* AUTH_SERVER_SUPPORT */

#if defined (AUTH_SERVER_SUPPORT) || defined (HAVE_GSSAPI)

static void
pserver_read_line (char **tmp, size_t *tmp_len)
{
    int status;

    /* Make sure the protocol starts off on the right foot... */
    status = buf_read_short_line (buf_from_net, tmp, tmp_len, PATH_MAX);
    if (status == -1)
    {
# ifdef HAVE_SYSLOG_H
	syslog (LOG_DAEMON | LOG_NOTICE,
	        "unexpected EOF encountered during authentication");
# endif /* HAVE_SYSLOG_H */
	error (1, 0, "unexpected EOF encountered during authentication");
    }
    if (status == -2)
	status = ENOMEM;
    if (status != 0)
    {
# ifdef HAVE_SYSLOG_H
	syslog (LOG_DAEMON | LOG_NOTICE,
                "error reading from net while validating pserver");
# endif /* HAVE_SYSLOG_H */
	error (1, status, "error reading from net while validating pserver");
    }
}

/* Read username and password from client (i.e., stdin).
   If correct, then switch to run as that user and send an ACK to the
   client via stdout, else send NACK and die. */
void
pserver_authenticate_connection (void)
{
    char *tmp;
#ifdef AUTH_SERVER_SUPPORT
    char *repository = NULL;
    char *username = NULL;
    char *password = NULL;

    char *host_user;
    char *descrambled_password;
#endif /* AUTH_SERVER_SUPPORT */
    int verify_and_exit = 0;

    /* The Authentication Protocol.  Client sends:
     *
     *   BEGIN AUTH REQUEST\n
     *   <REPOSITORY>\n
     *   <USERNAME>\n
     *   <PASSWORD>\n
     *   END AUTH REQUEST\n
     *
     * Server uses above information to authenticate, then sends
     *
     *   I LOVE YOU\n
     *
     * if it grants access, else
     *
     *   I HATE YOU\n
     *
     * if it denies access (and it exits if denying).
     *
     * When the client is "cvs login", the user does not desire actual
     * repository access, but would like to confirm the password with
     * the server.  In this case, the start and stop strings are
     *
     *   BEGIN VERIFICATION REQUEST\n
     *
     *	    and
     *
     *   END VERIFICATION REQUEST\n
     *
     * On a verification request, the server's responses are the same
     * (with the obvious semantics), but it exits immediately after
     * sending the response in both cases.
     *
     * Why is the repository sent?  Well, note that the actual
     * client/server protocol can't start up until authentication is
     * successful.  But in order to perform authentication, the server
     * needs to look up the password in the special CVS passwd file,
     * before trying /etc/passwd.  So the client transmits the
     * repository as part of the "authentication protocol".  The
     * repository will be redundantly retransmitted later, but that's no
     * big deal.
     */

    /* Initialize buffers.  */
    buf_to_net = fd_buffer_initialize (STDOUT_FILENO, 0, NULL, false,
				       outbuf_memory_error);
    buf_from_net = fd_buffer_initialize (STDIN_FILENO, 0, NULL, true,
					 outbuf_memory_error);

#ifdef SO_KEEPALIVE
    /* Set SO_KEEPALIVE on the socket, so that we don't hang forever
       if the client dies while we are waiting for input.  */
    {
	int on = 1;

	if (setsockopt (STDIN_FILENO, SOL_SOCKET, SO_KEEPALIVE,
			&on, sizeof on) < 0)
	{
# ifdef HAVE_SYSLOG_H
	    syslog (LOG_DAEMON | LOG_ERR, "error setting KEEPALIVE: %m");
# endif /* HAVE_SYSLOG_H */
	}
    }
#endif

    /* Make sure the protocol starts off on the right foot... */
    pserver_read_line (&tmp, NULL);

    if (strcmp (tmp, "BEGIN VERIFICATION REQUEST") == 0)
	verify_and_exit = 1;
    else if (strcmp (tmp, "BEGIN AUTH REQUEST") == 0)
	;
    else if (strcmp (tmp, "BEGIN GSSAPI REQUEST") == 0)
    {
#ifdef HAVE_GSSAPI
	free (tmp);
	gserver_authenticate_connection ();
	return;
#else
	error (1, 0, "GSSAPI authentication not supported by this server");
#endif
    }
    else
	error (1, 0, "bad auth protocol start: %s", tmp);

#ifndef AUTH_SERVER_SUPPORT

    error (1, 0, "Password authentication not supported by this server");

#else /* AUTH_SERVER_SUPPORT */

    free (tmp);

    /* Get the three important pieces of information in order. */
    /* See above comment about error handling.  */
    pserver_read_line (&repository, NULL);
    pserver_read_line (&username, NULL);
    pserver_read_line (&password, NULL);

    /* ... and make sure the protocol ends on the right foot. */
    /* See above comment about error handling.  */
    pserver_read_line (&tmp, NULL);
    if (strcmp (tmp,
		verify_and_exit ?
		"END VERIFICATION REQUEST" : "END AUTH REQUEST")
	!= 0)
    {
	error (1, 0, "bad auth protocol end: %s", tmp);
    }
    free (tmp);

    if (!root_allow_ok (repository))
    {
	error (1, 0, "%s: no such repository", repository);
# ifdef HAVE_SYSLOG_H
	syslog (LOG_DAEMON | LOG_NOTICE, "login refused for %s", repository);
# endif /* HAVE_SYSLOG_H */
	goto i_hate_you;
    }

    /* OK, now parse the config file, so we can use it to control how
       to check passwords.  If there was an error parsing the config
       file, parse_config already printed an error.  We keep going.
       Why?  Because if we didn't, then there would be no way to check
       in a new CVSROOT/config file to fix the broken one!  */
    config = get_root_allow_config (repository, gConfigPath);

    /* We need the real cleartext before we hash it. */
    descrambled_password = descramble (password);
    host_user = check_password (username, descrambled_password, repository);
    if (host_user == NULL)
    {
# ifdef HAVE_SYSLOG_H
	syslog (LOG_DAEMON | LOG_NOTICE, "login failure (for %s)", repository);
# endif /* HAVE_SYSLOG_H */
	memset (descrambled_password, 0, strlen (descrambled_password));
	free (descrambled_password);
    i_hate_you:
	buf_output0 (buf_to_net, "I HATE YOU\n");
	buf_flush (buf_to_net, true);

	/* Don't worry about server_cleanup, server_active isn't set
	   yet.  */
	exit (EXIT_FAILURE);
    }
    memset (descrambled_password, 0, strlen (descrambled_password));
    free (descrambled_password);

    /* Don't go any farther if we're just responding to "cvs login". */
    if (verify_and_exit)
    {
	buf_output0 (buf_to_net, "I LOVE YOU\n");
	buf_flush (buf_to_net, true);
	exit (EXIT_SUCCESS);
    }

    /* Set Pserver_Repos so that we can check later that the same
       repository is sent in later client/server protocol. */
    Pserver_Repos = xmalloc (strlen (repository) + 1);
    strcpy (Pserver_Repos, repository);

    /* Switch to run as this user. */
    switch_to_user (username, host_user);
    free (host_user);
    free (repository);
    free (username);
    free (password);

    buf_output0 (buf_to_net, "I LOVE YOU\n");
    buf_flush (buf_to_net, true);
#endif /* AUTH_SERVER_SUPPORT */
}

#endif /* AUTH_SERVER_SUPPORT || HAVE_GSSAPI */


#ifdef HAVE_KERBEROS
void
kserver_authenticate_connection( void )
{
    int status;
    char instance[INST_SZ];
    struct sockaddr_storage peer;
    struct sockaddr_storage laddr;
    int plen, llen;
    KTEXT_ST ticket;
    AUTH_DAT auth;
    char version[KRB_SENDAUTH_VLEN];
    char user[ANAME_SZ];

    strcpy (instance, "*");
    plen = sizeof peer;
    llen = sizeof laddr;
    if (getpeername (STDIN_FILENO, (struct sockaddr *) &peer, &plen) < 0
	|| getsockname (STDIN_FILENO, (struct sockaddr *) &laddr,
			&llen) < 0)
    {
	printf ("E Fatal error, aborting.\n\
error %s getpeername or getsockname failed\n", strerror (errno));

	exit (EXIT_FAILURE);
    }

#ifdef SO_KEEPALIVE
    /* Set SO_KEEPALIVE on the socket, so that we don't hang forever
       if the client dies while we are waiting for input.  */
    {
	int on = 1;

	if (setsockopt (STDIN_FILENO, SOL_SOCKET, SO_KEEPALIVE,
			   (char *) &on, sizeof on) < 0)
	{
# ifdef HAVE_SYSLOG_H
	    syslog (LOG_DAEMON | LOG_ERR, "error setting KEEPALIVE: %m");
# endif /* HAVE_SYSLOG_H */
	}
    }
#endif

    status = krb_recvauth (KOPT_DO_MUTUAL, STDIN_FILENO, &ticket, "rcmd",
			   instance, &peer, &laddr, &auth, "", sched,
			   version);
    if (status != KSUCCESS)
    {
	printf ("E Fatal error, aborting.\n\
error 0 kerberos: %s\n", krb_get_err_text(status));

	exit (EXIT_FAILURE);
    }

    memcpy (kblock, auth.session, sizeof (C_Block));

    /* Get the local name.  */
    status = krb_kntoln (&auth, user);
    if (status != KSUCCESS)
    {
	printf ("E Fatal error, aborting.\n"
		"error 0 kerberos: can't get local name: %s\n",
		krb_get_err_text(status));

	exit (EXIT_FAILURE);
    }

    /* Switch to run as this user. */
    switch_to_user ("Kerberos 4", user);
}
#endif /* HAVE_KERBEROS */



# ifdef HAVE_GSSAPI /* && SERVER_SUPPORT */
/* Authenticate a GSSAPI connection.  This is called from
 * pserver_authenticate_connection, and it handles success and failure
 * the same way.
 *
 * GLOBALS
 *   server_hostname	The name of this host, as set via a call to
 *			xgethostname() in main().
 */
static void
gserver_authenticate_connection (void)
{
    char *hn;
    gss_buffer_desc tok_in, tok_out;
    char buf[1024];
    char *credbuf;
    size_t credbuflen;
    OM_uint32 stat_min, ret;
    gss_name_t server_name, client_name;
    gss_cred_id_t server_creds;
    int nbytes;
    gss_OID mechid;

    hn = canon_host (server_hostname);
    if (!hn)
	error (1, 0, "can't get canonical hostname for `%s': %s",
	       server_hostname, ch_strerror ());

    sprintf (buf, "cvs@%s", hn);
    free (hn);
    tok_in.value = buf;
    tok_in.length = strlen (buf);

    if (gss_import_name (&stat_min, &tok_in, GSS_C_NT_HOSTBASED_SERVICE,
			 &server_name) != GSS_S_COMPLETE)
	error (1, 0, "could not import GSSAPI service name %s", buf);

    /* Acquire the server credential to verify the client's
       authentication.  */
    if (gss_acquire_cred (&stat_min, server_name, 0, GSS_C_NULL_OID_SET,
			  GSS_C_ACCEPT, &server_creds,
			  NULL, NULL) != GSS_S_COMPLETE)
	error (1, 0, "could not acquire GSSAPI server credentials");

    gss_release_name (&stat_min, &server_name);

    /* The client will send us a two byte length followed by that many
       bytes.  */
    if (fread (buf, 1, 2, stdin) != 2)
	error (1, errno, "read of length failed");

    nbytes = ((buf[0] & 0xff) << 8) | (buf[1] & 0xff);
    if (nbytes <= sizeof buf)
    {
        credbuf = buf;
        credbuflen = sizeof buf;
    }
    else
    {
        credbuflen = nbytes;
        credbuf = xmalloc (credbuflen);
    }
    
    if (fread (credbuf, 1, nbytes, stdin) != nbytes)
	error (1, errno, "read of data failed");

    gcontext = GSS_C_NO_CONTEXT;
    tok_in.length = nbytes;
    tok_in.value = credbuf;

    if (gss_accept_sec_context (&stat_min,
				&gcontext,	/* context_handle */
				server_creds,	/* verifier_cred_handle */
				&tok_in,	/* input_token */
				NULL,		/* channel bindings */
				&client_name,	/* src_name */
				&mechid,	/* mech_type */
				&tok_out,	/* output_token */
				&ret,
				NULL,		/* ignore time_rec */
				NULL)		/* ignore del_cred_handle */
	!= GSS_S_COMPLETE)
    {
	error (1, 0, "could not verify credentials");
    }

    /* FIXME: Use Kerberos v5 specific code to authenticate to a user.
       We could instead use an authentication to access mapping.  */
    {
	krb5_context kc;
	krb5_principal p;
	gss_buffer_desc desc;

	krb5_init_context (&kc);
	if (gss_display_name (&stat_min, client_name, &desc,
			      &mechid) != GSS_S_COMPLETE
	    || krb5_parse_name (kc, ((gss_buffer_t) &desc)->value, &p) != 0
	    || krb5_aname_to_localname (kc, p, sizeof buf, buf) != 0
	    || krb5_kuserok (kc, p, buf) != TRUE)
	{
	    error (1, 0, "access denied");
	}
	krb5_free_principal (kc, p);
	krb5_free_context (kc);
    }

    if (tok_out.length != 0)
    {
	char cbuf[2];

	cbuf[0] = (tok_out.length >> 8) & 0xff;
	cbuf[1] = tok_out.length & 0xff;
	if (fwrite (cbuf, 1, 2, stdout) != 2
	    || (fwrite (tok_out.value, 1, tok_out.length, stdout)
		!= tok_out.length))
	    error (1, errno, "fwrite failed");
    }

    switch_to_user ("GSSAPI", buf);

    if (credbuf != buf)
        free (credbuf);

    printf ("I LOVE YOU\n");
    fflush (stdout);
}

# endif /* HAVE_GSSAPI */

#endif /* SERVER_SUPPORT */

#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)

/* This global variable is non-zero if the user requests encryption on
   the command line.  */
int cvsencrypt;

/* This global variable is non-zero if the users requests stream
   authentication on the command line.  */
int cvsauthenticate;

#ifdef ENCRYPTION

#ifdef HAVE_KERBEROS

/* An encryption interface using Kerberos.  This is built on top of a
   packetizing buffer.  */

/* This structure is the closure field of the Kerberos translation
   routines.  */
struct krb_encrypt_data
{
    /* The Kerberos key schedule.  */
    Key_schedule sched;
    /* The Kerberos DES block.  */
    C_Block block;
};



/* Decrypt Kerberos data.  */
static int
krb_encrypt_input( void *fnclosure, const char *input, char *output, int size )
{
    struct krb_encrypt_data *kd = (struct krb_encrypt_data *) fnclosure;
    int tcount;

    des_cbc_encrypt ((char *) input, (char *) output,
		     size, kd->sched, &kd->block, 0);

    /* SIZE is the size of the buffer, which is set by the encryption
       routine.  The packetizing buffer will arrange for the first two
       bytes in the decrypted buffer to be the real (unaligned)
       length.  As a safety check, make sure that the length in the
       buffer corresponds to SIZE.  Note that the length in the buffer
       is just the length of the data.  We must add 2 to account for
       the buffer count itself.  */
    tcount = ((output[0] & 0xff) << 8) + (output[1] & 0xff);
    if (((tcount + 2 + 7) & ~7) != size)
      error (1, 0, "Decryption failure");

    return 0;
}



/* Encrypt Kerberos data.  */
static int
krb_encrypt_output( void *fnclosure, const char *input, char *output,
                    int size, int *translated )
{
    struct krb_encrypt_data *kd = (struct krb_encrypt_data *) fnclosure;
    int aligned;

    /* For security against a known plaintext attack, we should
       initialize any padding bytes to random values.  Instead, we
       just pick up whatever is on the stack, which is at least better
       than using zero.  */

    /* Align SIZE to an 8 byte boundary.  Note that SIZE includes the
       two byte buffer count at the start of INPUT which was added by
       the packetizing buffer.  */
    aligned = (size + 7) & ~7;

    /* We use des_cbc_encrypt rather than krb_mk_priv because the
       latter sticks a timestamp in the block, and krb_rd_priv expects
       that timestamp to be within five minutes of the current time.
       Given the way the CVS server buffers up data, that can easily
       fail over a long network connection.  We trust krb_recvauth to
       guard against a replay attack.  */

    des_cbc_encrypt ((char *) input, (char *) output, aligned,
		     kd->sched, &kd->block, 1);

    *translated = aligned;

    return 0;
}



/* Create a Kerberos encryption buffer.  We use a packetizing buffer
   with Kerberos encryption translation routines.  */
struct buffer *
krb_encrypt_buffer_initialize( struct buffer *buf, int input,
                               Key_schedule sched, C_Block block,
                               void *memory( struct buffer * ) )
{
    struct krb_encrypt_data *kd;

    kd = (struct krb_encrypt_data *) xmalloc (sizeof *kd);
    memcpy (kd->sched, sched, sizeof (Key_schedule));
    memcpy (kd->block, block, sizeof (C_Block));

    return packetizing_buffer_initialize (buf,
					  input ? krb_encrypt_input : NULL,
					  input ? NULL : krb_encrypt_output,
					  kd,
					  memory);
}

#endif /* HAVE_KERBEROS */
#endif /* ENCRYPTION */
#endif /* defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */



/* Output LEN bytes at STR.  If LEN is zero, then output up to (not including)
   the first '\0' byte.  */
void
cvs_output (const char *str, size_t len)
{
    if (len == 0)
	len = strlen (str);
#ifdef SERVER_SUPPORT
    if (error_use_protocol)
    {
	if (buf_to_net)
	{
	    buf_output (saved_output, str, len);
	    buf_copy_lines (buf_to_net, saved_output, 'M');
	}
# if HAVE_SYSLOG_H
	else
	    syslog (LOG_DAEMON | LOG_ERR,
		    "Attempt to write message after close of network buffer.  "
		    "Message was: %s",
		    str);
# endif /* HAVE_SYSLOG_H */
    }
    else if (server_active)
    {
	if (protocol)
	{
	    buf_output (saved_output, str, len);
	    buf_copy_lines (protocol, saved_output, 'M');
	    buf_send_counted (protocol);
	}
# if HAVE_SYSLOG_H
	else
	    syslog (LOG_DAEMON | LOG_ERR,
		    "Attempt to write message before initialization of "
		    "protocol buffer.  Message was: %s",
		    str);
# endif /* HAVE_SYSLOG_H */
    }
    else
#endif
    {
	size_t written;
	size_t to_write = len;
	const char *p = str;

	/* Local users that do 'cvs status 2>&1' on a local repository
	   may see the informational messages out-of-order with the
	   status messages unless we use the fflush (stderr) here. */
	fflush (stderr);

	while (to_write > 0)
	{
	    written = fwrite (p, 1, to_write, stdout);
	    if (written == 0)
		break;
	    p += written;
	    to_write -= written;
	}
    }
}

/* Output LEN bytes at STR in binary mode.  If LEN is zero, then
   output zero bytes.  */

void
cvs_output_binary (char *str, size_t len)
{
#ifdef SERVER_SUPPORT
    if (error_use_protocol || server_active)
    {
	struct buffer *buf;
	char size_text[40];

	if (error_use_protocol)
	    buf = buf_to_net;
	else
	    buf = protocol;

	assert (buf);

	if (!supported_response ("Mbinary"))
	{
	    error (0, 0, "\
this client does not support writing binary files to stdout");
	    return;
	}

	buf_output0 (buf, "Mbinary\012");
	sprintf (size_text, "%lu\012", (unsigned long) len);
	buf_output0 (buf, size_text);

	/* Not sure what would be involved in using buf_append_data here
	   without stepping on the toes of our caller (which is responsible
	   for the memory allocation of STR).  */
	buf_output (buf, str, len);

	if (!error_use_protocol)
	    buf_send_counted (protocol);
    }
    else
#endif
    {
	size_t written;
	size_t to_write = len;
	const char *p = str;
#ifdef USE_SETMODE_STDOUT
	int oldmode;
#endif

	/* Local users that do 'cvs status 2>&1' on a local repository
	   may see the informational messages out-of-order with the
	   status messages unless we use the fflush (stderr) here. */
	fflush (stderr);

#ifdef USE_SETMODE_STDOUT
	/* It is possible that this should be the same ifdef as
	   USE_SETMODE_BINARY but at least for the moment we keep them
	   separate.  Mostly this is just laziness and/or a question
	   of what has been tested where.  Also there might be an
	   issue of setmode vs. _setmode.  */
	/* The Windows doc says to call setmode only right after startup.
	   I assume that what they are talking about can also be helped
	   by flushing the stream before changing the mode.  */
	fflush (stdout);
	oldmode = _setmode (_fileno (stdout), OPEN_BINARY);
	if (oldmode < 0)
	    error (0, errno, "failed to setmode on stdout");
#endif

	while (to_write > 0)
	{
	    written = fwrite (p, 1, to_write, stdout);
	    if (written == 0)
		break;
	    p += written;
	    to_write -= written;
	}
#ifdef USE_SETMODE_STDOUT
	fflush (stdout);
	if (_setmode (_fileno (stdout), oldmode) != OPEN_BINARY)
	    error (0, errno, "failed to setmode on stdout");
#endif
    }
}



/* Like CVS_OUTPUT but output is for stderr not stdout.  */
void
cvs_outerr (const char *str, size_t len)
{
    if (len == 0)
	len = strlen (str);
#ifdef SERVER_SUPPORT
    if (error_use_protocol)
    {
	if (buf_to_net)
	{
	    buf_output (saved_outerr, str, len);
	    buf_copy_lines (buf_to_net, saved_outerr, 'E');
	}
# if HAVE_SYSLOG_H
	else
	    syslog (LOG_DAEMON | LOG_ERR,
		    "Attempt to write error message after close of network "
		    "buffer.  Message was: `%s'",
		    str);
# endif /* HAVE_SYSLOG_H */
    }
    else if (server_active)
    {
	if (protocol)
	{
	    buf_output (saved_outerr, str, len);
	    buf_copy_lines (protocol, saved_outerr, 'E');
	    buf_send_counted (protocol);
	}
# if HAVE_SYSLOG_H
	else
	    syslog (LOG_DAEMON | LOG_ERR,
		    "Attempt to write error message before initialization of "
		    "protocol buffer.  Message was: `%s'",
		    str);
# endif /* HAVE_SYSLOG_H */
    }
    else
#endif
    {
	size_t written;
	size_t to_write = len;
	const char *p = str;

	/* Make sure that output appears in order if stdout and stderr
	   point to the same place.  For the server case this is taken
	   care of by the fact that saved_outerr always holds less
	   than a line.  */
	fflush (stdout);

	while (to_write > 0)
	{
	    written = fwrite (p, 1, to_write, stderr);
	    if (written == 0)
		break;
	    p += written;
	    to_write -= written;
	}
    }
}



/* Flush stderr.  stderr is normally flushed automatically, of course,
   but this function is used to flush information from the server back
   to the client.  */
void
cvs_flusherr (void)
{
#ifdef SERVER_SUPPORT
    if (error_use_protocol)
    {
	/* skip the actual stderr flush in this case since the parent process
	 * on the server should only be writing to stdout anyhow
	 */
	/* Flush what we can to the network, but don't block.  */
	buf_flush (buf_to_net, 0);
    }
    else if (server_active)
    {
	/* make sure stderr is flushed before we send the flush count on the
	 * protocol pipe
	 */
	fflush (stderr);
	/* Send a special count to tell the parent to flush.  */
	buf_send_special_count (protocol, -2);
    }
    else
#endif
	fflush (stderr);
}



/* Make it possible for the user to see what has been written to
   stdout (it is up to the implementation to decide exactly how far it
   should go to ensure this).  */
void
cvs_flushout (void)
{
#ifdef SERVER_SUPPORT
    if (error_use_protocol)
    {
	/* Flush what we can to the network, but don't block.  */
	buf_flush (buf_to_net, 0);
    }
    else if (server_active)
    {
	/* Just do nothing.  This is because the code which
	   cvs_flushout replaces, setting stdout to line buffering in
	   main.c, didn't get called in the server child process.  But
	   in the future it is quite plausible that we'll want to make
	   this case work analogously to cvs_flusherr.

	   FIXME - DRP - I tried to implement this and triggered the following
	   error: "Protocol error: uncounted data discarded".  I don't need
	   this feature right now, so I'm not going to bother with it yet.
	 */
	buf_send_special_count (protocol, -1);
    }
    else
#endif
	fflush (stdout);
}



/* Output TEXT, tagging it according to TAG.  There are lots more
   details about what TAG means in cvsclient.texi but for the simple
   case (e.g. non-client/server), TAG is just "newline" to output a
   newline (in which case TEXT must be NULL), and any other tag to
   output normal text.

   Note that there is no way to output either \0 or \n as part of TEXT.  */

void
cvs_output_tagged (const char *tag, const char *text)
{
    if (text != NULL && strchr (text, '\n') != NULL)
	/* Uh oh.  The protocol has no way to cope with this.  For now
	   we dump core, although that really isn't such a nice
	   response given that this probably can be caused by newlines
	   in filenames and other causes other than bugs in CVS.  Note
	   that we don't want to turn this into "MT newline" because
	   this case is a newline within a tagged item, not a newline
	   as extraneous sugar for the user.  */
	assert (0);

    /* Start and end tags don't take any text, per cvsclient.texi.  */
    if (tag[0] == '+' || tag[0] == '-')
	assert (text == NULL);

#ifdef SERVER_SUPPORT
    if (server_active && supported_response ("MT"))
    {
	struct buffer *buf;

	if (error_use_protocol)
	    buf = buf_to_net;
	else
	    buf = protocol;

	buf_output0 (buf, "MT ");
	buf_output0 (buf, tag);
	if (text != NULL)
	{
	    buf_output (buf, " ", 1);
	    buf_output0 (buf, text);
	}
	buf_output (buf, "\n", 1);

	if (!error_use_protocol)
	    buf_send_counted (protocol);
    }
    else
#endif /* SERVER_SUPPORT */
    {
	/* No MT support or we are using a local repository. */
	if (strcmp (tag, "newline") == 0)
	    cvs_output ("\n", 1);
	else if (strcmp (tag, "date") == 0)
	{
#ifdef SERVER_SUPPORT
	    if (server_active)
		/* Output UTC when running as a server without MT support in
		 * the client since it is likely to be more meaningful than
	         * localtime.
		 */
		cvs_output (text, 0);
	    else
#endif /* SERVER_SUPPORT */
	    {
		char *date_in = xstrdup (text);
		char *date = format_date_alloc (date_in);
		cvs_output (date, 0);
		free (date);
		free (date_in);
	    }
	}
	else if (text != NULL)
	    cvs_output (text, 0);
    }
}



/*
 * void cvs_trace(int level, const char *fmt, ...)
 *
 * Print tracing information to stderr on request.  Levels are implemented
 * as with CVSNT.
 */
void
cvs_trace (int level, const char *fmt, ...)
{
    if (trace >= level)
    {
	va_list va;

	va_start (va, fmt);
#ifdef SERVER_SUPPORT
	fprintf (stderr,"%c -> ",server_active?(isProxyServer()?'P':'S'):' ');
#else /* ! SERVER_SUPPORT */
	fprintf (stderr,"  -> ");
#endif
	vfprintf (stderr, fmt, va);
	fprintf (stderr,"\n");
	va_end (va);
    }
}
