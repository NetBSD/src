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
 * Various useful functions for the CVS support code.
 */

#include "cvs.h"

#include "canonicalize.h"
#include "canon-host.h"
#include "getline.h"
#include "vasprintf.h"
#include "vasnprintf.h"

/* Get wint_t.  */
#ifdef HAVE_WINT_T
# include <wchar.h>
#endif



extern char *getlogin (void);



/* *STRPTR is a pointer returned from malloc (or NULL), pointing to *N
   characters of space.  Reallocate it so that points to at least
   NEWSIZE bytes of space.  Gives a fatal error if out of memory;
   if it returns it was successful.  */
void
expand_string (char **strptr, size_t *n, size_t newsize)
{
    while (*n < newsize)
	*strptr = x2realloc (*strptr, n);
}



/* char *
 * Xreadlink (const char *link, size_t size)
 *
 * INPUTS
 *  link	The original path.
 *  size	A guess as to the size needed for the path. It need
 *              not be right.
 * RETURNS
 *  The resolution of the final symbolic link in the path.
 *
 * ERRORS
 *  This function exits with a fatal error if it fails to read the
 *  link for any reason.
 */
char *
Xreadlink (const char *link, size_t size)
{
    char *file = xreadlink (link, size);

    if (file == NULL)
	error (1, errno, "cannot readlink %s", link);

    return file;
}



/* *STR is a pointer to a malloc'd string or NULL.  *LENP is its allocated
 * length.  If *STR is NULL then *LENP must be 0 and visa-versa.
 * Add SRC to the end of *STR, reallocating *STR if necessary.  */
void
xrealloc_and_strcat (char **str, size_t *lenp, const char *src)
{
    bool newstr = !*lenp;
    expand_string (str, lenp, (newstr ? 0 : strlen (*str)) + strlen (src) + 1);
    if (newstr)
	strcpy (*str, src);
    else
	strcat (*str, src);
}



/* Remove trailing newlines from STRING, destructively.
 *
 * RETURNS
 *
 *   True if any newlines were removed, false otherwise.
 */
int
strip_trailing_newlines (char *str)
{
    size_t index, origlen;
    index = origlen = strlen (str);

    while (index > 0 && str[index-1] == '\n')
	str[--index] = '\0';

    return index != origlen;
}



/* Return the number of levels that PATH ascends above where it starts.
 * For example:
 *
 *   "../../foo" -> 2
 *   "foo/../../bar" -> 1
 */
int
pathname_levels (const char *p)
{
    int level;
    int max_level;

    if (p == NULL) return 0;

    max_level = 0;
    level = 0;
    do
    {
	/* Now look for pathname level-ups.  */
	if (p[0] == '.' && p[1] == '.' && (p[2] == '\0' || ISSLASH (p[2])))
	{
	    --level;
	    if (-level > max_level)
		max_level = -level;
	}
	else if (p[0] == '\0' || ISSLASH (p[0]) ||
		 (p[0] == '.' && (p[1] == '\0' || ISSLASH (p[1]))))
	    ;
	else
	    ++level;

	/* q = strchr (p, '/'); but sub ISSLASH() for '/': */
	while (*p != '\0' && !ISSLASH (*p)) p++;
	if (*p != '\0') p++;
    } while (*p != '\0');
    return max_level;
}



/* Free a vector, where (*ARGV)[0], (*ARGV)[1], ... (*ARGV)[*PARGC - 1]
   are malloc'd and so is *ARGV itself.  Such a vector is allocated by
   line2argv or expand_wild, for example.  */
void
free_names (int *pargc, char **argv)
{
    register int i;

    for (i = 0; i < *pargc; i++)
    {					/* only do through *pargc */
	free (argv[i]);
    }
    free (argv);
    *pargc = 0;				/* and set it to zero when done */
}



/* Convert LINE into arguments separated by SEPCHARS.  Set *ARGC
   to the number of arguments found, and (*ARGV)[0] to the first argument,
   (*ARGV)[1] to the second, etc.  *ARGV is malloc'd and so are each of
   (*ARGV)[0], (*ARGV)[1], ...  Use free_names() to return the memory
   allocated here back to the free pool.  */
void
line2argv (int *pargc, char ***argv, char *line, char *sepchars)
{
    char *cp;
    /* Could make a case for size_t or some other unsigned type, but
       we'll stick with int to avoid signed/unsigned warnings when
       comparing with *pargc.  */
    int argv_allocated;

    /* Small for testing.  */
    argv_allocated = 1;
    *argv = xnmalloc (argv_allocated, sizeof (**argv));

    *pargc = 0;
    for (cp = strtok (line, sepchars); cp; cp = strtok (NULL, sepchars))
    {
	if (*pargc == argv_allocated)
	{
	    argv_allocated *= 2;
	    *argv = xnrealloc (*argv, argv_allocated, sizeof (**argv));
	}
	(*argv)[*pargc] = xstrdup (cp);
	(*pargc)++;
    }
}



/*
 * Returns the number of dots ('.') found in an RCS revision number
 */
int
numdots (const char *s)
{
    int dots = 0;

    for (; *s; s++)
    {
	if (*s == '.')
	    dots++;
    }
    return (dots);
}



/* Compare revision numbers REV1 and REV2 by consecutive fields.
   Return negative, zero, or positive in the manner of strcmp.  The
   two revision numbers must have the same number of fields, or else
   compare_revnums will return an inaccurate result. */
int
compare_revnums (const char *rev1, const char *rev2)
{
    const char *sp, *tp;
    char *snext, *tnext;
    int result = 0;

    sp = rev1;
    tp = rev2;
    while (result == 0)
    {
	result = strtoul (sp, &snext, 10) - strtoul (tp, &tnext, 10);
	if (*snext == '\0' || *tnext == '\0')
	    break;
	sp = snext + 1;
	tp = tnext + 1;
    }

    return result;
}



/* Increment a revision number.  Working on the string is a bit awkward,
   but it avoid problems with integer overflow should the revision numbers
   get really big.  */
char *
increment_revnum (const char *rev)
{
    char *newrev, *p;
    size_t len = strlen (rev);

    newrev = xmalloc (len + 2);
    memcpy (newrev, rev, len + 1);
    for (p = newrev + len; p != newrev; )
    {
	--p;
	if (!isdigit(*p))
	{
	    ++p;
	    break;
	}
	if (*p != '9')
	{
	    ++*p;
	    return newrev;
	}
	*p = '0';
    }
    /* The number was all 9s, so change the first character to 1 and add
       a 0 to the end.  */
    *p = '1';
    p = newrev + len;
    *p++ = '0';
    *p = '\0';
    return newrev;
}



