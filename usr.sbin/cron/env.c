/*	$NetBSD: env.c,v 1.11 1999/03/22 22:18:45 aidan Exp $	*/

/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Paul Vixie          <paul@vix.com>          uunet!decwrl!vixie!paul
 */

#include <sys/cdefs.h>
#if !defined(lint) && !defined(LINT)
#if 0
static char rcsid[] = "Id: env.c,v 2.7 1994/01/26 02:25:50 vixie Exp";
#else
__RCSID("$NetBSD: env.c,v 1.11 1999/03/22 22:18:45 aidan Exp $");
#endif
#endif


#include "cron.h"
#include <string.h>

char **
env_init()
{
	char	**p = (char **) malloc(sizeof(char **));

	p[0] = NULL;
	return (p);
}


void
env_free(envp)
	char	**envp;
{
	char	**p;

	for (p = envp;  *p;  p++)
		free(*p);
	free(envp);
}


char **
env_copy(envp)
	char	**envp;
{
	int	count, i;
	char	**p;

	for (count = 0;  envp[count] != NULL;  count++)
		;
	p = (char **) malloc((count+1) * sizeof(char *));  /* 1 for the NULL */
	for (i = 0;  i < count;  i++)
		p[i] = strdup(envp[i]);
	p[count] = NULL;
	return (p);
}


char **
env_set(envp, envstr)
	char	**envp;
	char	*envstr;
{
	int	count, found;
	char	**p;

	/*
	 * count the number of elements, including the null pointer;
	 * also set 'found' to -1 or index of entry if already in here.
	 */
	found = -1;
	for (count = 0;  envp[count] != NULL;  count++) {
		if (!strcmp_until(envp[count], envstr, '='))
			found = count;
	}
	count++;	/* for the NULL */

	if (found != -1) {
		/*
		 * it exists already, so just free the existing setting,
		 * save our new one there, and return the existing array.
		 */
		free(envp[found]);
		envp[found] = strdup(envstr);
		return (envp);
	}

	/*
	 * it doesn't exist yet, so resize the array, move null pointer over
	 * one, save our string over the old null pointer, and return resized
	 * array.
	 */
	p = (char **) realloc((void *) envp,
			      (unsigned) ((count+1) * sizeof(char **)));
	p[count] = p[count-1];
	p[count-1] = strdup(envstr);
	return (p);
}


/* return	ERR = end of file
 *		FALSE = not an env setting (file was repositioned)
 *		TRUE = was an env setting
 */
int
load_env(envstr, f)
	char	*envstr;
	FILE	*f;
{
	long	filepos;
	int	fileline, len;
	char	*name, *name_end, *val, *equal;
	char	*s;

	filepos = ftell(f);
	fileline = LineNumber;
	skip_comments(f);
	if (EOF == get_string(envstr, MAX_ENVSTR, f, "\n"))
		return (ERR);

	Debug(DPARS, ("load_env, read <%s>\n", envstr))

	equal = strchr(envstr, '=');
	if (equal) {
		/*
		 * decide if this is an environment variable or not by
		 * checking for spaces in the middle of the variable name.
		 * (it could also be a crontab line of the form
		 * <min> <hour> <day> <month> <weekday> command flag=value)
		 */
		/* space before var name */
		for (name = envstr; name < equal && isspace(*name); name++)
			;

		/* var name */
		if (*name == '"' || *name == '\'') {
			s = strchr(name + 1, *name);
			name++;
			if (!s || s > equal) {
				Debug(DPARS, ("load_env, didn't get valid string"));
				fseek(f, filepos, 0);
				Set_LineNum(fileline);
				return (FALSE);
			}
			name_end = s++;
		} else {
			for (s = name ; s < equal && !isspace(*s); s++)
				;
			name_end = s;
			if (s < equal)
				s++;
		}

		/* space after var name */
		for ( ; s < equal && isspace(*s); s++)
			;
		/*
		 * "s" should equal "equal"..  otherwise, this is not an
		 * environment set command.
		 */
	}
	if (equal == NULL || name == name_end || s != equal) {
		Debug(DPARS, ("load_env, didn't get valid string"));
		fseek(f, filepos, 0);
		Set_LineNum(fileline);
		return (FALSE);
	}

	/*
	 * process value string
	 */
	val = equal + 1;
	while (*val && isspace(*val))
		val++;
	if (*val) {
		len = strdtb(val);
		if (len >= 2 && (val[0] == '\'' || val[0] == '"') &&
		    val[len-1] == val[0]) {
			val[len-1] = '\0';
			val++;
		}
	}

	*name_end = '\0';
	(void) snprintf(envstr, MAX_ENVSTR, "%s=%s", name, val);
	Debug(DPARS, ("load_env, <%s> <%s> -> <%s>\n", name, val, envstr))
	return (TRUE);
}


char *
env_get(name, envp)
	char	*name;
	char	**envp;
{
	int	len = strlen(name);
	char	*p, *q;

	while ((p = *envp++) != NULL) {
		if (!(q = strchr(p, '=')))
			continue;
		if ((q - p) == len && !strncmp(p, name, len))
			return (q+1);
	}
	return (NULL);
}
