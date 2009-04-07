/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Poritons Copyright (c) 1992, Mark D. Baushke
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Name of Root
 * 
 * Determine the path to the CVSROOT and set "Root" accordingly.
 */

#include "cvs.h"
#include <assert.h>
#include "getline.h"

/* Printable names for things in the current_parsed_root->method enum variable.
   Watch out if the enum is changed in cvs.h! */

const char method_names[][16] = {
    "undefined", "local", "server (rsh)", "pserver",
    "kserver", "gserver", "ext", "fork"
};

#ifndef DEBUG

cvsroot_t *
Name_Root (const char *dir, const char *update_dir)
{
    FILE *fpin;
    cvsroot_t *ret;
    const char *xupdate_dir;
    char *root = NULL;
    size_t root_allocated = 0;
    char *tmp;
    char *cvsadm;
    char *cp;
    int len;

    TRACE (TRACE_FLOW, "Name_Root (%s, %s)",
	   dir ? dir : "(null)",
	   update_dir ? update_dir : "(null)");

    if (update_dir && *update_dir)
	xupdate_dir = update_dir;
    else
	xupdate_dir = ".";

    if (dir != NULL)
    {
	cvsadm = Xasprintf ("%s/%s", dir, CVSADM);
	tmp = Xasprintf ("%s/%s", dir, CVSADM_ROOT);
    }
    else
    {
	cvsadm = xstrdup (CVSADM);
	tmp = xstrdup (CVSADM_ROOT);
    }

    /*
     * Do not bother looking for a readable file if there is no cvsadm
     * directory present.
     *
     * It is possible that not all repositories will have a CVS/Root
     * file. This is ok, but the user will need to specify -d
     * /path/name or have the environment variable CVSROOT set in
     * order to continue.  */
    if ((!isdir (cvsadm)) || (!isreadable (tmp)))
    {
	ret = NULL;
	goto out;
    }

    /*
     * The assumption here is that the CVS Root is always contained in the
     * first line of the "Root" file.
     */
    fpin = xfopen (tmp, "r");

    if ((len = getline (&root, &root_allocated, fpin)) < 0)
    {
	int saved_errno = errno;
	/* FIXME: should be checking for end of file separately; errno
	   is not set in that case.  */
	error (0, 0, "in directory %s:", xupdate_dir);
	error (0, saved_errno, "cannot read %s", CVSADM_ROOT);
	error (0, 0, "please correct this problem");
	ret = NULL;
	goto out;
    }
    fclose (fpin);
    cp = root + len - 1;
    if (*cp == '\n')
	*cp = '\0';			/* strip the newline */

    /*
     * root now contains a candidate for CVSroot. It must be an
     * absolute pathname or specify a remote server.
     */

    ret = parse_cvsroot (root);
    if (ret == NULL)
    {
	error (0, 0, "in directory %s:", xupdate_dir);
	error (0, 0,
	       "ignoring %s because it does not contain a valid root.",
	       CVSADM_ROOT);
	goto out;
    }

    if (!ret->isremote && !isdir (ret->directory))
    {
	error (0, 0, "in directory %s:", xupdate_dir);
	error (0, 0,
	       "ignoring %s because it specifies a non-existent repository %s",
	       CVSADM_ROOT, root);
	ret = NULL;
	goto out;
    }


 out:
    free (cvsadm);
    free (tmp);
    if (root != NULL)
	free (root);
    return ret;
}



/*
 * Write the CVS/Root file so that the environment variable CVSROOT
 * and/or the -d option to cvs will be validated or not necessary for
 * future work.
 */
void
Create_Root (const char *dir, const char *rootdir)
{
    FILE *fout;
    char *tmp;

    if (noexec)
	return;

    /* record the current cvs root */

    if (rootdir != NULL)
    {
        if (dir != NULL)
	    tmp = Xasprintf ("%s/%s", dir, CVSADM_ROOT);
        else
	    tmp = xstrdup (CVSADM_ROOT);

        fout = xfopen (tmp, "w+");
        if (fprintf (fout, "%s\n", rootdir) < 0)
	    error (1, errno, "write to %s failed", tmp);
        if (fclose (fout) == EOF)
	    error (1, errno, "cannot close %s", tmp);
	free (tmp);
    }
}

#endif /* ! DEBUG */