/* Return the username by which the caller should be identified in
   CVS, in contexts such as the author field of RCS files, various
   logs, etc.  */
char *
getcaller (void)
{
#ifndef SYSTEM_GETCALLER
    static char *cache;
    struct passwd *pw;
    uid_t uid;
#endif

    /* If there is a CVS username, return it.  */
#ifdef AUTH_SERVER_SUPPORT
    if (CVS_Username != NULL)
	return CVS_Username;
#endif

#ifdef SYSTEM_GETCALLER
    return SYSTEM_GETCALLER ();
#else
    /* Get the caller's login from his uid.  If the real uid is "root"
       try LOGNAME USER or getlogin(). If getlogin() and getpwuid()
       both fail, return the uid as a string.  */

    if (cache != NULL)
	return cache;

    uid = getuid ();
    if (uid == (uid_t) 0)
    {
	char *name;

	/* super-user; try getlogin() to distinguish */
	if (((name = getlogin ()) || (name = getenv("LOGNAME")) ||
	     (name = getenv("USER"))) && *name)
	{
	    cache = xstrdup (name);
	    return cache;
	}
    }
    if ((pw = (struct passwd *) getpwuid (uid)) == NULL)
    {
	cache = Xasprintf ("uid%lu", (unsigned long) uid);
	return cache;
    }
    cache = xstrdup (pw->pw_name);
    return cache;
#endif
}



#ifdef lint
# ifndef __GNUC__
/* ARGSUSED */
bool
get_date (struct timespec *result, char const *p, struct timespec const *now)
{
    result->tv_sec = 0;
    result->tv_nsec = 0;

    return false;
}
# endif
#endif



/* Given some revision, REV, return the first prior revision that exists in the
 * RCS file, RCS.
 *
 * ASSUMPTIONS
 *   REV exists.
 *
 * INPUTS
 *   RCS	The RCS node pointer.
 *   REV	An existing revision in the RCS file referred to by RCS.
 *
 * RETURNS
 *   The first prior revision that exists in the RCS file, or NULL if no prior
 *   revision exists.  The caller is responsible for disposing of this string.
 *
 * NOTES
 *   This function currently neglects the case where we are on the trunk with
 *   rev = X.1, where X != 1.  If rev = X.Y, where X != 1 and Y > 1, then this
 *   function should work fine, as revision X.1 must exist, due to RCS rules.
 */
char *
previous_rev (RCSNode *rcs, const char *rev)
{
    char *p;
    char *tmp = xstrdup (rev);
    long r1;
    char *retval;

    /* Our retval can have no more digits and dots than our input revision.  */
    retval = xmalloc (strlen (rev) + 1);
    p = strrchr (tmp, '.');
    *p = '\0';
    r1 = strtol (p+1, NULL, 10);
    do {
	if (--r1 == 0)
	{
		/* If r1 == 0, then we must be on a branch and our parent must
		 * exist, or we must be on the trunk with a REV like X.1.
		 * We are neglecting the X.1 with X != 1 case by assuming that
		 * there is no previous revision when we discover we were on
		 * the trunk.
		 */
		p = strrchr (tmp, '.');
		if (p == NULL)
		    /* We are on the trunk.  */
		    retval = NULL;
		else
		{
		    *p = '\0';
		    sprintf (retval, "%s", tmp);
		}
		break;
	}
	sprintf (retval, "%s.%ld", tmp, r1);
    } while (!RCS_exist_rev (rcs, retval));

    free (tmp);
    return retval;
}



/* Given two revisions, find their greatest common ancestor.  If the
   two input revisions exist, then rcs guarantees that the gca will
   exist.  */
char *
gca (const char *rev1, const char *rev2)
{
    int dots;
    char *gca, *g;
    const char *p1, *p2;
    int r1, r2;
    char *retval;

    if (rev1 == NULL || rev2 == NULL)
    {
	error (0, 0, "sanity failure in gca");
	abort();
    }

    /* The greatest common ancestor will have no more dots, and numbers
       of digits for each component no greater than the arguments.  Therefore
       this string will be big enough.  */
    g = gca = xmalloc (strlen (rev1) + strlen (rev2) + 100);

    /* walk the strings, reading the common parts. */
    p1 = rev1;
    p2 = rev2;
    do
    {
	r1 = strtol (p1, (char **) &p1, 10);
	r2 = strtol (p2, (char **) &p2, 10);
	
	/* use the lowest. */
	(void) sprintf (g, "%d.", r1 < r2 ? r1 : r2);
	g += strlen (g);
	if (*p1 == '.') ++p1;
	else break;
	if (*p2 == '.') ++p2;
	else break;
    } while (r1 == r2);

    /* erase that last dot. */
    *--g = '\0';

    /* numbers differ, or we ran out of strings.  we're done with the
       common parts.  */

    dots = numdots (gca);
    if (dots == 0)
    {
	/* revisions differ in trunk major number.  */

	if (r2 < r1) p1 = p2;
	if (*p1 == '\0')
	{
	    /* we only got one number.  this is strange.  */
	    error (0, 0, "bad revisions %s or %s", rev1, rev2);
	    abort();
	}
	else
	{
	    /* we have a minor number.  use it.  */
	    *g++ = '.';
	    while (*p1 != '.' && *p1 != '\0')
		*g++ = *p1++;
	    *g = '\0';
	}
    }
    else if ((dots & 1) == 0)
    {
	/* if we have an even number of dots, then we have a branch.
	   remove the last number in order to make it a revision.  */
	
	g = strrchr (gca, '.');
	*g = '\0';
    }

    retval = xstrdup (gca);
    free (gca);
    return retval;
}



/* Give fatal error if REV is numeric and ARGC,ARGV imply we are
   planning to operate on more than one file.  The current directory
   should be the working directory.  Note that callers assume that we
   will only be checking the first character of REV; it need not have
   '\0' at the end of the tag name and other niceties.  Right now this
   is only called from admin.c, but if people like the concept it probably
   should also be called from diff -r, update -r, get -r, and log -r.  */
void
check_numeric (const char *rev, int argc, char **argv)
{
    if (rev == NULL || !isdigit ((unsigned char) *rev))
	return;

    /* Note that the check for whether we are processing more than one
       file is (basically) syntactic; that is, we don't behave differently
       depending on whether a directory happens to contain only a single
       file or whether it contains more than one.  I strongly suspect this
       is the least confusing behavior.  */
    if (argc != 1
	|| (!wrap_name_has (argv[0], WRAP_TOCVS) && isdir (argv[0])))
    {
	error (0, 0, "while processing more than one file:");
	error (1, 0, "attempt to specify a numeric revision");
    }
}



