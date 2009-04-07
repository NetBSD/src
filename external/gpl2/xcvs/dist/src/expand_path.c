/* expand_path.c -- expand environmental variables in passed in string
 *
 * Copyright (C) 1995-2005 The Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * The main routine is expand_path(), it is the routine that handles
 * the '~' character in four forms: 
 *     ~name
 *     ~name/
 *     ~/
 *     ~
 * and handles environment variables contained within the pathname
 * which are defined by:
 *     ${var_name}   (var_name is the name of the environ variable)
 *     $var_name     (var_name ends w/ non-alphanumeric char other than '_')
 */

#include "cvs.h"
#include <sys/types.h>

/* User variables.  */

List *variable_list;

static void variable_delproc (Node *);

static void
variable_delproc (Node *node)
{
    free (node->data);
}

/* Currently used by -s option; we might want a way to set user
   variables in a file in the $CVSROOT/CVSROOT directory too.  */

void
variable_set (char *nameval)
{
    char *p;
    char *name;
    Node *node;

    p = nameval;
    while (isalnum ((unsigned char) *p) || *p == '_')
	++p;
    if (*p != '=')
	error (1, 0, "invalid character in user variable name in %s", nameval);
    if (p == nameval)
	error (1, 0, "empty user variable name in %s", nameval);
    name = xmalloc (p - nameval + 1);
    strncpy (name, nameval, p - nameval);
    name[p - nameval] = '\0';
    /* Make p point to the value.  */
    ++p;
    if (strchr (p, '\012'))
	error (1, 0, "linefeed in user variable value in %s", nameval);

    if (!variable_list)
	variable_list = getlist ();

    node = findnode (variable_list, name);
    if (!node)
    {
	node = getnode ();
	node->type = VARIABLE;
	node->delproc = variable_delproc;
	node->key = name;
	node->data = xstrdup (p);
	(void) addnode (variable_list, node);
    }
    else
    {
	/* Replace the old value.  For example, this means that -s
	   options on the command line override ones from .cvsrc.  */
	free (node->data);
	node->data = xstrdup (p);
	free (name);
    }
}



/* Expand variable NAME into its contents, per the rules above.
 *
 * CVSROOT is used to expanding $CVSROOT.
 *
 * RETURNS
 *   A pointer to the requested variable contents or NULL when the requested
 *   variable is not found.
 *
 * ERRORS
 *   None, though this function may generate warning messages when NAME is not
 *   found.
 */
static const char *
expand_variable (const char *name, const char *cvsroot,
		 const char *file, int line)
{
    if (!strcmp (name, CVSROOT_ENV))
	return cvsroot;
    else if (!strcmp (name, "RCSBIN"))
    {
	error (0, 0, "RCSBIN internal variable is no longer supported");
	return NULL;
    }
    else if (!strcmp (name, EDITOR1_ENV))
	return Editor;
    else if (!strcmp (name, EDITOR2_ENV))
	return Editor;
    else if (!strcmp (name, EDITOR3_ENV))
	return Editor;
    else if (!strcmp (name, "USER"))
	return getcaller ();
    else if (!strcmp (name, "SESSIONID")
	     || !strcmp (name, "COMMITID"))
	return global_session_id;
    else if (isalpha (name[0]))
    {
	/* These names are reserved for future versions of CVS,
	   so that is why it is an error.  */
	if (line)
	    error (0, 0, "%s:%d: no such internal variable $%s",
		   file, line, name);
	else
	    error (0, 0, "%s: no such internal variable $%s",
		   file, name);
	return NULL;
    }
    else if (name[0] == '=')
    {
	Node *node;
	/* Crazy syntax for a user variable.  But we want
	   *something* that lets the user name a user variable
	   anything he wants, without interference from
	   (existing or future) internal variables.  */
	node = findnode (variable_list, name + 1);
	if (!node)
	{
	    if (line)
		error (0, 0, "%s:%d: no such user variable ${%s}",
		       file, line, name);
	    else
		error (0, 0, "%s: no such user variable ${%s}",
		       file, name);
	    return NULL;
	}
	return node->data;
    }
    else
    {
	/* It is an unrecognized character.  We return an error to
	   reserve these for future versions of CVS; it is plausible
	   that various crazy syntaxes might be invented for inserting
	   information about revisions, branches, etc.  */
	if (line)
	    error (0, 0, "%s:%d: unrecognized variable syntax %s",
		   file, line, name);
	else
	    error (0, 0, "%s: unrecognized variable syntax %s",
		   file, name);
	return NULL;
    }
}



/* This routine will expand the pathname to account for ~ and $
 * characters as described above.  Returns a pointer to a newly
 * malloc'd string.  If an error occurs, an error message is printed
 * via error() and NULL is returned.  FILE and LINE are the filename
 * and linenumber to include in the error message.  FILE must point
 * to something; LINE can be zero to indicate the line number is not
 * known.
 *
 * When FORMATSAFE is set, percent signs (`%') in variable contents are doubled
 * to prevent later expansion by format_cmdline.
 *
 * CVSROOT is used to expanding $CVSROOT.
 */