/* Translate an absolute repository string for a primary server and return it.
 *
 * INPUTS
 *   root_in	The root to be translated.
 *
 * RETURNS
 *   A translated string this function owns, or a pointer to the original
 *   string passed in if no translation was necessary.
 *
 *   If the returned string is the translated one, it may be overwritten
 *   by the next call to this function.
 */
const char *
primary_root_translate (const char *root_in)
{
#ifdef PROXY_SUPPORT
    char *translated;
    static char *previous = NULL;
    static size_t len;

    /* This can happen, for instance, during `cvs init'.  */
    if (!config) return root_in;

    if (config->PrimaryServer
        && !strncmp (root_in, config->PrimaryServer->directory,
		     strlen (config->PrimaryServer->directory))
        && (ISSLASH (root_in[strlen (config->PrimaryServer->directory)])
            || root_in[strlen (config->PrimaryServer->directory)] == '\0')
       )
    {
	translated =
	    Xasnprintf (previous, &len,
		        "%s%s", current_parsed_root->directory,
	                root_in + strlen (config->PrimaryServer->directory));
	if (previous && previous != translated)
	    free (previous);
	return previous = translated;
    }
#endif

    /* There is no primary root configured or it didn't match.  */
    return root_in;
}



/* Translate a primary root in reverse for PATHNAMEs in responses.
 *
 * INPUTS
 *   root_in	The root to be translated.
 *
 * RETURNS
 *   A translated string this function owns, or a pointer to the original
 *   string passed in if no translation was necessary.
 *
 *   If the returned string is the translated one, it may be overwritten
 *   by the next call to this function.
 */
const char *
primary_root_inverse_translate (const char *root_in)
{
#ifdef PROXY_SUPPORT
    char *translated;
    static char *previous = NULL;
    static size_t len;

    /* This can happen, for instance, during `cvs init'.  */
    if (!config) return root_in;

    if (config->PrimaryServer
        && !strncmp (root_in, current_parsed_root->directory,
		     strlen (current_parsed_root->directory))
        && (ISSLASH (root_in[strlen (current_parsed_root->directory)])
            || root_in[strlen (current_parsed_root->directory)] == '\0')
       )
    {
	translated =
	    Xasnprintf (previous, &len,
		        "%s%s", config->PrimaryServer->directory,
	                root_in + strlen (current_parsed_root->directory));
	if (previous && previous != translated)
	    free (previous);
	return previous = translated;
    }
#endif

    /* There is no primary root configured or it didn't match.  */
    return root_in;
}



/* The root_allow_* stuff maintains a list of valid CVSROOT
   directories.  Then we can check against them when a remote user
   hands us a CVSROOT directory.  */
static List *root_allow;

static void
delconfig (Node *n)
{
    if (n->data) free_config (n->data);
}



void
root_allow_add (const char *arg, const char *configPath)
{
    Node *n;

    if (!root_allow) root_allow = getlist();
    n = getnode();
    n->key = xstrdup (arg);
    n->data = parse_config (arg, configPath);
    n->delproc = delconfig;
    addnode (root_allow, n);
}

void
root_allow_free (void)
{
    dellist (&root_allow);
}

bool
root_allow_ok (const char *arg)
{
    if (!root_allow)
    {
	/* Probably someone upgraded from CVS before 1.9.10 to 1.9.10
	   or later without reading the documentation about
	   --allow-root.  Printing an error here doesn't disclose any
	   particularly useful information to an attacker because a
	   CVS server configured in this way won't let *anyone* in.  */

	/* Note that we are called from a context where we can spit
	   back "error" rather than waiting for the next request which
	   expects responses.  */
	printf ("\
error 0 Server configuration missing --allow-root in inetd.conf\n");
	exit (EXIT_FAILURE);
    }

    if (findnode (root_allow, arg))
	return true;
    return false;
}



/* Get a config we stored in response to root_allow.
 *
 * RETURNS
 *   The config associated with ARG.
 */
struct config *
get_root_allow_config (const char *arg, const char *configPath)
{
    Node *n;

    TRACE (TRACE_FUNCTION, "get_root_allow_config (%s)", arg);

    if (root_allow)
	n = findnode (root_allow, arg);
    else
	n = NULL;

    if (n) return n->data;
    return parse_config (arg, configPath);
}



