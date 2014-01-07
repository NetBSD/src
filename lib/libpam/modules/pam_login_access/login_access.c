/*	$NetBSD: login_access.c,v 1.8 2014/01/07 02:07:43 joerg Exp $	*/

/*
 * This module implements a simple but effective form of login access
 * control based on login names and on host (or domain) names, internet
 * addresses (or network numbers), or on terminal line names in case of
 * non-networked logins. Diagnostics are reported through syslog(3).
 *
 * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
 */

#if 0
#ifndef lint
static char sccsid[] = "%Z% %M% %I% %E% %U%";
#endif
#endif

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/lib/libpam/modules/pam_login_access/login_access.c,v 1.12 2004/03/05 08:10:18 markm Exp $");
#else
__RCSID("$NetBSD: login_access.c,v 1.8 2014/01/07 02:07:43 joerg Exp $");
#endif

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <stdarg.h>

#include "pam_login_access.h"

#define _PATH_LOGACCESS		"/etc/login.access"

 /* Delimiters for fields and for lists of users, ttys or hosts. */

static char fs[] = ":";			/* field separator */
static char sep[] = ", \t";		/* list-element separator */

 /* Constants to be used in assignments only, not in comparisons... */

#define YES             1
#define NO              0

static int	from_match(const char *, const char *);
static int	list_match(char *, const char *,
				int (*)(const char *, const char *));
static int	netgroup_match(const char *, const char *, const char *);
static int	string_match(const char *, const char *);
static int	user_match(const char *, const char *);

/* login_access - match username/group and host/tty with access control file */

__printflike(2, 3)
static void
logit(int level, const char *fmt, ...)
{
	va_list ap;
	struct syslog_data data = SYSLOG_DATA_INIT;

	openlog_r("pam_login_access", LOG_PID, LOG_AUTHPRIV, &data);
	va_start(ap, fmt);
	vsyslog_r(level, &data, fmt, ap);
	va_end(ap);
	closelog_r(&data);
}

int
login_access(const char *user, const char *from)
{
    FILE   *fp;
    char    line[BUFSIZ];
    char   *perm;			/* becomes permission field */
    char   *users;			/* becomes list of login names */
    char   *froms;			/* becomes list of terminals or hosts */
    int     match = NO;
    size_t  end;
    int     lineno = 0;			/* for diagnostics */

    /*
     * Process the table one line at a time and stop at the first match.
     * Blank lines and lines that begin with a '#' character are ignored.
     * Non-comment lines are broken at the ':' character. All fields are
     * mandatory. The first field should be a "+" or "-" character. A
     * non-existing table means no access control.
     */

    if ((fp = fopen(_PATH_LOGACCESS, "r")) != NULL) {
	while (!match && fgets(line, sizeof(line), fp)) {
	    lineno++;
	    if ((end = strlen(line)) == 0 || line[end - 1] != '\n') {
		logit(LOG_ERR, "%s: line %d: missing newline or line too long",
		       _PATH_LOGACCESS, lineno);
		continue;
	    }
	    if (line[0] == '#')
		continue;			/* comment line */
	    while (end > 0 && isspace((unsigned char)line[end - 1]))
		end--;
	    line[end] = 0;			/* strip trailing whitespace */
	    if (line[0] == 0)			/* skip blank lines */
		continue;
	    if (!(perm = strtok(line, fs))
		|| !(users = strtok((char *) 0, fs))
		|| !(froms = strtok((char *) 0, fs))
		|| strtok((char *) 0, fs)) {
		logit(LOG_ERR, "%s: line %d: bad field count", _PATH_LOGACCESS,
		       lineno);
		continue;
	    }
	    if (perm[0] != '+' && perm[0] != '-') {
		logit(LOG_ERR, "%s: line %d: bad first field", _PATH_LOGACCESS,
		       lineno);
		continue;
	    }
	    match = (list_match(froms, from, from_match)
		     && list_match(users, user, user_match));
	}
	(void) fclose(fp);
    } else if (errno != ENOENT) {
	logit(LOG_ERR, "cannot open %s: %s", _PATH_LOGACCESS, strerror(errno));
    }
    return (match == 0 || (line[0] == '+'));
}

