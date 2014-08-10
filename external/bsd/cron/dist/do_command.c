/*	$NetBSD: do_command.c,v 1.3.20.1 2014/08/10 07:06:51 tls Exp $	*/

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
static char rcsid[] = "Id: do_command.c,v 1.9 2004/01/23 18:56:42 vixie Exp";
#else
__RCSID("$NetBSD: do_command.c,v 1.3.20.1 2014/08/10 07:06:51 tls Exp $");
#endif
#endif

#include "cron.h"
#include <unistd.h>

static void		child_process(entry *);
static int		safe_p(const char *, const char *);

void
do_command(entry *e, user *u) {
	Debug(DPROC, ("[%ld] do_command(%s, (%s,%ld,%ld))\n",
		      (long)getpid(), e->cmd, u->name,
		      (long)e->pwd->pw_uid, (long)e->pwd->pw_gid));

	/* fork to become asynchronous -- parent process is done immediately,
	 * and continues to run the normal cron code, which means return to
	 * tick().  the child and grandchild don't leave this function, alive.
	 *
	 * vfork() is unsuitable, since we have much to do, and the parent
	 * needs to be able to run off and fork other processes.
	 */
	switch (fork()) {
	case -1:
		log_it("CRON", getpid(), "error", "can't fork");
		break;
	case 0:
		/* child process */
		acquire_daemonlock(1);
		child_process(e);
		Debug(DPROC, ("[%ld] child process done, exiting\n",
			      (long)getpid()));
		_exit(OK_EXIT);
		break;
	default:
		/* parent process */
		break;
	}
	Debug(DPROC, ("[%ld] main process returning to work\n",(long)getpid()));
}