/* This global variable holds the global -d option.  It is NULL if -d
   was not used, which means that we must get the CVSroot information
   from the CVSROOT environment variable or from a CVS/Root file.  */
char *CVSroot_cmdline;



/* FIXME - Deglobalize this. */
cvsroot_t *current_parsed_root = NULL;
/* Used to save the original root being processed so that we can still find it
 * in lists and the like after a `Redirect' response.  Also set to mirror
 * current_parsed_root in server mode so that code which runs on both the
 * client and server but which wants to use original data on the client can
 * just always reference the original_parsed_root.
 */
const cvsroot_t *original_parsed_root;


/* allocate and initialize a cvsroot_t
 *
 * We must initialize the strings to NULL so we know later what we should
 * free
 *
 * Some of the other zeroes remain meaningful as, "never set, use default",
 * or the like
 */
/* Functions which allocate memory are not pure.  */
static cvsroot_t *new_cvsroot_t(void)
    __attribute__( (__malloc__) );
static cvsroot_t *
new_cvsroot_t (void)
{
    cvsroot_t *newroot;

    /* gotta store it somewhere */
    newroot = xmalloc(sizeof(cvsroot_t));

    newroot->original = NULL;
    newroot->directory = NULL;
    newroot->method = null_method;
    newroot->isremote = false;
#ifdef CLIENT_SUPPORT
    newroot->username = NULL;
    newroot->password = NULL;
    newroot->hostname = NULL;
    newroot->cvs_rsh = NULL;
    newroot->cvs_server = NULL;
    newroot->port = 0;
    newroot->proxy_hostname = NULL;
    newroot->proxy_port = 0;
    newroot->redirect = true;	/* Advertise Redirect support */
#endif /* CLIENT_SUPPORT */

    return newroot;
}



/* Dispose of a cvsroot_t and its component parts.
 *
 * NOTE
 *  It is dangerous for most code to call this function since parse_cvsroot
 *  maintains a cache of parsed roots.
 */
static void
free_cvsroot_t (cvsroot_t *root)
{
    assert (root);
    if (root->original != NULL)
	free (root->original);
    if (root->directory != NULL)
	free (root->directory);
#ifdef CLIENT_SUPPORT
    if (root->username != NULL)
	free (root->username);
    if (root->password != NULL)
    {
	/* I like to be paranoid */
	memset (root->password, 0, strlen (root->password));
	free (root->password);
    }
    if (root->hostname != NULL)
	free (root->hostname);
    if (root->cvs_rsh != NULL)
	free (root->cvs_rsh);
    if (root->cvs_server != NULL)
	free (root->cvs_server);
    if (root->proxy_hostname != NULL)
	free (root->proxy_hostname);
#endif /* CLIENT_SUPPORT */
    free (root);
}



/*
 * Parse a CVSROOT string to allocate and return a new cvsroot_t structure.
 * Valid specifications are:
 *
 *	:(gserver|kserver|pserver):[[user][:password]@]host[:[port]]/path
 *	[:(ext|server):][[user]@]host[:]/path
 *	[:local:[e:]]/path
 *	:fork:/path
 *
 * INPUTS
 *	root_in		C String containing the CVSROOT to be parsed.
 *
 * RETURNS
 *	A pointer to a newly allocated cvsroot_t structure upon success and
 *	NULL upon failure.  The caller should never dispose of this structure,
 *	as it is stored in a cache, but the caller may rely on it not to
 *	change.
 *
 * NOTES
 * 	This would have been a lot easier to write in Perl.
 *
 *	Would it make sense to reimplement the root and config file parsing
 *	gunk in Lex/Yacc?
 *
 * SEE ALSO
 * 	free_cvsroot_t()
 */