/* list_match - match an item against a list of tokens with exceptions */

static int
list_match(char *list, const char *item,
    int (*match_fn)(const char *, const char *))
{
    char   *tok;
    int     match = NO;

    /*
     * Process tokens one at a time. We have exhausted all possible matches
     * when we reach an "EXCEPT" token or the end of the list. If we do find
     * a match, look for an "EXCEPT" list and recurse to determine whether
     * the match is affected by any exceptions.
     */

    for (tok = strtok(list, sep); tok != 0; tok = strtok((char *) 0, sep)) {
	if (strcasecmp(tok, "EXCEPT") == 0)	/* EXCEPT: give up */
	    break;
	if ((match = (*match_fn)(tok, item)) != 0)	/* YES */
	    break;
    }
    /* Process exceptions to matches. */

    if (match != NO) {
	while ((tok = strtok((char *) 0, sep)) && strcasecmp(tok, "EXCEPT"))
	     /* VOID */ ;
	if (tok == 0 || list_match((char *) 0, item, match_fn) == NO)
	    return (match);
    }
    return (NO);
}

/* netgroup_match - match group against machine or user */

static int
netgroup_match(const char *group __unused,
    const char *machine __unused, const char *user __unused)
{
    logit(LOG_ERR, "NIS netgroup support not configured");
    return 0;
}

/* user_match - match a username against one token */

static int
user_match(const char *tok, const char *string)
{
    struct group grres, *group;
    char grbuf[1024];
    int     i;

    /*
     * If a token has the magic value "ALL" the match always succeeds.
     * Otherwise, return YES if the token fully matches the username, or if
     * the token is a group that contains the username.
     */

    if (tok[0] == '@') {			/* netgroup */
	return (netgroup_match(tok + 1, (char *) 0, string));
    } else if (string_match(tok, string)) {	/* ALL or exact match */
	return (YES);
    } else if (getgrnam_r(tok, &grres, grbuf, sizeof(grbuf), &group) == 0 &&
	group != NULL) {/* try group membership */
	for (i = 0; group->gr_mem[i]; i++)
	    if (strcasecmp(string, group->gr_mem[i]) == 0)
		return (YES);
    }
    return (NO);
}

/* from_match - match a host or tty against a list of tokens */

static int
from_match(const char *tok, const char *string)
{
    size_t     tok_len;
    size_t     str_len;

    /*
     * If a token has the magic value "ALL" the match always succeeds. Return
     * YES if the token fully matches the string. If the token is a domain
     * name, return YES if it matches the last fields of the string. If the
     * token has the magic value "LOCAL", return YES if the string does not
     * contain a "." character. If the token is a network number, return YES
     * if it matches the head of the string.
     */

    if (tok[0] == '@') {			/* netgroup */
	return (netgroup_match(tok + 1, string, (char *) 0));
    } else if (string_match(tok, string)) {	/* ALL or exact match */
	return (YES);
    } else if (tok[0] == '.') {			/* domain: match last fields */
	if ((str_len = strlen(string)) > (tok_len = strlen(tok))
	    && strcasecmp(tok, string + str_len - tok_len) == 0)
	    return (YES);
    } else if (strcasecmp(tok, "LOCAL") == 0) {	/* local: no dots */
	if (strchr(string, '.') == 0)
	    return (YES);
    } else if (tok[(tok_len = strlen(tok)) - 1] == '.'	/* network */
	       && strncmp(tok, string, tok_len) == 0) {
	return (YES);
    }
    return (NO);
}

/* string_match - match a string against one token */

static int
string_match(const char *tok, const char *string)
{

    /*
     * If the token has the magic value "ALL" the match always succeeds.
     * Otherwise, return YES if the token fully matches the string.
     */

    if (strcasecmp(tok, "ALL") == 0) {		/* all: always matches */
	return (YES);
    } else if (strcasecmp(tok, string) == 0) {	/* try exact match */
	return (YES);
    }
    return (NO);
}
