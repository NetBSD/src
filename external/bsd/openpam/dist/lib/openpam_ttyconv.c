/*	$NetBSD: openpam_ttyconv.c,v 1.6.2.2 2012/04/17 00:03:59 yamt Exp $	*/

/*-
 * Copyright (c) 2002-2003 Networks Associates Technology, Inc.
 * Copyright (c) 2004-2011 Dag-Erling Smørgrav
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by ThinkSec AS and
 * Network Associates Laboratories, the Security Research Division of
 * Network Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Id: openpam_ttyconv.c 437 2011-09-13 12:00:13Z des
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <paths.h>

#include <security/pam_appl.h>

#include "openpam_impl.h"

int openpam_ttyconv_timeout = 0;

#ifdef GETPASS_ECHO
static char *
xprompt(const char *msg, FILE *infp, FILE *outfp, FILE *errfp, int fl)
{
	char *rv;
	int fd[3];
	fd[0] = fileno(infp);
	fd[1] = fileno(outfp);
	fd[2] = fileno(errfp);

	rv = getpassfd(msg, NULL, 0, fd, GETPASS_NEED_TTY |
	    GETPASS_FAIL_EOF | GETPASS_NO_SIGNAL |
	    (fl == 0 ? GETPASS_ECHO : 0), openpam_ttyconv_timeout);
	if (rv == NULL)
		fprintf(errfp, " %s\n", strerror(errno));
	else if (fl)
		fputs("\n", errfp);
	return rv;
}
#define prompt(m, i, o, e) xprompt(m, i, o, e, 0)
#define prompt_echo_off(m, i, o, e) xprompt(m, i, o, e, 1)
#else
static void
timeout(int sig)
{
	/*LINTED unused*/
	(void)sig;
}

static char *
prompt(const char *msg, FILE *infp, FILE *outfp, FILE *errfp)
{
	char buf[PAM_MAX_RESP_SIZE];
	struct sigaction action, saved_action;
	sigset_t saved_sigset, sigs;
	unsigned int saved_alarm;
	int eof, error, fd;
	size_t len;
	char *retval;
	char ch;

	saved_alarm = 0;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGINT);
	sigaddset(&sigs, SIGQUIT);
	sigaddset(&sigs, SIGTSTP);
	sigprocmask(SIG_SETMASK, &sigs, &saved_sigset);
	action.sa_handler = &timeout;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	sigaction(SIGALRM, &action, &saved_action);


	fputs(msg, outfp);
	fflush(outfp);
#ifdef HAVE_FPURGE
	fpurge(infp);
#endif
	fd = fileno(infp);
	buf[0] = '\0';
	eof = error = 0;
	saved_alarm = 0;
	if (openpam_ttyconv_timeout >= 0)
		saved_alarm = alarm((unsigned int)openpam_ttyconv_timeout);
	ch = '\0';
	for (len = 0; ch != '\n' && !eof && !error; ++len) {
		switch (read(fd, &ch, (size_t)1)) {
		case 1:
			if (len < PAM_MAX_RESP_SIZE - 1) {
				buf[len + 1] = '\0';
				buf[len] = ch;
			}
			break;
		case 0:
			eof = 1;
			break;
		default:
			error = errno;
			break;
		}
	}
	if (openpam_ttyconv_timeout >= 0)
		alarm(0);
	sigaction(SIGALRM, &saved_action, NULL);
	sigprocmask(SIG_SETMASK, &saved_sigset, NULL);
	if (saved_alarm > 0)
		alarm(saved_alarm);
	if (error == EINTR)
		fputs(" timeout!", errfp);
	if (error || eof) {
		fputs("\n", errfp);
		memset(buf, 0, sizeof(buf));
		return (NULL);
	}
	/* trim trailing whitespace */
	for (len = strlen(buf); len > 0; --len)
		if (buf[len - 1] != '\r' && buf[len - 1] != '\n')
			break;
	buf[len] = '\0';
	retval = strdup(buf);
	memset(buf, 0, sizeof(buf));
	return (retval);
}