cvsroot_t *
parse_cvsroot (const char *root_in)
{
    cvsroot_t *newroot;			/* the new root to be returned */
    char *cvsroot_save;			/* what we allocated so we can dispose
					 * it when finished */
    char *cvsroot_copy, *p;		/* temporary pointers for parsing */
#if defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
    char *q;				/* temporary pointer for parsing */
    char *firstslash;			/* save where the path spec starts
					 * while we parse
					 * [[user][:password]@]host[:[port]]
					 */
    int check_hostname, no_port, no_password, no_proxy;
#endif /* defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */
    static List *cache = NULL;
    Node *node;

    assert (root_in != NULL);

    /* This message is TRACE_FLOW since this function is called repeatedly by
     * the recursion routines.
     */
    TRACE (TRACE_FLOW, "parse_cvsroot (%s)", root_in);

    if ((node = findnode (cache, root_in)))
	return node->data;

    assert (root_in);

    /* allocate some space */
    newroot = new_cvsroot_t();

    /* save the original string */
    newroot->original = xstrdup (root_in);

    /* and another copy we can munge while parsing */
    cvsroot_save = cvsroot_copy = xstrdup (root_in);

    if (*cvsroot_copy == ':')
    {
	char *method = ++cvsroot_copy;

	/* Access method specified, as in
	 * "cvs -d :(gserver|kserver|pserver):[[user][:password]@]host[:[port]]/path",
	 * "cvs -d [:(ext|server):][[user]@]host[:]/path",
	 * "cvs -d :local:e:\path",
	 * "cvs -d :fork:/path".
	 * We need to get past that part of CVSroot before parsing the
	 * rest of it.
	 */

	if (! (p = strchr (method, ':')))
	{
	    error (0, 0, "No closing `:' on method in CVSROOT.");
	    goto error_exit;
	}
	*p = '\0';
	cvsroot_copy = ++p;

#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
	/* Look for method options, for instance, proxy, proxyport.
	 * Calling strtok again is saved until after parsing the method.
	 */
	method = strtok (method, ";");
	if (!method)
	    /* Could just exit now, but this keeps the error message in sync.
	     */
	    method = "";
#endif /* defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */

	/* Now we have an access method -- see if it's valid. */

	if (!strcasecmp (method, "local"))
	    newroot->method = local_method;
	else if (!strcasecmp (method, "pserver"))
	    newroot->method = pserver_method;
	else if (!strcasecmp (method, "kserver"))
	    newroot->method = kserver_method;
	else if (!strcasecmp (method, "gserver"))
	    newroot->method = gserver_method;
	else if (!strcasecmp (method, "server"))
	    newroot->method = server_method;
	else if (!strcasecmp (method, "ext"))
	    newroot->method = ext_method;
	else if (!strcasecmp (method, "fork"))
	    newroot->method = fork_method;
	else
	{
	    error (0, 0, "Unknown method (`%s') in CVSROOT.", method);
	    goto error_exit;
	}

#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
	/* Parse the method options, for instance, proxy, proxyport */
	while ((p = strtok (NULL, ";")))
	{
	    char *q = strchr (p, '=');
	    if (q == NULL)
	    {
	        error (0, 0, "Option (`%s') has no argument in CVSROOT.",
                       p);
	        goto error_exit;
	    }

	    *q++ = '\0';
	    TRACE (TRACE_DATA, "CVSROOT option=`%s' value=`%s'", p, q);
	    if (!strcasecmp (p, "proxy"))
	    {
		newroot->proxy_hostname = xstrdup (q);
	    }
	    else if (!strcasecmp (p, "proxyport"))
	    {
		char *r = q;
		if (*r == '-') r++;
		while (*r)
		{
		    if (!isdigit(*r++))
		    {
			error (0, 0,
"CVSROOT may only specify a positive, non-zero, integer proxy port (not `%s').",
			       q);
			goto error_exit;
		    }
		}
		if ((newroot->proxy_port = atoi (q)) <= 0)
		    error (0, 0,
"CVSROOT may only specify a positive, non-zero, integer proxy port (not `%s').",
			   q);
	    }
	    else if (!strcasecmp (p, "CVS_RSH"))
	    {
		/* override CVS_RSH environment variable */
		if (newroot->method == ext_method)
		    newroot->cvs_rsh = xstrdup (q);
	    }
	    else if (!strcasecmp (p, "CVS_SERVER"))
	    {
		/* override CVS_SERVER environment variable */
		if (newroot->method == ext_method
		    || newroot->method == fork_method)
		    newroot->cvs_server = xstrdup (q);
	    }
	    else if (!strcasecmp (p, "Redirect"))
		readBool ("CVSROOT", "Redirect", q, &newroot->redirect);
	    else
	    {
	        error (0, 0, "Unknown option (`%s') in CVSROOT.", p);
	        goto error_exit;
	    }
	}
#endif /* defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */
    }
    else
    {
	/* If the method isn't specified, assume EXT_METHOD if the string looks
	   like a relative path and LOCAL_METHOD otherwise.  */

	newroot->method = ((*cvsroot_copy != '/' && strchr (cvsroot_copy, '/'))
			  ? ext_method
			  : local_method);
    }

    /*
     * There are a few sanity checks we can do now, only knowing the
     * method of this root.
     */

    newroot->isremote = (newroot->method != local_method);

#if defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
    if (readonlyfs && newroot->isremote)
	error (1, 0,
"Read-only repository feature unavailable with remote roots (cvsroot = %s)",
	       cvsroot_copy);

    if ((newroot->method != local_method)
	&& (newroot->method != fork_method)
       )
    {
	/* split the string into [[user][:password]@]host[:[port]] & /path
	 *
	 * this will allow some characters such as '@' & ':' to remain unquoted
	 * in the path portion of the spec
	 */
	if ((p = strchr (cvsroot_copy, '/')) == NULL)
	{
	    error (0, 0, "CVSROOT requires a path spec:");
	    error (0, 0,
":(gserver|kserver|pserver):[[user][:password]@]host[:[port]]/path");
	    error (0, 0, "[:(ext|server):][[user]@]host[:]/path");
	    goto error_exit;
	}
	firstslash = p;		/* == NULL if '/' not in string */
	*p = '\0';

	/* Check to see if there is a username[:password] in the string. */
	if ((p = strchr (cvsroot_copy, '@')) != NULL)
	{
	    *p = '\0';
	    /* check for a password */
	    if ((q = strchr (cvsroot_copy, ':')) != NULL)
	    {
		*q = '\0';
		newroot->password = xstrdup (++q);
		/* Don't check for *newroot->password == '\0' since
		 * a user could conceivably wish to specify a blank password
		 *
		 * (newroot->password == NULL means to use the
		 * password from .cvspass)
		 */
	    }

	    /* copy the username */
	    if (*cvsroot_copy != '\0')
		/* a blank username is impossible, so leave it NULL in that
		 * case so we know to use the default username
		 */
		newroot->username = xstrdup (cvsroot_copy);

	    cvsroot_copy = ++p;
	}

	/* now deal with host[:[port]] */

	/* the port */
	if ((p = strchr (cvsroot_copy, ':')) != NULL)
	{
	    *p++ = '\0';
	    if (strlen(p))
	    {
		q = p;
		if (*q == '-') q++;
		while (*q)
		{
		    if (!isdigit(*q++))
		    {
			error (0, 0,
"CVSROOT may only specify a positive, non-zero, integer port (not `%s').",
				p);
			error (0, 0,
                               "Perhaps you entered a relative pathname?");
			goto error_exit;
		    }
		}
		if ((newroot->port = atoi (p)) <= 0)
		{
		    error (0, 0,
"CVSROOT may only specify a positive, non-zero, integer port (not `%s').",
			    p);
		    error (0, 0, "Perhaps you entered a relative pathname?");
		    goto error_exit;
		}
	    }
	}

	/* copy host */
	if (*cvsroot_copy != '\0')
	    /* blank hostnames are invalid, but for now leave the field NULL
	     * and catch the error during the sanity checks later
	     */
	    newroot->hostname = xstrdup (cvsroot_copy);

	/* restore the '/' */
	cvsroot_copy = firstslash;
	*cvsroot_copy = '/';
    }
#endif /* defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */

    /*
     * Parse the path for all methods.
     */
    /* Here & local_cvsroot() should be the only places this needs to be
     * called on a CVSROOT now.  cvsroot->original is saved for error messages
     * and, otherwise, we want no trailing slashes.
     */
    Sanitize_Repository_Name (cvsroot_copy);
    newroot->directory = xstrdup (cvsroot_copy);

    /*
     * Do various sanity checks.
     */

#if defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
    if (newroot->username && ! newroot->hostname)
    {
	error (0, 0, "Missing hostname in CVSROOT.");
	goto error_exit;
    }

    /* We won't have attempted to parse these without CLIENT_SUPPORT or
     * SERVER_SUPPORT.
     */
    check_hostname = 0;
    no_password = 1;
    no_proxy = 1;
    no_port = 0;
#endif /* defined (CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */
    switch (newroot->method)
    {
    case local_method:
#if defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
	if (newroot->username || newroot->hostname)
	{
	    error (0, 0, "Can't specify hostname and username in CVSROOT");
	    error (0, 0, "when using local access method.");
	    goto error_exit;
	}
#endif /* defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */
	/* cvs.texinfo has always told people that CVSROOT must be an
	   absolute pathname.  Furthermore, attempts to use a relative
	   pathname produced various errors (I couldn't get it to work),
	   so there would seem to be little risk in making this a fatal
	   error.  */
	if (!ISABSOLUTE (newroot->directory))
	{
	    error (0, 0, "CVSROOT must be an absolute pathname (not `%s')",
		   newroot->directory);
	    error (0, 0, "when using local access method.");
	    goto error_exit;
	}
#if defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
	/* We don't need to check for these in :local: mode, really, since
	 * we shouldn't be able to hit the code above which parses them, but
	 * I'm leaving them here in lieu of assertions.
	 */
	no_port = 1;
	/* no_password already set */
#endif /* defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */
	break;
#if defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
    case fork_method:
	/* We want :fork: to behave the same as other remote access
           methods.  Therefore, don't check to see that the repository
           name is absolute -- let the server do it.  */
	if (newroot->username || newroot->hostname)
	{
	    error (0, 0, "Can't specify hostname and username in CVSROOT");
	    error (0, 0, "when using fork access method.");
	    goto error_exit;
	}
	newroot->hostname = xstrdup("server");  /* for error messages */
	if (!ISABSOLUTE (newroot->directory))
	{
	    error (0, 0, "CVSROOT must be an absolute pathname (not `%s')",
		   newroot->directory);
	    error (0, 0, "when using fork access method.");
	    goto error_exit;
	}
	no_port = 1;
	/* no_password already set */
	break;
    case kserver_method:
	check_hostname = 1;
	/* no_password already set */
	break;
    case gserver_method:
	check_hostname = 1;
	no_proxy = 0;
	/* no_password already set */
	break;
    case server_method:
    case ext_method:
	no_port = 1;
	/* no_password already set */
	check_hostname = 1;
	break;
    case pserver_method:
	no_password = 0;
	no_proxy = 0;
	check_hostname = 1;
	break;
#endif /* defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */
    default:
	error (1, 0, "Invalid method found in parse_cvsroot");
    }

#if defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT)
    if (no_password && newroot->password)
    {
	error (0, 0, "CVSROOT password specification is only valid for");
	error (0, 0, "pserver connection method.");
	goto error_exit;
    }
    if (no_proxy && (newroot->proxy_hostname || newroot->proxy_port))
    {
	error (0, 0,
"CVSROOT proxy specification is only valid for gserver and");
	error (0, 0, "pserver connection methods.");
	goto error_exit;
    }

    if (!newroot->proxy_hostname && newroot->proxy_port)
    {
	error (0, 0, "Proxy port specified in CVSROOT without proxy host.");
	goto error_exit;
    }

    if (check_hostname && !newroot->hostname)
    {
	error (0, 0, "Didn't specify hostname in CVSROOT.");
	goto error_exit;
    }

    if (no_port && newroot->port)
    {
        error (0, 0,
"CVSROOT port specification is only valid for gserver, kserver,");
        error (0, 0, "and pserver connection methods.");
        goto error_exit;
    }
#endif /* defined(CLIENT_SUPPORT) || defined (SERVER_SUPPORT) */

    if (*newroot->directory == '\0')
    {
	error (0, 0, "Missing directory in CVSROOT.");
	goto error_exit;
    }
    
    /* Hooray!  We finally parsed it! */
    free (cvsroot_save);

    if (!cache) cache = getlist();
    node = getnode();
    node->key = xstrdup (newroot->original);
    node->data = newroot;
    addnode (cache, node);
    return newroot;

error_exit:
    free (cvsroot_save);
    free_cvsroot_t (newroot);
    return NULL;
}