char *
expand_path (const char *name, const char *cvsroot, bool formatsafe,
	     const char *file, int line)
{
    size_t s, d, p;
    const char *e;

    char *mybuf = NULL;
    size_t mybuf_size = 0;
    char *buf = NULL;
    size_t buf_size = 0;

    char inquotes = '\0';

    char *result;

    /* Sorry this routine is so ugly; it is a head-on collision
       between the `traditional' unix *d++ style and the need to
       dynamically allocate.  It would be much cleaner (and probably
       faster, not that this is a bottleneck for CVS) with more use of
       strcpy & friends, but I haven't taken the effort to rewrite it
       thusly.  */

    /* First copy from NAME to MYBUF, expanding $<foo> as we go.  */
    s = d = 0;
    expand_string (&mybuf, &mybuf_size, d + 1);
    while ((mybuf[d++] = name[s]) != '\0')
    {
	if (name[s] == '\\')
	{
	    /* The next character is a literal.  Leave the \ in the string
	     * since it will be needed again when the string is split into
	     * arguments.
	     */
	    /* if we have a \ as the last character of the string, just leave
	     * it there - this is where we would set the escape flag to tell
	     * our parent we want another line if we cared.
	     */
	    if (name[++s])
	    {
		expand_string (&mybuf, &mybuf_size, d + 1);
		mybuf[d++] = name[s++];
	    }
	}
	/* skip $ variable processing for text inside single quotes */
	else if (inquotes == '\'')
	{
	    if (name[s++] == '\'')
	    {
		inquotes = '\0';
	    }
	}
	else if (name[s] == '\'')
	{
	    s++;
	    inquotes = '\'';
	}
	else if (name[s] == '"')
	{
	    s++;
	    if (inquotes) inquotes = '\0';
	    else inquotes = '"';
	}
	else if (name[s++] == '$')
	{
	    int flag = (name[s] == '{');
	    p = d;

	    expand_string (&mybuf, &mybuf_size, d + 1);
	    for (; (mybuf[d++] = name[s]); s++)
	    {
		if (flag
		    ? name[s] =='}'
		    : !isalnum (name[s]) && name[s] != '_')
		    break;
		expand_string (&mybuf, &mybuf_size, d + 1);
	    }
	    mybuf[--d] = '\0';
	    e = expand_variable (&mybuf[p+flag], cvsroot, file, line);

	    if (e)
	    {
		expand_string (&mybuf, &mybuf_size, d + 1);
		for (d = p - 1; (mybuf[d++] = *e++); )
		{
		    expand_string (&mybuf, &mybuf_size, d + 1);
		    if (mybuf[d-1] == '"')
		    {
			/* escape the double quotes if we're between a matched
			 * pair of double quotes so that this sub will be
			 * passed inside as or as part of a single argument
			 * during the argument split later.
			 */
			if (inquotes)
			{
			    mybuf[d-1] = '\\';
			    expand_string (&mybuf, &mybuf_size, d + 1);
			    mybuf[d++] = '"';
			}
		    }
		    else if (formatsafe && mybuf[d-1] == '%')
		    {
			/* escape '%' to get past printf style format strings
			 * later (in make_cmdline).
			 */
			expand_string (&mybuf, &mybuf_size, d + 1);
			mybuf[d] = '%';
			d++;
		    }
		}
		--d;
		if (flag && name[s])
		    s++;
	    }
	    else
		/* expand_variable has already printed an error message.  */
		goto error_exit;
	}
	expand_string (&mybuf, &mybuf_size, d + 1);
    }
    expand_string (&mybuf, &mybuf_size, d + 1);
    mybuf[d] = '\0';

    /* Then copy from MYBUF to BUF, expanding ~.  */
    s = d = 0;
    /* If you don't want ~username ~/ to be expanded simply remove
     * This entire if statement including the else portion
     */
    if (mybuf[s] == '~')
    {
	p = d;
	while (mybuf[++s] != '/' && mybuf[s] != '\0')
	{
	    expand_string (&buf, &buf_size, p + 1);
	    buf[p++] = name[s];
	}
	expand_string (&buf, &buf_size, p + 1);
	buf[p] = '\0';

	if (p == d)
	    e = get_homedir ();
	else
	{
#ifdef GETPWNAM_MISSING
	    if (line)
		error (0, 0,
		       "%s:%d:tilde expansion not supported on this system",
		       file, line);
	    else
		error (0, 0, "%s:tilde expansion not supported on this system",
		       file);
	    goto error_exit;
#else
	    struct passwd *ps;
	    ps = getpwnam (buf + d);
	    if (ps == NULL)
	    {
		if (line)
		    error (0, 0, "%s:%d: no such user %s",
			   file, line, buf + d);
		else
		    error (0, 0, "%s: no such user %s", file, buf + d);
		goto error_exit;
	    }
	    e = ps->pw_dir;
#endif
	}
	if (!e)
	    error (1, 0, "cannot find home directory");

	p = strlen (e);
	expand_string (&buf, &buf_size, d + p);
	memcpy (buf + d, e, p);
	d += p;
    }
    /* Kill up to here */
    p = strlen (mybuf + s) + 1;
    expand_string (&buf, &buf_size, d + p);
    memcpy (buf + d, mybuf + s, p);

    /* OK, buf contains the value we want to return.  Clean up and return
       it.  */
    free (mybuf);
    /* Save a little memory with xstrdup; buf will tend to allocate
       more than it needs to.  */
    result = xstrdup (buf);
    free (buf);
    return result;

 error_exit:
    if (mybuf) free (mybuf);
    if (buf) free (buf);
    return NULL;
}