/*
 *  Sanity checks and any required fix-up on message passed to RCS via '-m'.
 *  RCS 5.7 requires that a non-total-whitespace, non-null message be provided
 *  with '-m'.  Returns a newly allocated, non-empty buffer with whitespace
 *  stripped from end of lines and end of buffer.
 *
 *  TODO: We no longer use RCS to manage repository files, so maybe this
 *  nonsense about non-empty log fields can be dropped.
 */
char *
make_message_rcsvalid (const char *message)
{
    char *dst, *dp;
    const char *mp;

    if (message == NULL) message = "";

    /* Strip whitespace from end of lines and end of string. */
    dp = dst = (char *) xmalloc (strlen (message) + 1);
    for (mp = message; *mp != '\0'; ++mp)
    {
	if (*mp == '\n')
	{
	    /* At end-of-line; backtrack to last non-space. */
	    while (dp > dst && (dp[-1] == ' ' || dp[-1] == '\t'))
		--dp;
	}
	*dp++ = *mp;
    }

    /* Backtrack to last non-space at end of string, and truncate. */
    while (dp > dst && isspace ((unsigned char) dp[-1]))
	--dp;
    *dp = '\0';

    /* After all that, if there was no non-space in the string,
       substitute a non-empty message. */
    if (*dst == '\0')
    {
	free (dst);
	dst = xstrdup ("*** empty log message ***");
    }

    return dst;
}



/* Does the file FINFO contain conflict markers?  The whole concept
   of looking at the contents of the file to figure out whether there are
   unresolved conflicts is kind of bogus (people do want to manage files
   which contain those patterns not as conflict markers), but for now it
   is what we do.  */
int
file_has_markers (const struct file_info *finfo)
{
    FILE *fp;
    char *line = NULL;
    size_t line_allocated = 0;
    int result;

    result = 0;
    fp = CVS_FOPEN (finfo->file, "r");
    if (fp == NULL)
	error (1, errno, "cannot open %s", finfo->fullname);
    while (getline (&line, &line_allocated, fp) > 0)
    {
	if (strncmp (line, RCS_MERGE_PAT_1, sizeof RCS_MERGE_PAT_1 - 1) == 0 ||
	    strncmp (line, RCS_MERGE_PAT_2, sizeof RCS_MERGE_PAT_2 - 1) == 0 ||
	    strncmp (line, RCS_MERGE_PAT_3, sizeof RCS_MERGE_PAT_3 - 1) == 0)
	{
	    result = 1;
	    goto out;
	}
    }
    if (ferror (fp))
	error (0, errno, "cannot read %s", finfo->fullname);
out:
    if (fclose (fp) < 0)
	error (0, errno, "cannot close %s", finfo->fullname);
    if (line != NULL)
	free (line);
    return result;
}



/* Read the entire contents of the file NAME into *BUF.
   If NAME is NULL, read from stdin.  *BUF
   is a pointer returned from malloc (or NULL), pointing to *BUFSIZE
   bytes of space.  The actual size is returned in *LEN.  On error,
   give a fatal error.  The name of the file to use in error messages
   (typically will include a directory if we have changed directory)
   is FULLNAME.  MODE is "r" for text or "rb" for binary.  */
void
get_file (const char *name, const char *fullname, const char *mode, char **buf,
	  size_t *bufsize, size_t *len)
{
    struct stat s;
    size_t nread;
    char *tobuf;
    FILE *e;
    size_t filesize;

    if (name == NULL)
    {
	e = stdin;
	filesize = 100;	/* force allocation of minimum buffer */
    }
    else
    {
	/* Although it would be cleaner in some ways to just read
	   until end of file, reallocating the buffer, this function
	   does get called on files in the working directory which can
	   be of arbitrary size, so I think we better do all that
	   extra allocation.  */

	if (stat (name, &s) < 0)
	    error (1, errno, "can't stat %s", fullname);

	/* Convert from signed to unsigned.  */
	filesize = s.st_size;

	e = xfopen (name, mode);
    }

    if (*buf == NULL || *bufsize <= filesize)
    {
	*bufsize = filesize + 1;
	*buf = xrealloc (*buf, *bufsize);
    }

    tobuf = *buf;
    nread = 0;
    while (1)
    {
	size_t got;

	got = fread (tobuf, 1, *bufsize - (tobuf - *buf), e);
	if (ferror (e))
	    error (1, errno, "can't read %s", fullname);
	nread += got;
	tobuf += got;

	if (feof (e))
	    break;

	/* Allocate more space if needed.  */
	if (tobuf == *buf + *bufsize)
	{
	    int c;
	    long off;

	    c = getc (e);
	    if (c == EOF)
		break;
	    off = tobuf - *buf;
	    expand_string (buf, bufsize, *bufsize + 100);
	    tobuf = *buf + off;
	    *tobuf++ = c;
	    ++nread;
	}
    }

    if (e != stdin && fclose (e) < 0)
	error (0, errno, "cannot close %s", fullname);

    *len = nread;

    /* Force *BUF to be large enough to hold a null terminator. */
    if (nread == *bufsize)
	expand_string (buf, bufsize, *bufsize + 1);
    (*buf)[nread] = '\0';
}



/* Follow a chain of symbolic links to its destination.  FILENAME
   should be a handle to a malloc'd block of memory which contains the
   beginning of the chain.  This routine will replace the contents of
   FILENAME with the destination (a real file).  */
void
resolve_symlink (char **filename)
{
    ssize_t rsize;

    if (filename == NULL || *filename == NULL)
	return;

    while ((rsize = islink (*filename)) > 0)
    {
#ifdef HAVE_READLINK
	/* The clean thing to do is probably to have each filesubr.c
	   implement this (with an error if not supported by the
	   platform, in which case islink would presumably return 0).
	   But that would require editing each filesubr.c and so the
	   expedient hack seems to be looking at HAVE_READLINK.  */
	char *newname = Xreadlink (*filename, rsize);
	
	if (ISABSOLUTE (newname))
	{
	    free (*filename);
	    *filename = newname;
	}
	else
	{
	    const char *oldname = last_component (*filename);
	    int dirlen = oldname - *filename;
	    char *fullnewname = xmalloc (dirlen + strlen (newname) + 1);
	    strncpy (fullnewname, *filename, dirlen);
	    strcpy (fullnewname + dirlen, newname);
	    free (newname);
	    free (*filename);
	    *filename = fullnewname;
	}
#else
	error (1, 0, "internal error: islink doesn't like readlink");
#endif
    }
}



/*
 * Rename a file to an appropriate backup name based on BAKPREFIX.
 * If suffix non-null, then ".<suffix>" is appended to the new name.
 *
 * Returns the new name, which caller may free() if desired.
 */