#ifdef AUTH_CLIENT_SUPPORT
/* Use root->username, root->hostname, root->port, and root->directory
 * to create a normalized CVSROOT fit for the .cvspass file
 *
 * username defaults to the result of getcaller()
 * port defaults to the result of get_cvs_port_number()
 *
 * FIXME - we could cache the canonicalized version of a root inside the
 * cvsroot_t, but we'd have to un'const the input here and stop expecting the
 * caller to be responsible for our return value
 *
 * ASSUMPTIONS
 *   ROOT->method == pserver_method
 */
char *
normalize_cvsroot (const cvsroot_t *root)
{
    char *cvsroot_canonical;
    char *p, *hostname;

    assert (root && root->hostname && root->directory);

    /* use a lower case hostname since we know hostnames are case insensitive */
    /* Some logic says we should be tacking our domain name on too if it isn't
     * there already, but for now this works.  Reverse->Forward lookups are
     * almost certainly too much since that would make CVS immune to some of
     * the DNS trickery that makes life easier for sysadmins when they want to
     * move a repository or the like
     */
    p = hostname = xstrdup (root->hostname);
    while (*p)
    {
	*p = tolower (*p);
	p++;
    }

    cvsroot_canonical = Xasprintf (":pserver:%s@%s:%d%s",
                                   root->username ? root->username
                                                  : getcaller(),
                                   hostname, get_cvs_port_number (root),
                                   root->directory);

    free (hostname);
    return cvsroot_canonical;
}
#endif /* AUTH_CLIENT_SUPPORT */



