/* Copyright 1988,1990,1993 by Paul Vixie
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

#if !defined(lint) && !defined(LINT)
static char rcsid[] = "$Id: user.c,v 1.1.1.1 1994/01/05 20:40:16 jtc Exp $";
#endif

/* vix 26jan87 [log is in RCS file]
 */


#include "cron.h"
#include "externs.h"


void
free_user(u)
	user	*u;
{
	entry	*e, *ne;
	char	**env;

	for (e = u->crontab;  e != NULL;  e = ne) {
		ne = e->next;
		free_entry(e);
	}
	for (env = u->envp;  *env;  env++)
		(void) free(*env);
	(void) free(u->envp);
	(void) free(u);
}


user *
load_user(crontab_fd, name, uid, gid, dir, syscron)
	int	crontab_fd;
	char	*name;
	int	uid;
	int	gid;
	char	*dir;
	int	syscron;		/* system crontab file? */
{
	char	envstr[MAX_ENVSTR];
	FILE	*file;
	user	*u;
	entry	*e;
	int	status;

	if (!(file = fdopen(crontab_fd, "r"))) {
		perror("fdopen on crontab_fd in load_user");
		return NULL;
	}

	Debug(DPARS, ("load_user()\n"))

	/* file is open.  build user entry, then read the crontab file.
	 */
	u = (user *) malloc(sizeof(user));
	u->uid     = uid;
	u->gid     = gid;
	u->envp    = env_init();
	u->crontab = NULL;

	/*
	 * do auto env settings that the user could reset in the cron tab
	 */
	sprintf(envstr, "SHELL=%s", _PATH_BSHELL);
	u->envp = env_set(u->envp, envstr);

	sprintf(envstr, "HOME=%s", dir);
	u->envp = env_set(u->envp, envstr);

	/* load the crontab
	 */
	while ((status = load_env(envstr, file)) >= OK) {
		if (status == TRUE) {
			u->envp = env_set(u->envp, envstr);
		} else {
			if (NULL != (e = load_entry(file, NULL, syscron))) {
				e->next = u->crontab;
				u->crontab = e;
			}
		}
	}

	/*
	 * do automatic env settings that should have precedence over any
	 * set in the cron tab.
	 */
	sprintf(envstr, "%s=%s", "LOGNAME", name);
	u->envp = env_set(u->envp, envstr);
#if defined(BSD)
	sprintf(envstr, "%s=%s", "USER", name);
	u->envp = env_set(u->envp, envstr);
#endif

	/*
	 * done. close file, return pointer to 'user' structure
	 */
	fclose(file);

	Debug(DPARS, ("...load_user() done\n"))

	return u;
}