char *
backup_file (const char *filename, const char *suffix)
{
    char *backup_name = Xasprintf ("%s%s%s%s", BAKPREFIX, filename,
				   suffix ? "." : "", suffix ? suffix : "");

    if (isfile (filename))
        copy_file (filename, backup_name);

    return backup_name;
}



/*
 * Copy a string into a buffer escaping any shell metacharacters.  The
 * buffer should be at least twice as long as the string.
 *
 * Returns a pointer to the terminating NUL byte in buffer.
 */
char *
shell_escape(char *buf, const char *str)
{
    static const char meta[] = "$`\\\"";
    const char *p;

    for (;;)
    {
	p = strpbrk(str, meta);
	if (!p) p = str + strlen(str);
	if (p > str)
	{
	    memcpy(buf, str, p - str);
	    buf += p - str;
	}
	if (!*p) break;
	*buf++ = '\\';
	*buf++ = *p++;
	str = p;
    }
    *buf = '\0';
    return buf;
}



/*
 * We can only travel forwards in time, not backwards.  :)
 */
void
sleep_past (time_t desttime)
{
    time_t t;
    long s;
    long us;

    while (time (&t) <= desttime)
    {
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tv;
	gettimeofday (&tv, NULL);
	if (tv.tv_sec > desttime)
	    break;
	s = desttime - tv.tv_sec;
	if (tv.tv_usec > 0)
	    us = 1000000 - tv.tv_usec;
	else
	{
	    s++;
	    us = 0;
	}
#else
	/* default to 20 ms increments */
	s = desttime - t;
	us = 20000;
#endif

	{
	    struct timespec ts;
	    ts.tv_sec = s;
	    ts.tv_nsec = us * 1000;
	    (void)nanosleep (&ts, NULL);
	}
    }
}



/* used to store callback data in a list indexed by the user format string
 */
typedef int (*CONVPROC_t) (Node *, void *);
struct cmdline_bindings
{
    char conversion;
    void *data;
    CONVPROC_t convproc;
    void *closure;
};
/* since we store the above in a list, we need to dispose of the data field.
 * we don't have to worry about convproc or closure since pointers are stuck
 * in there directly and format_cmdline's caller is responsible for disposing
 * of those if necessary.
 */
static void
cmdline_bindings_hash_node_delete (Node *p)
{
    struct cmdline_bindings *b = p->data;

    if (b->conversion != ',')
    {
	free (b->data);
    }
    free (b);
}



/*
 * assume s is a literal argument and put it between quotes,
 * escaping as appropriate for a shell command line
 *
 * the caller is responsible for disposing of the new string
 */
char *
cmdlinequote (char quotes, char *s)
{
    char *quoted = cmdlineescape (quotes, s);
    char *buf = Xasprintf ("%c%s%c", quotes, quoted, quotes);

    free (quoted);
    return buf;
}



/* read quotes as the type of quotes we are between (if any) and then make our
 * argument so it could make it past a cmdline parser (using sh as a model)
 * inside the quotes (if any).
 *
 * if you were planning on expanding any paths, it should be done before
 * calling this function, as it escapes shell metacharacters.
 *
 * the caller is responsible for disposing of the new string
 *
 * FIXME: See about removing/combining this functionality with shell_escape()
 * in subr.c.
 */
char *
cmdlineescape (char quotes, char *s)
{
    char *buf = NULL;
    size_t length = 0;
    char *d = NULL;
    size_t doff;
    char *lastspace;

    lastspace = s - 1;
    do
    {
	/* FIXME: Single quotes only require other single quotes to be escaped
	 * for Bourne Shell.
	 */
	if ( isspace( *s ) ) lastspace = s;
	if( quotes
	    ? ( *s == quotes
	        || ( quotes == '"'
	             && ( *s == '$' || *s == '`' || *s == '\\' ) ) )
	    : ( strchr( "\\$`'\"*?", *s )
	        || isspace( *s )
	        || ( lastspace == ( s - 1 )
	             && *s == '~' ) ) )
	{
	    doff = d - buf;
	    expand_string (&buf, &length, doff + 1);
	    d = buf + doff;
	    *d++ = '\\';
	}	
	doff = d - buf;
	expand_string (&buf, &length, doff + 1);
	d = buf + doff;
    } while ((*d++ = *s++) != '\0');
    return (buf);
}



/* expand format strings in a command line.  modeled roughly after printf
 *
 * this function's arg list must be NULL terminated
 *
 * assume a space delimited list of args is the desired final output,
 * but args can be quoted (" or ').
 *
 * the best usage examples are in tag.c & logmsg.c, but here goes:
 *
 * INPUTS
 *    int oldway	to support old format strings
 *    char *srepos	you guessed it
 *    char *format	the format string to parse
 *    ...		NULL terminated data list in the following format:
 *    			char *userformat, char *printfformat, <type> data
 *    			    where
 *    				char *userformat	a list of possible
 *    							format characters the
 *    							end user might pass us
 *    							in the format string
 *    							(e.g. those found in
 *    							taginfo or loginfo)
 *    							multiple characters in
 *    							this strings will be
 *    							aliases for each other
 *    				char *printfformat	the same list of args
 *    							printf uses to
 *    							determine what kind of
 *    							data the next arg will
 *    							be
 *    				<type> data		a piece of data to be
 *    							formatted into the user
 *    							string, <type>
 *    							determined by the
 *    							printfformat string.
 *    		or	
 *    			char *userformat, char *printfformat, List *data,
 *    				int (*convproc) (Node *, void *), void *closure
 *    			    where
 *    				char *userformat	same as above, except
 *    							multiple characters in
 *    							this string represent
 *    							different node
 *    							attributes which can be
 *    							retrieved from data by
 *    							convproc
 *    				char *printfformat	= ","
 *				List *data		the list to be walked
 *							with walklist &
 *							convproc to retrieve
 *							data for each of the
 *							possible format
 *							characters in
 *							userformat
 *				int (*convproc)()	see data
 *				void *closure		arg to be passed into
 *							walklist as closure
 *							data for convproc
 *
 * EXAMPLE
 *    (ignoring oldway variable and srepos since those are only around while we
 *    SUPPORT_OLD_INFO_FMT_STRINGS)
 *    format_cmdline ("/cvsroot/CVSROOT/mytaginfoproc %t %o %{sVv}",
 *                    "t", "s", "newtag",
 *                    "o", "s", "mov",
 *                    "xG", "ld", longintwhichwontbeusedthispass,
 *                    "sVv", ",", tlist, pretag_list_to_args_proc,
 *                    (void *) mydata,
 *                    (char *) NULL);
 *
 *    would generate the following command line, assuming two files in tlist,
 *    file1 & file2, each with old versions 1.1 and new version 1.1.2.3:
 *
 *    	  /cvsroot/CVSROOT/mytaginfoproc "newtag" "mov" "file1" "1.1" "1.1.2.3" "file2" "1.1" "1.1.2.3"
 *
 * RETURNS
 *    pointer to newly allocated string.  the caller is responsible for
 *    disposing of this string.
 */