#ifdef PROXY_SUPPORT
/* A walklist() function to walk the root_allow list looking for a PrimaryServer
 * configuration with a directory matching the requested directory.
 *
 * If found, replace it.
 */
static bool get_local_root_dir_done;
static int
get_local_root_dir (Node *p, void *root_in)
{
    struct config *c = p->data;
    char **r = root_in;

    if (get_local_root_dir_done)
	return 0;

    if (c->PrimaryServer && !strcmp (*r, c->PrimaryServer->directory))
    {
	free (*r);
	*r = xstrdup (p->key);
	get_local_root_dir_done = true;
    }
    return 0;
}
#endif /* PROXY_SUPPORT */



/* allocate and return a cvsroot_t structure set up as if we're using the local
 * repository DIR.  */
cvsroot_t *
local_cvsroot (const char *dir)
{
    cvsroot_t *newroot = new_cvsroot_t();

    newroot->original = xstrdup(dir);
    newroot->method = local_method;
    newroot->directory = xstrdup(dir);
    /* Here and parse_cvsroot() should be the only places this needs to be
     * called on a CVSROOT now.  cvsroot->original is saved for error messages
     * and, otherwise, we want no trailing slashes.
     */
    Sanitize_Repository_Name (newroot->directory);

#ifdef PROXY_SUPPORT
    /* Translate the directory to a local one in the case that we are
     * configured as a secondary.  If root_allow has not been initialized,
     * nothing happens.
     */
    get_local_root_dir_done = false;
    walklist (root_allow, get_local_root_dir, &newroot->directory);
#endif /* PROXY_SUPPORT */

    return newroot;
}