static void
child_process(entry *e) {
	int stdin_pipe[2], stdout_pipe[2];
	char * volatile input_data;
	char *homedir, *usernm, * volatile mailto;
	int children = 0;

	Debug(DPROC, ("[%ld] child_process('%s')\n", (long)getpid(), e->cmd));

	setproctitle("running job");

	/* discover some useful and important environment settings
	 */
	usernm = e->pwd->pw_name;
	mailto = env_get("MAILTO", e->envp);

	/* our parent is watching for our death by catching SIGCHLD.  we
	 * do not care to watch for our children's deaths this way -- we
	 * use wait() explicitly.  so we have to reset the signal (which
	 * was inherited from the parent).
	 */
	(void) signal(SIGCHLD, SIG_DFL);

	/* create some pipes to talk to our future child
	 */
	if (pipe(stdin_pipe) == -1) 	/* child's stdin */
		log_it("CRON", getpid(), "error", "create child stdin pipe");
	if (pipe(stdout_pipe) == -1)	/* child's stdout */
		log_it("CRON", getpid(), "error", "create child stdout pipe");
	
	/* since we are a forked process, we can diddle the command string
	 * we were passed -- nobody else is going to use it again, right?
	 *
	 * if a % is present in the command, previous characters are the
	 * command, and subsequent characters are the additional input to
	 * the command.  An escaped % will have the escape character stripped
	 * from it.  Subsequent %'s will be transformed into newlines,
	 * but that happens later.
	 */
	/*local*/{
		int escaped = FALSE;
		int ch;
		char *p;

		/* translation:
		 *	\% -> %
		 *	%  -> end of command, following is command input.
		 *	\x -> \x	for all x != %
		 */
		input_data = p = e->cmd;
		while ((ch = *input_data++) != '\0') {
 			if (escaped) {
				if (ch != '%')
					*p++ = '\\';
			} else {
				if (ch == '%') {
					break;
				}
			}

			if (!(escaped = (ch == '\\'))) {
				*p++ = ch;
			}
		}
		if (ch == '\0') {
			/* move pointer back, so that code below
			 * won't think we encountered % sequence */
			input_data--;
		}
		if (escaped)
			*p++ = '\\';

		*p = '\0';
	}

	/* fork again, this time so we can exec the user's command.
	 */
	switch (vfork()) {
	case -1:
		log_it("CRON", getpid(), "error", "can't vfork");
		exit(ERROR_EXIT);
		/*NOTREACHED*/
	case 0:
		Debug(DPROC, ("[%ld] grandchild process vfork()'ed\n",
			      (long)getpid()));

		/* write a log message.  we've waited this long to do it
		 * because it was not until now that we knew the PID that
		 * the actual user command shell was going to get and the
		 * PID is part of the log message.
		 */
		if ((e->flags & DONT_LOG) == 0) {
			char *x = mkprints(e->cmd, strlen(e->cmd));

			log_it(usernm, getpid(), "CMD START", x);
			free(x);
		}

		/* that's the last thing we'll log.  close the log files.
		 */
		log_close();

		/* get new pgrp, void tty, etc.
		 */
		if (setsid() == -1)
			syslog(LOG_ERR, "setsid() failure: %m");

		/* close the pipe ends that we won't use.  this doesn't affect
		 * the parent, who has to read and write them; it keeps the
		 * kernel from recording us as a potential client TWICE --
		 * which would keep it from sending SIGPIPE in otherwise
		 * appropriate circumstances.
		 */
		(void)close(stdin_pipe[WRITE_PIPE]);
		(void)close(stdout_pipe[READ_PIPE]);

		/* grandchild process.  make std{in,out} be the ends of
		 * pipes opened by our daddy; make stderr go to stdout.
		 */
		if (stdin_pipe[READ_PIPE] != STDIN) {
			(void)dup2(stdin_pipe[READ_PIPE], STDIN);
			(void)close(stdin_pipe[READ_PIPE]);
		}
		if (stdout_pipe[WRITE_PIPE] != STDOUT) {
			(void)dup2(stdout_pipe[WRITE_PIPE], STDOUT);
			(void)close(stdout_pipe[WRITE_PIPE]);
		}
		(void)dup2(STDOUT, STDERR);

		/* set our directory, uid and gid.  Set gid first, since once
		 * we set uid, we've lost root privledges.
		 */
#ifdef LOGIN_CAP
		{
#ifdef BSD_AUTH
			auth_session_t *as;
#endif
			login_cap_t *lc;
			char *p;

			if ((lc = login_getclass(e->pwd->pw_class)) == NULL) {
				warnx("unable to get login class for `%s'",
				    e->pwd->pw_name);
				_exit(ERROR_EXIT);
			}
			if (setusercontext(lc, e->pwd, e->pwd->pw_uid, LOGIN_SETALL) < 0) {
				warnx("setusercontext failed for `%s'",
				    e->pwd->pw_name);
				_exit(ERROR_EXIT);
			}
#ifdef BSD_AUTH
			as = auth_open();
			if (as == NULL || auth_setpwd(as, e->pwd) != 0) {
				warn("can't malloc");
				_exit(ERROR_EXIT);
			}
			if (auth_approval(as, lc, usernm, "cron") <= 0) {
				warnx("approval failed for `%s'",
				    e->pwd->pw_name);
				_exit(ERROR_EXIT);
			}
			auth_close(as);
#endif /* BSD_AUTH */
			login_close(lc);

			/* If no PATH specified in crontab file but
			 * we just added one via login.conf, add it to
			 * the crontab environment.
			 */
			if (env_get("PATH", e->envp) == NULL) {
				if ((p = getenv("PATH")) != NULL)
					e->envp = env_set(e->envp, p);
			}
		}
#else
		if (setgid(e->pwd->pw_gid) != 0) {
			syslog(LOG_ERR, "setgid(%d) failed for %s: %m",
			    e->pwd->pw_gid, e->pwd->pw_name);
			_exit(ERROR_EXIT);
		}
		if (initgroups(usernm, e->pwd->pw_gid) != 0) {
			syslog(LOG_ERR, "initgroups(%s, %d) failed for %s: %m",
			    usernm, e->pwd->pw_gid, e->pwd->pw_name);
			_exit(ERROR_EXIT);
		}
#if (defined(BSD)) && (BSD >= 199103)
		if (setlogin(usernm) < 0) {
			syslog(LOG_ERR, "setlogin(%s) failure for %s: %m",
			    usernm, e->pwd->pw_name);
			_exit(ERROR_EXIT);
		}
#endif /* BSD */
		if (setuid(e->pwd->pw_uid) != 0) {
			syslog(LOG_ERR, "setuid(%d) failed for %s: %m",
			    e->pwd->pw_uid, e->pwd->pw_name);
			_exit(ERROR_EXIT);
		}
		/* we aren't root after this... */
#endif /* LOGIN_CAP */
		homedir = env_get("HOME", e->envp);
		if (chdir(homedir) != 0) {
			syslog(LOG_ERR, "chdir(%s) $HOME failed for %s: %m",
			    homedir, e->pwd->pw_name);
			_exit(ERROR_EXIT);
		}

#ifdef USE_SIGCHLD
		/* our grandparent is watching for our death by catching
		 * SIGCHLD.  the parent is ignoring SIGCHLD's; we want
		 * to restore default behaviour.
		 */
		(void) signal(SIGCHLD, SIG_DFL);
#endif
		(void) signal(SIGHUP, SIG_DFL);

		/*
		 * Exec the command.
		 */
		{
			char	*shell = env_get("SHELL", e->envp);

# if DEBUGGING
			if (DebugFlags & DTEST) {
				(void)fprintf(stderr,
				"debug DTEST is on, not exec'ing command.\n");
				(void)fprintf(stderr,
				"\tcmd='%s' shell='%s'\n", e->cmd, shell);
				_exit(OK_EXIT);
			}
# endif /*DEBUGGING*/
			(void)execle(shell, shell, "-c", e->cmd, NULL, e->envp);
			warn("execl: couldn't exec `%s'", shell);
			_exit(ERROR_EXIT);
		}
		break;
	default:
		/* parent process */
		break;
	}

	children++;

	/* middle process, child of original cron, parent of process running
	 * the user's command.
	 */

	Debug(DPROC, ("[%ld] child continues, closing pipes\n",(long)getpid()));

	/* close the ends of the pipe that will only be referenced in the
	 * grandchild process...
	 */
	(void)close(stdin_pipe[READ_PIPE]);
	(void)close(stdout_pipe[WRITE_PIPE]);

	/*
	 * write, to the pipe connected to child's stdin, any input specified
	 * after a % in the crontab entry.  while we copy, convert any
	 * additional %'s to newlines.  when done, if some characters were
	 * written and the last one wasn't a newline, write a newline.
	 *
	 * Note that if the input data won't fit into one pipe buffer (2K
	 * or 4K on most BSD systems), and the child doesn't read its stdin,
	 * we would block here.  thus we must fork again.
	 */

	if (*input_data && fork() == 0) {
		FILE *out = fdopen(stdin_pipe[WRITE_PIPE], "w");
		int need_newline = FALSE;
		int escaped = FALSE;
		int ch;

		Debug(DPROC, ("[%ld] child2 sending data to grandchild\n",
			      (long)getpid()));

		/* close the pipe we don't use, since we inherited it and
		 * are part of its reference count now.
		 */
		(void)close(stdout_pipe[READ_PIPE]);

		/* translation:
		 *	\% -> %
		 *	%  -> \n
		 *	\x -> \x	for all x != %
		 */
		while ((ch = *input_data++) != '\0') {
			if (escaped) {
				if (ch != '%')
					(void)putc('\\', out);
			} else {
				if (ch == '%')
					ch = '\n';
			}

			if (!(escaped = (ch == '\\'))) {
				(void)putc(ch, out);
				need_newline = (ch != '\n');
			}
		}
		if (escaped)
			(void)putc('\\', out);
		if (need_newline)
			(void)putc('\n', out);

		/* close the pipe, causing an EOF condition.  fclose causes
		 * stdin_pipe[WRITE_PIPE] to be closed, too.
		 */
		(void)fclose(out);

		Debug(DPROC, ("[%ld] child2 done sending to grandchild\n",
			      (long)getpid()));
		exit(0);
	}

	/* close the pipe to the grandkiddie's stdin, since its wicked uncle
	 * ernie back there has it open and will close it when he's done.
	 */
	(void)close(stdin_pipe[WRITE_PIPE]);

	children++;

	/*
	 * read output from the grandchild.  it's stderr has been redirected to
	 * it's stdout, which has been redirected to our pipe.  if there is any
	 * output, we'll be mailing it to the user whose crontab this is...
	 * when the grandchild exits, we'll get EOF.
	 */

	Debug(DPROC, ("[%ld] child reading output from grandchild\n",
		      (long)getpid()));

	/*local*/{
		FILE	*in = fdopen(stdout_pipe[READ_PIPE], "r");
		int	ch = getc(in);

		if (ch != EOF) {
			FILE	*mail = NULL;
			int	bytes = 1;
			int	status = 0;

			Debug(DPROC|DEXT,
			      ("[%ld] got data (%x:%c) from grandchild\n",
			       (long)getpid(), ch, ch));

			/* get name of recipient.  this is MAILTO if set to a
			 * valid local username; USER otherwise.
			 */
			if (mailto) {
				/* MAILTO was present in the environment
				 */
				if (!*mailto) {
					/* ... but it's empty. set to NULL
					 */
					mailto = NULL;
				}
			} else {
				/* MAILTO not present, set to USER.
				 */
				mailto = usernm;
			}
		
			/* if we are supposed to be mailing, MAILTO will
			 * be non-NULL.  only in this case should we set
			 * up the mail command and subjects and stuff...
			 */

			if (mailto && safe_p(usernm, mailto)) {
				char	**env;
				char	mailcmd[MAX_COMMAND];
				char	hostname[MAXHOSTNAMELEN + 1];

				(void)gethostname(hostname, MAXHOSTNAMELEN);
				if (strlens(MAILFMT, MAILARG, NULL) + 1
				    >= sizeof mailcmd) {
					warnx("mailcmd too long");
					(void) _exit(ERROR_EXIT);
				}
				(void)snprintf(mailcmd, sizeof(mailcmd), 
				    MAILFMT, MAILARG);
				if (!(mail = cron_popen(mailcmd, "w", e->pwd))) {
					warn("cannot run `%s'", mailcmd);
					(void) _exit(ERROR_EXIT);
				}
				(void)fprintf(mail,
				    "From: root (Cron Daemon)\n");
				(void)fprintf(mail, "To: %s\n", mailto);
				(void)fprintf(mail,
				    "Subject: Cron <%s@%s> %s\n",
				    usernm, first_word(hostname, "."), e->cmd);
				(void)fprintf(mail,
				    "Auto-Submitted: auto-generated\n");
#ifdef MAIL_DATE
				(void)fprintf(mail, "Date: %s\n",
					arpadate(&StartTime));
#endif /*MAIL_DATE*/
				for (env = e->envp;  *env;  env++)
					(void)fprintf(mail,
					    "X-Cron-Env: <%s>\n", *env);
				(void)fprintf(mail, "\n");

				/* this was the first char from the pipe
				 */
				(void)putc(ch, mail);
			}

			/* we have to read the input pipe no matter whether
			 * we mail or not, but obviously we only write to
			 * mail pipe if we ARE mailing.
			 */

			while (EOF != (ch = getc(in))) {
				bytes++;
				if (mailto)
					(void)putc(ch, mail);
			}

			/* only close pipe if we opened it -- i.e., we're
			 * mailing...
			 */

			if (mailto) {
				Debug(DPROC, ("[%ld] closing pipe to mail\n",
					      (long)getpid()));
				/* Note: the pclose will probably see
				 * the termination of the grandchild
				 * in addition to the mail process, since
				 * it (the grandchild) is likely to exit
				 * after closing its stdout.
				 */
				status = cron_pclose(mail);
			}

			/* if there was output and we could not mail it,
			 * log the facts so the poor user can figure out
			 * what's going on.
			 */
			if (mailto && status) {
				char buf[MAX_TEMPSTR];

				(void)snprintf(buf, sizeof(buf),
			"mailed %d byte%s of output but got status 0x%04x\n",
					bytes, (bytes==1)?"":"s",
					status);
				log_it(usernm, getpid(), "MAIL", buf);
			}

		} /*if data from grandchild*/

		Debug(DPROC, ("[%ld] got EOF from grandchild\n",
			      (long)getpid()));

		(void)fclose(in);	/* also closes stdout_pipe[READ_PIPE] */
	}

	/* wait for children to die.
	 */
	for (; children > 0; children--) {
		WAIT_T waiter;
		PID_T pid;

		Debug(DPROC, ("[%ld] waiting for grandchild #%d to finish\n",
			      (long)getpid(), children));
		while ((pid = wait(&waiter)) < OK && errno == EINTR)
			;
		if (pid < OK) {
			Debug(DPROC,
			      ("[%ld] no more grandchildren--mail written?\n",
			       (long)getpid()));
			break;
		}
		Debug(DPROC, ("[%ld] grandchild #%ld finished, status=%04x",
			      (long)getpid(), (long)pid, WEXITSTATUS(waiter)));
		if (WIFSIGNALED(waiter) && WCOREDUMP(waiter))
			Debug(DPROC, (", dumped core"));
		Debug(DPROC, ("\n"));
	}

	/* Log the time when we finished deadling with the job */
	/*local*/{
		char *x = mkprints(e->cmd, strlen(e->cmd));

		log_it(usernm, getpid(), "CMD FINISH", x);
		free(x);
	}
}

static int
safe_p(const char *usernm, const char *s) {
	static const char safe_delim[] = "@!:%-.,";     /* conservative! */
	const char *t;
	int ch, first;

	for (t = s, first = 1; (ch = *t++) != '\0'; first = 0) {
		if (isascii(ch) && isprint(ch) &&
		    (isalnum(ch) || (!first && strchr(safe_delim, ch))))
			continue;
		log_it(usernm, getpid(), "UNSAFE", s);
		return (FALSE);
	}
	return (TRUE);
}