char *
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
format_cmdline (bool oldway, const char *srepos, const char *format, ...)
#else /* SUPPORT_OLD_INFO_FMT_STRINGS */
format_cmdline (const char *format, ...)
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
{
    va_list args;	/* our input function args */
    char *buf;		/* where we store our output string */
    size_t length;	/* the allocated length of our output string in bytes.
			 * used as a temporary storage for the length of the
			 * next function argument during function
			 * initialization
			 */
    char *pfmt;		/* initially the list of fmt keys passed in,
			 * but used as a temporary key buffer later
			 */
    char *fmt;		/* buffer for format string which we are processing */
    size_t flen;	/* length of fmt buffer */
    char *d, *q, *r;    /* for walking strings */
    const char *s;
    size_t doff, qoff;
    char inquotes;

    List *pflist = getlist();	/* our list of input data indexed by format
				 * "strings"
				 */
    Node *p;
    struct cmdline_bindings *b;
    static int warned_of_deprecation = 0;
    char key[] = "?";		/* Used as temporary storage for a single
				 * character search string used to locate a
				 * hash key.
				 */
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    /* state varialbes in the while loop which parses the actual
     * format string in the final parsing pass*/
    int onearg;
    int subbedsomething;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */

#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    if (oldway && !warned_of_deprecation)
    {
	/* warn the user that we don't like his kind 'round these parts */
	warned_of_deprecation = 1;
	error (0, 0,
"warning:  Set to use deprecated info format strings.  Establish\n"
"compatibility with the new info file format strings (add a temporary '1' in\n"
"all info files after each '%%' which doesn't represent a literal percent)\n"
"and set UseNewInfoFmtStrings=yes in CVSROOT/config.  After that, convert\n"
"individual command lines and scripts to handle the new format at your\n"
"leisure.");
    }
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */

    va_start (args, format);

    /* read our possible format strings
     * expect a certain number of arguments by type and a NULL format
     * string to terminate the list.
     */
    while ((pfmt = va_arg (args, char *)) != NULL)
    {
	char *conversion = va_arg (args, char *);

	char conversion_error = 0;
	char char_conversion = 0;
	char decimal_conversion = 0;
	char integer_conversion = 0;
	char string_conversion = 0;

	/* allocate space to save our data */
	b = xmalloc(sizeof(struct cmdline_bindings));

	/* where did you think we were going to store all this data??? */
	b->convproc = NULL;
	b->closure = NULL;

	/* read a length from the conversion string */
	s = conversion;
	length = 0;
	while (!length && *s)
	{
	    switch (*s)
	    {
		case 'h':
		    integer_conversion = 1;
		    if (s[1] == 'h')
		    {
			length = sizeof (char);
			s += 2;
		    }
		    else
		    {
			char_conversion = 1;
			length = sizeof (short);
			s++;
		    }
		    break;
#ifdef HAVE_INTMAX_T
		case 'j':
		    integer_conversion = 1;
		    length = sizeof (intmax_t);
		    s++;
		    break;
#endif /* HAVE_INTMAX_T */
		case 'l':
		    integer_conversion = 1;
		    if (s[1] == 'l')
		    {
#ifdef HAVE_LONG_LONG
			length = sizeof (long long);
#endif
			s += 2;
		    }
		    else
		    {
			char_conversion = 2;
			string_conversion = 2;
			length = sizeof (long);
			s++;
		    }
		    break;
		case 't':
		    integer_conversion = 1;
		    length = sizeof (ptrdiff_t);
		    s++;
		    break;
		case 'z':
		    integer_conversion = 1;
		    length = sizeof (size_t);
		    s++;
		    break;
#ifdef HAVE_LONG_DOUBLE
		case 'L':
		    decimal_conversion = 1;
		    length = sizeof (long double);
		    s++;
		    break;
#endif
		default:
		    char_conversion = 1;
		    decimal_conversion = 1;
		    integer_conversion = 1;
		    string_conversion = 1;
		    /* take care of it when we find out what we're looking for */
		    length = -1;
		    break;
	    }
	}
	/* if we don't have a valid conversion left, that is an error */
	/* read an argument conversion */
	buf = xmalloc (strlen(conversion) + 2);
	*buf = '%';
	strcpy (buf+1, conversion);
	switch (*s)
	{
	    case 'c':
		/* chars (an integer conversion) */
		if (!char_conversion)
		{
		    conversion_error = 1;
		    break;
		}
		if (char_conversion == 2)
		{
#ifdef HAVE_WINT_T
		    length = sizeof (wint_t);
#else
		    conversion_error = 1;
		    break;
#endif
		}
		else
		    length = sizeof (char);
		/* fall through... */
	    case 'd':
	    case 'i':
	    case 'o':
	    case 'u':
	    case 'x':
	    case 'X':
		/* integer conversions */
		if (!integer_conversion)
		{
		    conversion_error = 1;
		    break;
		}
		if (length == -1)
		{
		    length = sizeof (int);
		}
		switch (length)
		{
		    case sizeof(char):
		    {
		    	char arg_char = (char) va_arg (args, int);
			b->data = Xasprintf (buf, arg_char);
			break;
		    }
#ifdef UNIQUE_INT_TYPE_WINT_T		/* implies HAVE_WINT_T */
		    case sizeof(wint_t):
		    {
		    	wint_t arg_wint_t = va_arg (args, wint_t);
			b->data = Xasprintf (buf, arg_wint_t);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_WINT_T */
#ifdef UNIQUE_INT_TYPE_SHORT
		    case sizeof(short):
		    {
		    	short arg_short = (short) va_arg (args, int);
			b->data = Xasprintf (buf, arg_short);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_SHORT */
#ifdef UNIQUE_INT_TYPE_INT
		    case sizeof(int):
		    {
		    	int arg_int = va_arg (args, int);
			b->data = Xasprintf(buf, arg_int);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_INT */
#ifdef UNIQUE_INT_TYPE_LONG
		    case sizeof(long):
		    {
		    	long arg_long = va_arg (args, long);
			b->data = Xasprintf (buf, arg_long);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_LONG */
#ifdef UNIQUE_INT_TYPE_LONG_LONG	/* implies HAVE_LONG_LONG */
		    case sizeof(long long):
		    {
		    	long long arg_long_long = va_arg (args, long long);
			b->data = Xasprintf (buf, arg_long_long);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_LONG_LONG */
#ifdef UNIQUE_INT_TYPE_INTMAX_T		/* implies HAVE_INTMAX_T */
		    case sizeof(intmax_t):
		    {
		    	intmax_t arg_intmax_t = va_arg (args, intmax_t);
			b->data = Xasprintf (buf, arg_intmax_t);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_INTMAX_T */
#ifdef UNIQUE_INT_TYPE_SIZE_T
		    case sizeof(size_t):
		    {
		    	size_t arg_size_t = va_arg (args, size_t);
			b->data = Xasprintf (buf, arg_size_t);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_SIZE_T */
#ifdef UNIQUE_INT_TYPE_PTRDIFF_T
		    case sizeof(ptrdiff_t):
		    {
		    	ptrdiff_t arg_ptrdiff_t = va_arg (args, ptrdiff_t);
			b->data = Xasprintf (buf, arg_ptrdiff_t);
			break;
		    }
#endif /* UNIQUE_INT_TYPE_PTRDIFF_T */
		    default:
	    		dellist(&pflist);
	    		free(b);
			error (1, 0,
"internal error:  unknown integer arg size (%d)",
                               length);
			break;
		}
		break;
	    case 'a':
	    case 'A':
	    case 'e':
	    case 'E':
	    case 'f':
	    case 'F':
	    case 'g':
	    case 'G':
		/* decimal conversions */
		if (!decimal_conversion)
		{
		    conversion_error = 1;
		    break;
		}
		if (length == -1)
		{
		    length = sizeof (double);
		}
		switch (length)
		{
		    case sizeof(double):
		    {
		    	double arg_double = va_arg (args, double);
			b->data = Xasprintf (buf, arg_double);
			break;
		    }
#ifdef UNIQUE_FLOAT_TYPE_LONG_DOUBLE	/* implies HAVE_LONG_DOUBLE */
		    case sizeof(long double):
		    {
		    	long double arg_long_double = va_arg (args, long double);
			b->data = Xasprintf (buf, arg_long_double);
			break;
		    }
#endif /* UNIQUE_FLOAT_TYPE_LONG_DOUBLE */
		    default:
	    		dellist(&pflist);
	    		free(b);
			error (1, 0,
"internal error:  unknown floating point arg size (%d)",
                               length);
			break;
		}
		break;
	    case 's':
		switch (string_conversion)
		{
		    case 1:
			b->data = xstrdup (va_arg (args, char *));
			break;
#ifdef HAVE_WCHAR_T
		    case 2:
		    {
		    	wchar_t *arg_wchar_t_string = va_arg (args, wchar_t *);
			b->data = Xasprintf (buf, arg_wchar_t_string);
			break;
		    }
#endif /* HAVE_WCHAR_T */
		    default:
			conversion_error = 1;
			break;
		}
		break;
	    case ',':
		if (length != -1)
		{
		    conversion_error = 1;
		    break;
		}
		b->data = va_arg (args, List *);
		b->convproc = va_arg (args, CONVPROC_t);
		b->closure = va_arg (args, void *);
		break;
	    default:
		conversion_error = 1;
		break;
	}
	free (buf);
	/* fail if we found an error or haven't found the end of the string */
	if (conversion_error || s[1])
	{
	    error (1, 0,
"internal error (format_cmdline): '%s' is not a valid conversion!!!",
                   conversion);
	}


	/* save our type  - we really only care wheter it's a list type (',')
	 * or not from now on, but what the hell...
	 */
	b->conversion = *s;

	/* separate the user format string into parts and stuff our data into
	 * the pflist (once for each possible string - diverse keys can have
	 * duplicate data).
	 */
	q = pfmt;
	while (*q)
	{
    	    struct cmdline_bindings *tb;
	    if (*q == '{')
	    {
		s = q + 1;
		while (*++q && *q != '}');
		r = q + 1;
	    }
	    else
	    {
		s = q++;
		r = q;
	    }
	    if (*r)
	    {
		/* copy the data since we'll need it again */
    	    	tb = xmalloc(sizeof(struct cmdline_bindings));
		if (b->conversion == ',')
		{
		    tb->data = b->data;
		}
		else
		{
		    tb->data = xstrdup(b->data);
		}
		tb->conversion = b->conversion;
		tb->convproc = b->convproc;
		tb->closure = b->closure;
	    }
	    else
	    {
		/* we're done after this, so we don't need to copy the data */
		tb = b;
	    }
	    p = getnode();
	    p->key = xmalloc((q - s) + 1);
	    strncpy (p->key, s, q - s);
	    p->key[q-s] = '\0';
	    p->data = tb;
	    p->delproc = cmdline_bindings_hash_node_delete;
	    addnode(pflist,p);
	}
    }

    /* we're done with va_list */
    va_end(args);

    /* All formatted strings include a format character that resolves to the
     * empty string by default, so put it in pflist.
     */
    /* allocate space to save our data */
    b = xmalloc(sizeof(struct cmdline_bindings));
    b->conversion = 's';
    b->convproc = NULL;
    b->closure = NULL;
    b->data = xstrdup( "" );
    p = getnode();
    p->key = xstrdup( "n" );
    p->data = b;
    p->delproc = cmdline_bindings_hash_node_delete;
    addnode( pflist,p );

    /* finally, read the user string and copy it into rargv as appropriate */
    /* user format strings look as follows:
     *
     * %% is a literal %
     * \X, where X is any character = \X, (this is the escape you'd expect, but
     *        we are leaving the \ for an expected final pass which splits our
     *        output string into separate arguments
     *
     * %X means sub var "X" into location
     * %{VWXYZ} means sub V,W,X,Y,Z into location as a single arg.  The shell
     *        || would be to quote the comma separated arguments.  Each list
     *        that V, W, X, Y, and Z represent attributes of will cause a new
     *        tuple to be inserted for each list item with a space between
     *        items.
     *        e.g."V W1,X1,Z1 W2,X2,Z2 W3,X3,Z3 Y1 Y2" where V is not a list
     *        variable, W,X,&Z are attributes of a list with 3 items and Y is an
     *        attribute of a second list with 2 items.
     * %,{VWXYZ} means to separate the args.  The previous example would produce
     *        V W1 X1 Z1 W2 X2 Z2 W3 X3 Z3 Y1 Y2, where each variable is now a
     *        separate, space delimited, arguments within a single argument.
     * a%{XY}, where 'a' is a literal, still produces a single arg (a"X Y", in
     *        shell)
     * a%1{XY}, where 'a' is a literal, splits the literal as it produces
     *        multiple args (a X Y).  The rule is that each sub will produce a
     *        separate arg.  Without a comma, attributes will still be grouped
     *        together & comma separated in what could be a single argument,
     *        but internal quotes, commas, and spaces are not excaped.
     *
     * clearing the variable oldway, passed into this function, causes the
     * behavior of '1' and "," in the format string to reverse.
     */

    /* for convenience, use fmt as a temporary key buffer.
     * for speed, attempt to realloc it as little as possible
     */
    fmt = NULL;
    flen = 0;
    
    /* buf = current argv entry being built
     * length = current length of buf
     * s = next char in source buffer to read
     * d = next char location to write (in buf)
     * inquotes = current quote char or NUL
     */
    s = format;
    d = buf = NULL;
    length = 0;
    doff = d - buf;
    expand_string (&buf, &length, doff + 1);
    d = buf + doff;

    inquotes = '\0';
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    subbedsomething = 0;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
    while ((*d++ = *s) != '\0')
    {
	int list = 0;
	switch (*s++)
	{
	    case '\\':
		/* the character after a \ goes unprocessed but leave the \ in
		 * the string so the function that splits this string into a
		 * command line later can deal with quotes properly
		 *
		 * ignore a NUL
		 */
		if (*s)
		{
    		    doff = d - buf;
		    expand_string (&buf, &length, doff + 1);
		    d = buf + doff;
		    *d++ = *s++;
		}
		break;
	    case '\'':
	    case '"':
		/* keep track of quotes so we can escape quote chars we sub in
		 * - the API is that a quoted format string will guarantee that
		 * it gets passed into the command as a single arg
		 */
		if (!inquotes) inquotes = s[-1];
		else if (s[-1] == inquotes) inquotes = '\0';
		break;
	    case '%':
		if (*s == '%')
		{
		    /* "%%" is a literal "%" */
		    s++;
		    break;
		}
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		if (oldway && subbedsomething)
		{
		    /* the old method was to sub only the first format string */
		    break;
		}
		/* initialize onearg each time we get a new format string */
		onearg = oldway ? 1 : 0;
		subbedsomething = 1;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		d--;	/* we're going to overwrite the '%' regardless
			 * of other factors... */
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		/* detect '1' && ',' in the fmt string. */
		if (*s == '1')
		{
		    onearg = 1;
		    s++;
		    if (!oldway)
		    {
			/* FIXME - add FILE && LINE */
			error (0, 0,
"Using deprecated info format strings.  Convert your scripts to use\n"
"the new argument format and remove '1's from your info file format strings.");
		    }
		}
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		    
		/* parse the format string and sub in... */
		if (*s == '{')
		{
		    list = 1;
		    s++;
		}
		/* q = fmt start
		 * r = fmt end + 1
		 */
		q = fmt;
		do
		{
		    qoff = q - fmt;
		    expand_string (&fmt, &flen, qoff + 1);
		    q = fmt + qoff;
		} while ((*q = *s++) && list && *q++ != '}');
		/* we will always copy one character, so, whether in list mode
		 * or not, if we just copied a '\0', then we hit the end of the
		 * string before we should have
		 */
		if (!s[-1])
		{
		    /* if we copied a NUL while processing a list, fail
		     * - we had an empty fmt string or didn't find a list
		     * terminator ('}')
		     */
		    /* FIXME - this wants a file name and line number in a bad
		     * way.
		     */
		    error(1, 0,
"unterminated format string encountered in command spec.\n"
"This error is likely to have been caused by an invalid line in a hook script\n"
"spec (see taginfo, loginfo, verifymsginfo, etc. in the Cederqvist).  Most\n"
"likely the offending line would end with a '%%' character or contain a string\n"
"beginning \"%%{\" and no closing '}' before the end of the line.");
		}
		if (list)
		{
		    q[-1] = '\0';
		}
		else
		{
		    /* We're not in a list, so we must have just copied a
		     * single character.  Terminate the string.
		     */
		    q++;
		    qoff = q - fmt;
		    expand_string (&fmt, &flen, qoff + 1);
		    q = fmt + qoff;
		    *q = '\0';
		}
		/* fmt is now a pointer to a list of fmt chars, though the list
		 * could be a single element one
		 */
		q = fmt;
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		/* always add quotes in the deprecated onearg case - for
		 * backwards compatibility
		 */
		if (onearg)
		{
		    doff = d - buf;
		    expand_string (&buf, &length, doff + 1);
		    d = buf + doff;
		    *d++ = '"';
		}
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		/*
		 * for each character in the fmt string,
		 *
		 * all output will be separate quoted arguments (with
		 * internal quotes escaped) if the argument is in quotes
		 * unless the oldway variable is set, in which case the fmt
		 * statment will correspond to a single argument with
		 * internal space or comma delimited arguments
		 *
		 * see the "user format strings" section above for more info
		 */
		key[0] = *q;
		if ((p = findnode (pflist, key)) != NULL)
		{
		    b = p->data;
		    if (b->conversion == ',')
		    {
			/* process the rest of the format string as a list */
			struct format_cmdline_walklist_closure c;
			c.format = q;
			c.buf = &buf;
			c.length = &length;
			c.d = &d;
			c.quotes = inquotes;
			c.closure = b->closure;
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
			c.onearg = onearg;
			c.firstpass = 1;
			c.srepos = srepos;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
			walklist(b->data, b->convproc, &c);
			d--;	/* back up one space.  we know that ^
				   always adds 1 extra */
			q += strlen(q);
		    }
		    else
		    {
			/* got a flat item */
			char *outstr;
			if (strlen(q) > 1)
			{
			    error (1, 0,
"Multiple non-list variables are not allowed in a single format string.");
			}
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
			if (onearg)
			{
			    outstr = b->data;
			}
			else /* !onearg */
			{
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
			    /* the *only* case possible without
			     * SUPPORT_OLD_INFO_FORMAT_STRINGS
			     * - !onearg */
			    if (!inquotes)
			    {
				doff = d - buf;
				expand_string (&buf, &length, doff + 1);
				d = buf + doff;
				*d++ = '"';
			    }
			    outstr = cmdlineescape (inquotes ? inquotes : '"', b->data);
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
			} /* onearg */
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
			doff = d - buf;
			expand_string (&buf, &length, doff + strlen(outstr));
			d = buf + doff;
			strncpy(d, outstr, strlen(outstr));
			d += strlen(outstr);
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
			if (!onearg)
			{
			    free(outstr);
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
			    if (!inquotes)
			    {
				doff = d - buf;
				expand_string (&buf, &length, doff + 1);
				d = buf + doff;
				*d++ = '"';
			    }
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
			}
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
			q++;
		    }
		}
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		else if (onearg)
		{
		    /* the old standard was to ignore unknown format
		     * characters (print the empty string), but also that
		     * any format character meant print srepos first
		     */
		    q++;
		    doff = d - buf;
		    expand_string (&buf, &length, doff + strlen(srepos));
		    d = buf + doff;
		    strncpy(d, srepos, strlen(srepos));
		    d += strlen(srepos);
		}
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		else /* no key */
		{
		    /* print an error message to the user
		     * FIXME - this should have a file and line number!!! */
		    error (1, 0,
"Unknown format character in info file ('%s').\n"
"Info files are the hook files, verifymsg, taginfo, commitinfo, etc.",
                           q);
		}
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
		/* always add quotes in the deprecated onearg case - for
		 * backwards compatibility
		 */
		if (onearg)
		{
		    doff = d - buf;
		    expand_string (&buf, &length, doff + 1);
		    d = buf + doff;
		    *d++ = '"';
		}
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
		break;
	}
	doff = d - buf;
	expand_string (&buf, &length, doff + 1);
	d = buf + doff;
    } /* while (*d++ = *s) */
    if (fmt) free (fmt);
    if (inquotes)
    {
	/* FIXME - we shouldn't need this - Parse_Info should be handling
	 * multiple lines...
	 */
	error (1, 0, "unterminated quote in format string: %s", format);
    }

    dellist (&pflist);
    return buf;
}



/* Like xstrdup (), but can handle a NULL argument.
 */
char *
Xstrdup (const char *string)
{
  if (string == NULL) return NULL;
  return xmemdup (string, strlen (string) + 1);
}



/* Like xasprintf(), but consider all errors fatal (may never return NULL).
 */
char *
Xasprintf (const char *format, ...)
{
    va_list args;
    char *result;

    va_start (args, format);
    if (vasprintf (&result, format, args) < 0)
	error (1, errno, "Failed to write to string.");
    va_end (args);

    return result;
}



/* Like xasnprintf(), but consider all errors fatal (may never return NULL).
 */
char *
Xasnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
{
    va_list args;
    char *result;

    va_start (args, format);
    result = vasnprintf (resultbuf, lengthp, format, args);
    if (result == NULL)
	error (1, errno, "Failed to write to string.");
    va_end (args);

    return result;
}



/* Print a warning and return false if P doesn't look like a string specifying
 * a boolean value.
 *
 * Sets *VAL to the parsed value when it is found to be valid.  *VAL will not
 * be altered when false is returned.
 *
 * INPUTS
 *   infopath	Where the error is reported to be from on error.  This could
 *		be, for example, the name of the file the boolean is being read
 *		from.
 *   option	An option name being parsed, reported in traces and any error
 *		message.
 *   p		The string to actually read the option from.
 *   val	Pointer to where to store the boolean read from P.
 *
 * OUTPUTS
 *   val	TRUE/FALSE stored, as read, when there are no errors.
 *
 * RETURNS
 *   true	If VAL was read.
 *   false	On error.
 */
bool
readBool (const char *infopath, const char *option, const char *p, bool *val)
{
    TRACE (TRACE_FLOW, "readBool (%s, %s, %s)", infopath, option, p);
    if (!strcasecmp (p, "no") || !strcasecmp (p, "false")
        || !strcasecmp (p, "off") || !strcmp (p, "0"))
    {
	TRACE (TRACE_DATA, "Read %d for %s", *val, option);
	*val = false;
	return true;
    }
    else if (!strcasecmp (p, "yes") || !strcasecmp (p, "true")
	     || !strcasecmp (p, "on") || !strcmp (p, "1"))
    {
	TRACE (TRACE_DATA, "Read %d for %s", *val, option);
	*val = true;
	return true;
    }

    error (0, 0, "%s: unrecognized value `%s' for `%s'",
	   infopath, p, option);
    return false;
}



/*
 * Open a file, exiting with a message on error.
 *
 * INPUTS
 *   name	The name of the file to open.
 *   mode	Mode to open file in, as POSIX fopen().
 *
 * NOTES
 *   If you want to handle errors, just call fopen (NAME, MODE).
 *
 * RETURNS
 *   The new FILE pointer.
 */
FILE *
xfopen (const char *name, const char *mode)
{
    FILE *fp;

    if (!(fp = fopen (name, mode)))
	error (1, errno, "cannot open %s", name);
    return fp;
}



/* char *
 * xcanonicalize_file_name (const char *path)
 *
 * Like canonicalize_file_name(), but exit on error.
 *
 * INPUTS
 *  path	The original path.
 *
 * RETURNS
 *  The path with any symbolic links, `.'s, or `..'s, expanded.
 *
 * ERRORS
 *  This function exits with a fatal error if it fails to read the link for
 *  any reason.
 */
char *
xcanonicalize_file_name (const char *path)
{
    char *hardpath = canonicalize_file_name (path);
    if (!hardpath)
	error (1, errno, "Failed to resolve path: `%s'", path);
    return hardpath;
}



/* Declared in main.c.  */
extern char *server_hostname;

/* Return true if OTHERHOST resolves to this host in the DNS.
 *
 * GLOBALS
 *   server_hostname	The name of this host, as determined by the call to
 *			xgethostname() in main().
 *
 * RETURNS
 *   true	If OTHERHOST equals or resolves to HOSTNAME.
 *   false	Otherwise.
 */
bool
isThisHost (const char *otherhost)
{
    char *fqdno;
    char *fqdns;
    bool retval;

    /* As an optimization, check the literal strings before looking up
     * OTHERHOST in the DNS.
     */
    if (!strcasecmp (server_hostname, otherhost))
	return true;

    fqdno = canon_host (otherhost);
    if (!fqdno)
	error (1, 0, "Name lookup failed for `%s': %s",
	       otherhost, ch_strerror ());
    fqdns = canon_host (server_hostname);
    if (!fqdns)
	error (1, 0, "Name lookup failed for `%s': %s",
	       server_hostname, ch_strerror ());

    retval = !strcasecmp (fqdns, fqdno);

    free (fqdno);
    free (fqdns);
    return retval;
}



/* Return true if two paths match, resolving symlinks.
 */
bool
isSamePath (const char *path1_in, const char *path2_in)
{
    char *p1, *p2;
    bool same;

    if (!strcmp (path1_in, path2_in))
	return true;

    /* Path didn't match, but try to resolve any links that may be
     * present.
     */
    if (!isdir (path1_in) || !isdir (path2_in))
	/* To be resolvable, paths must exist on this server.  */
	return false;

    p1 = xcanonicalize_file_name (path1_in);
    p2 = xcanonicalize_file_name (path2_in);
    if (strcmp (p1, p2))
	same = false;
    else
	same = true;

    free (p1);
    free (p2);
    return same;
}
