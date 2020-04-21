/*	$NetBSD: user.c,v 1.2.48.1 2020/04/21 18:41:55 martin Exp $	*/

/* Copyright 1988,1990,1993,1994 by Paul Vixie
 * All rights reserved
 */

/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1997,2000 by Internet Software Consortium, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/cdefs.h>
#if !defined(lint) && !defined(LINT)
#if 0
static char rcsid[] = "Id: user.c,v 1.5 2004/01/23 18:56:43 vixie Exp";
+#else
+__RCSID("$NetBSD: user.c,v 1.2.48.1 2020/04/21 18:41:55 martin Exp $");
#endif
#endif

/* vix 26jan87 [log is in RCS file]
 */

#include "cron.h"

void
free_user(user *u) {
	entry *e, *ne;

	free(u->name);
	for (e = u->crontab;  e != NULL;  e = ne) {
		ne = e->next;
		job_remove(e, u);
		free_entry(e);
	}
	free(u);
}

user *
load_user(int crontab_fd, struct passwd	*pw, const char *name) {
	char envstr[MAX_ENVSTR];
	FILE *file;
	user *u;
	entry *e;
	int status, save_errno;
	char **envp, **tenvp;

	if (!(file = fdopen(crontab_fd, "r"))) {
		perror("fdopen on crontab_fd in load_user");
		return (NULL);
	}

	Debug(DPARS, ("load_user()\n"));

	/* file is open.  build user entry, then read the crontab file.
	 */
	if ((u = malloc(sizeof(*u))) == NULL)
		return (NULL);
	if ((u->name = strdup(name)) == NULL) {
		save_errno = errno;
		free(u);
		errno = save_errno;
		return (NULL);
	}
	u->crontab = NULL;

	/* init environment.  this will be copied/augmented for each entry.
	 */
	if ((envp = env_init()) == NULL) {
		save_errno = errno;
		free(u->name);
		free(u);
		errno = save_errno;
		return (NULL);
	}

	/* load the crontab
	 */
	while ((status = load_env(envstr, file)) >= OK) {
		switch (status) {
		case ERR:
			free_user(u);
			u = NULL;
			goto done;
		case FALSE:
			e = load_entry(file, NULL, pw, envp);
			if (e) {
				e->next = u->crontab;
				u->crontab = e;
			}
			break;
		case TRUE:
			if ((tenvp = env_set(envp, envstr)) == NULL) {
				save_errno = errno;
				free_user(u);
				u = NULL;
				errno = save_errno;
				goto done;
			}
			envp = tenvp;
			break;
		}
	}

 done:
	env_free(envp);
	(void)fclose(file);
	Debug(DPARS, ("...load_user() done\n"));
	return (u);
}