#ifdef DEBUG
/* This is for testing the parsing function.  Use

     gcc -I. -I.. -I../lib -DDEBUG root.c -o root

   to compile.  */

#include <stdio.h>

char *program_name = "testing";
char *cvs_cmd_name = "parse_cvsroot";		/* XXX is this used??? */

void
main (int argc, char *argv[])
{
    program_name = argv[0];

    if (argc != 2)
    {
	fprintf (stderr, "Usage: %s <CVSROOT>\n", program_name);
	exit (2);
    }
  
    if ((current_parsed_root = parse_cvsroot (argv[1])) == NULL)
    {
	fprintf (stderr, "%s: Parsing failed.\n", program_name);
	exit (1);
    }
    printf ("CVSroot: %s\n", argv[1]);
    printf ("current_parsed_root->method: %s\n",
	    method_names[current_parsed_root->method]);
    printf ("current_parsed_root->username: %s\n",
	    current_parsed_root->username
	      ? current_parsed_root->username : "NULL");
    printf ("current_parsed_root->hostname: %s\n",
	    current_parsed_root->hostname
	      ? current_parsed_root->hostname : "NULL");
    printf ("current_parsed_root->directory: %s\n",
	    current_parsed_root->directory);

   exit (0);
   /* NOTREACHED */
}
#endif