static char *
prompt_echo_off(const char *msg, FILE *infp, FILE *outfp, FILE *errfp)
{
	struct termios tattr;
	tcflag_t lflag;
	char *ret;
	int fd;

	fd = fileno(infp);
	if (tcgetattr(fd, &tattr) != 0) {
		openpam_log(PAM_LOG_ERROR, "tcgetattr(): %s", strerror(errno));
		return (NULL);
	}
	lflag = tattr.c_lflag;
	tattr.c_lflag &= ~ECHO;
	if (tcsetattr(fd, TCSAFLUSH, &tattr) != 0) {
		openpam_log(PAM_LOG_ERROR, "tcsetattr(): %s", strerror(errno));
		return (NULL);
	}
	ret = prompt(msg, infp, outfp, errfp);
	tattr.c_lflag = lflag;
	(void)tcsetattr(fd, TCSANOW, &tattr);
	if (ret != NULL)
		fputs("\n", outfp);
	return (ret);
}
#endif

/*
 * OpenPAM extension
 *
 * Simple tty-based conversation function
 */

int
openpam_ttyconv(int n,
	 const struct pam_message **msg,
	 struct pam_response **resp,
	 void *data)
{
	struct pam_response *aresp;
	int i;
	FILE *infp, *outfp, *errfp;

	ENTER();
	/*LINTED unused*/
	(void)data;
	if (n <= 0 || n > PAM_MAX_NUM_MSG)
		RETURNC(PAM_CONV_ERR);
	if ((aresp = calloc((size_t)n, sizeof *aresp)) == NULL)
		RETURNC(PAM_BUF_ERR);

	/*
	 * read and write to /dev/tty if possible; else read from
	 * stdin and write to stderr.
	 */ 
	if ((outfp = infp = errfp = fopen(_PATH_TTY, "w+")) == NULL) {
		errfp = stderr;
		outfp = stderr;
		infp = stdin;
	} 

	for (i = 0; i < n; ++i) {
		aresp[i].resp_retcode = 0;
		aresp[i].resp = NULL;
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
			aresp[i].resp = prompt_echo_off(msg[i]->msg, infp,
			    outfp, errfp);
			if (aresp[i].resp == NULL)
				goto fail;
			break;
		case PAM_PROMPT_ECHO_ON:
			aresp[i].resp = prompt(msg[i]->msg, infp, outfp, errfp);
			if (aresp[i].resp == NULL)
				goto fail;
			break;
		case PAM_ERROR_MSG:
			fputs(msg[i]->msg, errfp);
			if (strlen(msg[i]->msg) > 0 &&
			    msg[i]->msg[strlen(msg[i]->msg) - 1] != '\n')
				fputc('\n', errfp);
			break;
		case PAM_TEXT_INFO:
			fputs(msg[i]->msg, outfp);
			if (strlen(msg[i]->msg) > 0 &&
			    msg[i]->msg[strlen(msg[i]->msg) - 1] != '\n')
				fputc('\n', outfp);
			break;
		default:
			goto fail;
		}
	}
	if (infp != stdin)
		(void)fclose(infp);
	*resp = aresp;
	RETURNC(PAM_SUCCESS);
fail:
	for (i = 0; i < n; ++i) {
		if (aresp[i].resp != NULL) {
			memset(aresp[i].resp, 0, strlen(aresp[i].resp));
			FREE(aresp[i].resp);
		}
	}
	if (infp != stdin)
		(void)fclose(infp);
	memset(aresp, 0, n * sizeof *aresp);
	FREE(aresp);
	*resp = NULL;
	RETURNC(PAM_CONV_ERR);
	/*NOTREACHED*/
}

/*
 * Error codes:
 *
 *	PAM_SYSTEM_ERR
 *	PAM_BUF_ERR
 *	PAM_CONV_ERR
 */

/**
 * The =openpam_ttyconv function is a standard conversation function
 * suitable for use on TTY devices.
 * It should be adequate for the needs of most text-based interactive
 * programs.
 *
 * The =openpam_ttyconv function displays a prompt to, and reads in a
 * password from /dev/tty. If this file is not accessible, =openpam_ttyconv
 * displays the prompt on the standard error output and reads from the
 * standard input.
 *
 * The =openpam_ttyconv function allows the application to specify a
 * timeout for user input by setting the global integer variable
 * :openpam_ttyconv_timeout to the length of the timeout in seconds.
 *
 * >openpam_nullconv
 * >pam_prompt
 * >pam_vprompt
 * >getpass
 */
