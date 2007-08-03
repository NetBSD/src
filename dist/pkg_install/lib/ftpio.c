/*	$NetBSD: ftpio.c,v 1.1.1.2 2007/08/03 13:58:21 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
__RCSID("$NetBSD: ftpio.c,v 1.1.1.2 2007/08/03 13:58:21 joerg Exp $");
#endif

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1999 Hubert Feyrer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Hubert Feyrer for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_REGEX_H
#include <regex.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_TERMCAP_H
#include <termcap.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef EXPECT_DEBUG
#if HAVE_VIS_H
#include <vis.h>
#endif
#endif

#include "../lib/lib.h"

/*
 * Names of environment variables used to pass things to
 * subprocesses, for connection caching.
 */
#define PKG_FTPIO_COMMAND		"PKG_FTPIO_COMMAND"
#define PKG_FTPIO_ANSWER		"PKG_FTPIO_ANSWER"
#define PKG_FTPIO_CNT			"PKG_FTPIO_CNT"
#define PKG_FTPIO_CURRENTHOST		"PKG_FTPIO_CURRENTHOST"
#define PKG_FTPIO_CURRENTDIR		"PKG_FTPIO_CURRENTDIR"

#undef STANDALONE   /* define for standalone debugging */

/* File descriptors */
typedef struct {
    int command;
    int answer;
} fds;


#if EXPECT_DEBUG
static int	 expect_debug = 1;
#endif /* EXPECT_DEBUG */
static int	 needclose=0;
static int	 ftp_started=0;
static fds	 ftpio;
static int	 ftp_pid;
static char	 term[1024];
static char	 bold_on[1024];
static char	 bold_off[1024];

static char *ftp_expand_URL(const char *, char *);
static int hexvalue(char);
static char *http_expand_URL(const char *, char *);
static int http_extract_fn(char *, char *, size_t);
static void URL_decode(char *);

/*
 * expect "str" (a regular expression) on file descriptor "fd", storing
 * the FTP return code of the command in the integer "ftprc". The "str"
 * string is expected to match some FTP return codes after a '\n', e.g.
 * "\n(550|226).*\n" 
 */
static int
expect(int fd, const char *str, int *ftprc)
{
	int rc;
	char buf[256];  
#if EXPECT_DEBUG
	char *vstr;
#endif /* EXPECT_DEBUG */
	regex_t rstr;
	int done;
	struct pollfd set[1];
	int retval;
	regmatch_t match;
	int verbose_expect=0;

#if EXPECT_DEBUG
	vstr=malloc(2*sizeof(buf));
	if (vstr == NULL)
		err(EXIT_FAILURE, "expect: malloc() failed");
	strvis(vstr, str, VIS_NL|VIS_SAFE|VIS_CSTYLE);
#endif /* EXPECT_DEBUG */
	    
	if (regcomp(&rstr, str, REG_EXTENDED) != 0)
		err(EXIT_FAILURE, "expect: regcomp() failed");

#if EXPECT_DEBUG
	if (expect_debug)
		printf("expecting \"%s\" on fd %d ...\n", vstr, fd);
#endif /* EXPECT_DEBUG */

	if(0) setbuf(stdout, NULL);

	memset(buf, '\n', sizeof(buf));

	done=0;
	retval=0;
	set[0].fd = fd;
	set[0].events = POLLIN;
	while(!done) {
		rc = poll(set, 1, 60*60*1000);    /* seconds until next message from tar */
		switch (rc) {
		case -1:
			if (errno == EINTR)
				break;
			warn("expect: poll() failed (probably ftp died because of bad args)");
			done = 1;
			retval = -1;
			break;
		case 0:
			warnx("expect: poll() timeout");
			/* need to send ftp coprocess SIGINT to make it stop
			 * downloading into dir that we'll blow away in a second */
			kill(ftp_pid, SIGINT);

			/* Wait until ftp coprocess is responsive again 
			 * XXX Entering recursion here!
			 */
			rc = ftp_cmd("cd .\n", "\n(550|250).*\n");
			if (rc != 250) {
				/* now we have a really good reason to bail out ;) */
			}
			/* ftp is at command prompt again, and will wait for our
			 * next command. If we were downloading, we can now safely
			 * continue and remove the dir that the tar command was
			 * expanding to */
		    
			done = 1;	/* hope that's ok */
			retval = -1;
			break;
		default:
			if (set[0].revents & POLLHUP) {
				done = 1;
				retval = -1;
				break;
			}

			rc = read(fd, &buf[sizeof(buf) - 1], 1);
			if (rc <= 0) {
				done = 1;
				retval = -1;
				break;
			}

			if (verbose_expect)
				putchar(buf[sizeof(buf)-1]);

#if EXPECT_DEBUG
			{
				char *v=malloc(2*sizeof(buf));
				strvis(v, buf, VIS_NL|VIS_SAFE|VIS_CSTYLE);
				if (expect_debug)
					printf("expect=<%s>, buf=<%*s>\n", vstr, strlen(v), v);
				free(v);
			}
#endif /* EXPECT_DEBUG */

			if (regexec(&rstr, buf, 1, &match, 0) == 0) {
#if EXPECT_DEBUG
				if (expect_debug)
					printf("Gotcha -> %s!\n", buf+match.rm_so+1);
				fflush(stdout);
#endif /* EXPECT_DEBUG */

				if (ftprc && isdigit((unsigned char)buf[match.rm_so+1])) 
					*ftprc = atoi(buf+match.rm_so+1);

				done=1;
				retval=0;
			}
	    
			memmove(buf, buf+1, sizeof(buf)-1); /* yes, this is non-performant */
			break;
		}
	}

#if EXPECT_DEBUG
	printf("done.\n");

	if (str)
		free(vstr);
#endif /* EXPECT_DEBUG */

	return retval;
}

/*
 * send a certain ftp-command "cmd" to our FTP coprocess, and wait for
 * "expectstr" to be returned. Return numeric FTP return code or -1
 * in case of an error (usually expect() timeout) 
 */
int
ftp_cmd(const char *cmd, const char *expectstr)
{
	int rc=0, verbose_ftp=0;
	int len;

	if (Verbose)
		verbose_ftp=1;
    
	if (verbose_ftp)
		fprintf(stderr, "\n%sftp> %s%s", bold_on, cmd, bold_off);
    
	fflush(stdout);
	len = write(ftpio.command, cmd, strlen(cmd));
	if (len == strlen(cmd)) {
		if (expectstr) {
			/* set "rc" to the FTP error code: */
			if (expect(ftpio.answer, expectstr, &rc) == -1)
				rc = -1;	/* some error occurred */
		}
	} else {	
		if (Verbose)
			warn("short write");
	}

	return rc;
}


/*
 * Really fire up FTP coprocess
 */
static int
setupCoproc(const char *base)
{
	int command_pipe[2];
	int answer_pipe[2];
	int rc1, rc2;
	char buf[20];
	char *argv0 = (char *)strrchr(FTP_CMD, '/');
	if (argv0 == NULL)
		argv0 = FTP_CMD;
	else
		argv0++;

	rc1 = pipe(command_pipe);
	rc2 = pipe(answer_pipe);

	if(rc1==-1 || rc2==-1) {
		warn("setupCoproc: pipe() failed");
		return -1;
	}

	if (command_pipe[0] == -1 || command_pipe[1] == -1 ||
	    answer_pipe[0] == -1  || answer_pipe[1] == -1 ) {
		warn("setupCoproc: pipe() returned bogus descriptor");
		return -1;
	}

	rc1 = fork();
	switch (rc1) {
	case -1:
		/* Error */
	    
		warn("setupCoproc: fork() failed");
		return -1;
		break;

	case 0:
		/* Child */
	    
		(void) close(command_pipe[1]);
		rc1 = dup2(command_pipe[0], 0);
		if (rc1 == -1) {
			err(EXIT_FAILURE, "setupCoproc: dup2 failed (command_pipe[0])");
		}
		(void) close(command_pipe[0]);
	    
		(void) close(answer_pipe[0]);
		rc1 = dup2(answer_pipe[1], 1);
		if (rc1 == -1) {
			err(EXIT_FAILURE, "setupCoproc: dup2 failed (answer_pipe[1])");
		}
		(void) close(answer_pipe[1]);
	    
		setbuf(stdout, NULL);
	    
		if (Verbose)
			fprintf(stderr, "%sftp -detv %s%s\n", bold_on, base, bold_off);
		rc1 = execlp(FTP_CMD, argv0, "-detv", base, NULL);
		warn("setupCoproc: execlp() failed");
		exit(1);
		break;
	default: 
		/* Parent */
		(void) close(command_pipe[0]);
		(void) close(answer_pipe[1]);
	    
		(void) snprintf(buf, sizeof(buf), "%d", command_pipe[1]);
		setenv(PKG_FTPIO_COMMAND, buf, 1);
		(void) snprintf(buf, sizeof(buf), "%d", answer_pipe[0]);
		setenv(PKG_FTPIO_ANSWER, buf, 1);
	    
		ftpio.command = command_pipe[1];
		ftpio.answer  = answer_pipe[0];
		ftp_pid = rc1;			/* to ^C transfers */
	    
		fcntl(ftpio.command, F_SETFL, O_NONBLOCK);
		fcntl(ftpio.answer , F_SETFL, O_NONBLOCK);
	    
		break;
	}

	return 0;
}


/*
 * Dummy signal handler to detect if the ftp(1) coprocess or
 * and of the processes of the tar/gzip pipeline dies. 
 */
static void
sigchld_handler (int n)
{
	/* Make poll(2) return EINTR */
}


/*
 * SIGPIPE only happens when there's something wrong with the FTP
 * coprocess. In that case, set mark to not try to close shut down
 * the coprocess.
 */
static void
sigpipe_handler(int n)
{
	/* aparently our ftp companion died */
	if (Verbose)
		fprintf(stderr, "SIGPIPE!\n");
	needclose = 0;
}


/*
 * Close the FTP coprocess' current connection, but
 * keep the process itself alive. 
 */
void 
ftp_stop(void)
{
#if defined(__svr4__) && defined(__sun__)
	char	env[BUFSIZ];
#endif
	const char *tmp1, *tmp2;
	
	if (!ftp_started)
		return;
	
	tmp1=getenv(PKG_FTPIO_COMMAND);
	tmp2=getenv(PKG_FTPIO_ANSWER);
	
	/* (Only) the last one closes the link */
	if (tmp1 != NULL && tmp2 != NULL) {
		if (needclose)
			ftp_cmd("close\n", "\n(221 .*|Not connected.)\n");
		
		(void) close(ftpio.command);
		(void) close(ftpio.answer);
	}

#if defined(__svr4__) && defined(__sun__)
	(void) snprintf(env, sizeof(env), "%s=", PKG_FTPIO_COMMAND);
	putenv(env);
	(void) snprintf(env, sizeof(env), "%s=", PKG_FTPIO_ANSWER);
	putenv(env);
#else
	unsetenv(PKG_FTPIO_COMMAND); 
	unsetenv(PKG_FTPIO_ANSWER);
#endif
}


/*
 * (Start and re-)Connect the FTP coprocess to some host/dir.
 * If the requested host/dir is different than the one that the
 * coprocess is currently at, close first. 
 */
int
ftp_start(const char *base)
{
	const char *tmp1, *tmp2;
	char *p;
	int rc;
	char newHost[MAXHOSTNAMELEN];
	const char *newDir;
	const char *currentHost=getenv(PKG_FTPIO_CURRENTHOST);
	const char *currentDir=getenv(PKG_FTPIO_CURRENTDIR);
	int urllen;

	/* talk to termcap for bold on/off escape sequences */
	if (getenv("TERM") != NULL && tgetent(term, getenv("TERM")) > 0) {
		p = bold_on;  tgetstr("md", &p);
		p = bold_off; tgetstr("me", &p);
	} else {
		bold_on[0]  = '\0';
		bold_off[0] = '\0';
	}
	
	fileURLHost(base, newHost, sizeof(newHost));
	urllen = URLlength(base);
	if (urllen < 0 || !(newDir = strchr(base + URLlength(base), '/')))
		errx(EXIT_FAILURE, "ftp_start: bad URL '%s'", base);
	newDir++;
	if (currentHost
	    && currentDir
	    && ( strcmp(newHost, currentHost) != 0
		 || strcmp(newDir, currentDir) != 0)) { /* could handle new dir case better here, w/o reconnect */
		if (Verbose) {
			printf("ftp_start: new host or dir, stopping previous connect...\n");
			printf("currentHost='%s', newHost='%s'\n", currentHost, newHost);
			printf("currentDir='%s', newDir='%s'\n", currentDir, newDir);
		}
		
		ftp_stop();
		
		if (Verbose)
			printf("ftp stopped\n");
	}
	setenv(PKG_FTPIO_CURRENTHOST, newHost, 1); /* need to update this in the environment */
	setenv(PKG_FTPIO_CURRENTDIR, newDir, 1);   /* for subprocesses to have this available */
	
	tmp1=getenv(PKG_FTPIO_COMMAND);
	tmp2=getenv(PKG_FTPIO_ANSWER);
	if(tmp1==NULL || tmp2==NULL || *tmp1=='\0' || *tmp2=='\0') {
		/* no FTP coprocess running yet */
		
		if (Verbose)
			printf("Spawning FTP coprocess\n");
		
		rc = setupCoproc(base);
		if (rc == -1) {
			warnx("setupCoproc() failed");
			return -1;
		}
		
		needclose=1;
		signal(SIGPIPE, sigpipe_handler);
		signal(SIGCHLD, sigchld_handler);

		if ((expect(ftpio.answer, "\n(221|250|221|550).*\n", &rc) != 0)
		    || rc != 250) {
			warnx("expect1 failed, rc=%d", rc);
			return -1;
		}
		
		/* nbftp now issues a CWD for each part of the path
		 * and will return a code for each of them. No idea how to
		 * deal with that other than to issue a 'prompt off' to
		 * get something that we can wait for and that does NOT
		 * look like a CWD command's output */
		rc = ftp_cmd("prompt off\n", "\n(Interactive mode off|221).*\n");
		if ((rc == 221) || (rc == -1)) {
			/* something is wrong */
			ftp_started=1; /* not really, but for ftp_stop() */
			ftp_stop();
			warnx("prompt failed - wrong dir?");
			return -1;
		}
		
		ftp_started=1;
	} else {
		/* get FDs of our coprocess */
		
		ftpio.command = dup(atoi(tmp1));
		if (ftpio.command == -1 ) {
			warnx("command dup() failed, increase 'descriptors' limit");
			return -1;
		}
		ftpio.answer  = dup(atoi(tmp2));
		if (ftpio.answer == -1 ) {
			warnx("answer dup() failed, increase 'descriptors' limit");
			return -1;
		}
		
		if (Verbose)
			printf("Reusing FDs %s/%s for communication to FTP coprocess\n", tmp1, tmp2);
		
		fcntl(ftpio.command, F_SETFL, O_NONBLOCK);
		fcntl(ftpio.answer , F_SETFL, O_NONBLOCK);
	}
	
	return 0;
}


/*
 * Expand the given wildcard URL "wildcardurl" if possible, and store the
 * expanded value into "expandedurl". return 0 if successful, -1 else.
 */
int
expandURL(char *expandedurl, const char *wildcardurl)
{
	char *pattern;
	char *bestmatch;
	char base[MaxPathSize];

	pattern=strrchr(wildcardurl, '/');
	if (pattern == NULL){
		warnx("expandURL: no '/' in URL %s?!", wildcardurl);
		return -1;
	}
	if (pattern-strchr(wildcardurl, '/') < 2) {
		/* only one or two slashes in total */
		warnx("expandURL: not enough '/' in URL %s", wildcardurl);
		return -1;
	}
	(void) snprintf(base, sizeof(base), "%.*s/",
			(int)(pattern-wildcardurl), wildcardurl);
	pattern++;

	if (strncmp(wildcardurl, "ftp://", 6) == 0)
		bestmatch=ftp_expand_URL(base, pattern);
	else if (strncmp(wildcardurl, "http://", 7) == 0)
		bestmatch=http_expand_URL(base, pattern);
	else {
		warnx("expandURL: unknown protocol in URL `%s'", wildcardurl);
		return -1;
	}

	/* no match found */
	if (bestmatch == NULL)
		return -1;

	snprintf(expandedurl, MaxPathSize, "%s%s", base, bestmatch);
	if (Verbose)
		printf("best match: '%s'\n", expandedurl);

	return 0;
}

/* for a given wildcard ftp:// URL, find the best matching pkg */
static char *
ftp_expand_URL(const char *base, char *pattern)
{
	char *s, buf[MaxPathSize];
	char tmpname[MaxPathSize];
	char best[MaxPathSize];
	int rc, got_list, tfd, retry_tbz;

	retry_tbz = 0;
	best[0]='\0';

	rc = ftp_start(base);
	if (rc == -1) {
		warnx("ftp_start() failed");
		return NULL;
	}

	strlcpy(tmpname, "/var/tmp/pkg.XXXXXX", sizeof(tmpname));
	tfd=mkstemp(tmpname);
	if (tfd == -1) {
		warnx("Cannot generate temp file for ftp(1)'s nlist output");
		return NULL;
	}
	close(tfd); /* We don't need the file descriptor, but will use 
		       the file in a second */

	s=strpbrk(pattern, "<>[]?*{"); /* Could leave out "[]?*" here;
					* ftp(1) is not that stupid */
	if (!s) {
		/* This should only happen when getting here with (only) a package
		 * name specified to pkg_add, and PKG_PATH containing some URL.
		 */
		(void) snprintf(buf, sizeof(buf), "nlist %s %s\n", pattern, tmpname);
	} else {
		/* replace possible version(wildcard) given with "-*".
		 * we can't use the pkg wildcards here as dewey compare
		 * and alternates won't be handled by ftp(1); sort
		 * out later, using pkg_match() */
		if (retry_tbz) {
retry_with_tbz:
			(void) snprintf(buf,  sizeof(buf), "nlist %.*s*.tbz %s\n",
					(int)(s-pattern), pattern, tmpname);
			retry_tbz = 0;
		} else {
			(void) snprintf(buf,  sizeof(buf), "nlist %.*s*.tgz %s\n",
					(int)(s-pattern), pattern, tmpname);
			retry_tbz = 1;
		}
	}

	rc = ftp_cmd(buf, "\n(550|450|226).*\n"); /* catch errors */
	if (rc != 226)
		got_list = 0;
	else
		got_list = 1;

	/* Sync - don't remove */
	rc = ftp_cmd("cd .\n", "\n(550|250|257).*\n");
	if (rc != 250) {
		warnx("chdir failed!");
		unlink(tmpname);	/* remove clutter */
		return NULL;
	}
	
	if (got_list == 1 && access(tmpname, R_OK)==0) {
		int matches;
		FILE *f;
		char filename[MaxPathSize];

		f=fopen(tmpname, "r");
		if (f == NULL) {
			warn("fopen");
			unlink(tmpname);	/* remove clutter */
			return NULL;
		}
		matches=0;
		/* The following loop is basically the same as the readdir() loop
		 * in findmatchingname() */
		while (fgets(filename, sizeof(filename), f)) {

			/*
			 * We need to strip off any .t[bg]z etc.
			 * suffix here
			 */

			char s_filename[MaxPathSize];
			char s_pattern[MaxPathSize];
	    
			filename[strlen(filename)-1] = '\0';

			strip_txz(s_filename, NULL, filename);
			strip_txz(s_pattern, NULL, pattern);

			if (pkg_match(s_pattern, s_filename)) {
				matches++;

				/* compare findbestmatchingname() */
				findbestmatchingname_fn(pattern, filename, best);
			}
		}
		(void) fclose(f);

		if (matches == 0 && Verbose)
			warnx("nothing appropriate found");
	}

	if (retry_tbz)
		goto retry_with_tbz;

	unlink(tmpname);

	if (best[0] == '\0')
		return NULL;

	return strdup(best);
}

/* for a given wildcard http:// URL, find the best matching pkg */
static char *
http_expand_URL(const char *base, char *pattern)
{
	char    best[MaxPathSize];
	char    line[BUFSIZ];
	char    filename[MaxPathSize];
	FILE   *fp;
	int	pipefds[2];
	int     state;
	pid_t   pid;

	*best = '\0';

	/* Set up a pipe for getting the file list */
	if (pipe(pipefds) == -1) {
		warnx("cannot create pipe");
		return NULL;
	}
	if ((pid = fork()) == -1) {
		warnx("cannot fork ftp process");
		return NULL;
	}
	if (pid == 0) {		/* The child */
		if (dup2(pipefds[1], STDOUT_FILENO) == -1) {
			warnx("dup2 failed before starting ftp");
			_exit(2);
		}
		close(pipefds[0]);
		close(pipefds[1]);
		/* get URL contents to stdout and thus to parent,
		 * silently */
		execlp("ftp", "ftp", "-V", "-o", "-", base, NULL);
		warnx("failed to execute ftp");
		_exit(2);
	}

	/* parent */
	close(pipefds[1]);

	if ((fp=fdopen(pipefds[0], "r")) == NULL)
		warn("can't fdopen pipe end");
	else {
		char s_pattern[MaxPathSize];
		int len, offset;

		/* strip of .t[bg]z for comparison */
		strip_txz(s_pattern, NULL, pattern);

		/* initialize http_extract_fn internal state */
		http_extract_fn(NULL, NULL, 0);

		/* read line from HTTP output and extract filenames */
		while (fgets(line, sizeof(line), fp) != NULL) {
			len = offset = 0;
			while ((len=http_extract_fn(line+offset, filename,
						      sizeof(filename))) > 0) {
				char s_filename[MaxPathSize];

				offset += len;
				strip_txz(s_filename, NULL, filename);

				if (pkg_match(s_pattern, s_filename)) {
					/* compare findbestmatchingname() */
					findbestmatchingname_fn(pattern,
					    filename, best);
				}
			}
		}

	}

	fclose(fp);

	/* wait for child to exit */
	if (waitpid(pid, &state, 0) < 0) {
		/* error has been reported by child */
		return NULL;
	}

	if (best[0] == '\0') {
		if (Verbose)
			warnx("nothing appropriate found");
		return NULL;
	}

	return strdup(best);

}

enum http_states {
	ST_NONE,
	ST_LT, ST_LTA, ST_TAGA, ST_H, ST_R, ST_E, ST_F, ST_HREF,
	ST_TAG, ST_TAGAX
};

/* return any hrefs found */
static int
http_extract_fn(char *input, char *outbuf, size_t outbuflen)
{
	/* partial copied hrefs from previous calls are saved here */
	static char tempbuf[MaxPathSize];
	/* fill state of tempbuf */
	static int tempbuffill = 0;
	/* parsing state information */
	static enum http_states state;
	/* currently in double quotes (in parsing) */
	static int dqflag;
	char p;
	int offset, found;

	if (outbuf == NULL) {
		/* init */
		dqflag = tempbuffill = 0;
		state = ST_NONE;
		return 0;
	}

	offset = 0;
	found = 0;
	while ((p=input[offset++]) != '\0') {
		/* handle anything that's inside double quotes */
		if (dqflag) {
			/* incomplete href */
			if (state == ST_HREF) {
				/* check if space left in output
				 * buffer */
				if (tempbuffill >= sizeof(tempbuf)) {
					warnx("href starting with `%.*s'"
					      " too long", 60, tempbuf);
					/* ignore remainder */
					tempbuffill = 0;
					/* need space before "href"
					 * can start again (invalidly,
					 * of course, but we don't
					 * care) */
					state = ST_TAGAX;
				}

				/* href complete */
				if (p == '\"') {
					/* complete */
					dqflag = 0;
					tempbuf[tempbuffill++] = '\0';
					/* need space before "href"
					 * can start again (invalidly,
					 * of course, but we don't
					 * care) */
					state = ST_TAGAX;
					found = 1;
					break;
				} else {
					/* copy one more char */
					tempbuf[tempbuffill++] = p;
				}
			} else {
				/* leaving double quotes */
				if (p == '\"')
					dqflag = 0;
			}
			continue;
		}

		/*
		 * entering double quotes? (only relevant inside a tag)
		 */
		if (state != ST_NONE && p == '\"') {
			dqflag = 1;
			continue;
		}

		/* other cases */
		switch (state) {
		case ST_NONE:
			/* plain text, not in markup */
			if (p == '<')
				state = ST_LT;
			break;
		case ST_LT:
			/* in tag -- "<" already found */
			if (p == '>')
				state = ST_NONE;
			else if (p == 'a' || p == 'A')
				state = ST_LTA;
			else if (!isspace((unsigned char)p))
				state = ST_TAG;
			break;
		case ST_LTA:
			/* in tag -- "<a" already found */
			if (p == '>')
				state = ST_NONE;
			else if (isspace((unsigned char)p))
				state = ST_TAGA;
			else
				state = ST_TAG;
			break;
		case ST_TAG:
			/* in tag, but not "<a" -- disregard */
			if (p == '>')
				state = ST_NONE;
			break;
		case ST_TAGA:
			/* in a-tag -- "<a " already found */
			if (p == '>')
				state = ST_NONE;
			else if (p == 'h' || p == 'H')
				state = ST_H;
			else if (!isspace((unsigned char)p))
				state = ST_TAGAX;
			break;
		case ST_TAGAX:
			/* in unknown keyword in a-tag */
			if (p == '>')
				state = ST_NONE;
			else if (isspace((unsigned char)p))
				state = ST_TAGA;
			break;
		case ST_H:
			/* in a-tag -- "<a h" already found */
			if (p == '>')
				state = ST_NONE;
			else if (p == 'r' || p == 'R')
				state = ST_R;
			else if (isspace((unsigned char)p))
				state = ST_TAGA;
			else
				state = ST_TAGAX;
			break;
		case ST_R:
			/* in a-tag -- "<a hr" already found */
			if (p == '>')
				state = ST_NONE;
			else if (p == 'e' || p == 'E')
				state = ST_E;
			else if (isspace((unsigned char)p))
				state = ST_TAGA;
			else
				state = ST_TAGAX;
			break;
		case ST_E:
			/* in a-tag -- "<a hre" already found */
			if (p == '>')
				state = ST_NONE;
			else if (p == 'f' || p == 'F')
				state = ST_F;
			else if (isspace((unsigned char)p))
				state = ST_TAGA;
			else
				state = ST_TAGAX;
			break;
		case ST_F:
			/* in a-tag -- "<a href" already found */
			if (p == '>')
				state = ST_NONE;
			else if (p == '=')
				state = ST_HREF;
			else if (!isspace((unsigned char)p))
				state = ST_TAGAX;
			break;
		case ST_HREF:
			/* in a-tag -- "<a href=" already found */
			/* XXX: handle missing double quotes? */
			if (p == '>')
				state = ST_NONE;
			/* skip spaces before URL */
			else if (!isspace((unsigned char)p))
				state = ST_TAGA;
			break;
			/* no default case by purpose */
		}
	}

	if (p == '\0')
		return -1;

	if (found) {
		char *q;

		URL_decode(tempbuf);

		/* strip path (XXX) */
		if ((q=strrchr(tempbuf, '/')) == NULL)
			q = tempbuf;
			
		(void)strlcpy(outbuf, q, outbuflen);
		tempbuffill = 0;
	}
		
	return offset;
}


static int
hexvalue(char p)
{
	if (p >= '0' && p <= '9')
		return (p-'0');
	else if (p >= 'a' && p <= 'f')
		return (p-'a'+10);
	else if (p >= 'A' && p <= 'F')
		return (p-'A'+10);
	else
		return -1;
}

/* fetch and extract URL url into directory path */
static int
http_fetch(const char *url, const char *path)
{
	int     pipefds[2];
	int	stateftp, state;
	pid_t   pidftp, pid;

	/* Set up a pipe for passing the fetched contents. */
	if (pipe(pipefds) == -1) {
		warn("cannot create pipe");
		return -1;
	}
	/* fork ftp child */
	if ((pidftp = fork()) == -1) {
		warn("cannot fork process for ftp");
		return -1;
	}
	if (pidftp == 0) {
		/* child */
		if (dup2(pipefds[1], STDOUT_FILENO) == -1) {
			warn("dup2 failed before executing ftp");
			_exit(2);
		}
		close(pipefds[0]);
		close(pipefds[1]);
		execlp(FTP_CMD, FTP_CMD, "-o", "-", url, NULL);
		warnx("failed to execute ftp");
		_exit(2);
	}

	/* fork unpack child */
	if ((pid = fork()) == -1) {
		warn("cannot fork unpack process");
		return -1;
	}
	if (pid == 0) {
		/* child */
		if (dup2(pipefds[0], STDIN_FILENO) == -1) {
			warn("dup2 failed before unpack");
			_exit(2);
		}
		close(pipefds[0]);
		close(pipefds[1]);
		if ((path != NULL) && (chdir(path) < 0))
			_exit(127);

		if (unpack("-", NULL) != 0) {
			warnx("unpack failed");
			_exit(2);
		}

		_exit(0);
	}

	close(pipefds[0]);
	close(pipefds[1]);

	/* wait for unpack to exit */
	while (waitpid(pid, &state, 0) < 0) {
		if (errno != EINTR) {
			(void)waitpid(pidftp, &stateftp, 0);
			return -1;
		}
	}
	while (waitpid(pidftp, &stateftp, 0) < 0) {
		if (errno != EINTR) {
			return -1;
		}
	}

	if (!WIFEXITED(state) || !WIFEXITED(stateftp))
		return -1;

	if (WEXITSTATUS(state) != 0 || WEXITSTATUS(stateftp) != 0)
		return -1;

	return 0;
}

static void
URL_decode(char *URL)
{
	char *in, *out;

	in = out = URL;

	while (*in != '\0') {
		if (in[0] == '%' && in[1] != '\0' && in[2] != '\0') {
			/* URL-decode character */
			if (hexvalue(in[1]) != -1 && hexvalue(in[2]) != -1) {
				*out++ = hexvalue(in[1])*16+hexvalue(in[2]);
			}
			/* skip invalid encoded signs too */
			in += 3;
		}
		else
			*out++ = *in++;
	}

	*out = '\0';

	return;
}
/*
 * extract the given (expanded) URL "url" to the given directory "dir"
 * return -1 on error, 0 else;
 */
int
unpackURL(const char *url, const char *dir)
{
	char *pkg;
	int rc;
	char base[MaxPathSize];
	char pkg_path[MaxPathSize];

	{
		/* Verify if the URL is really ok */
		char expnd[MaxPathSize];

		rc=expandURL(expnd, url);
		if (rc == -1) {
			warnx("unpackURL: verification expandURL failed");
			return -1;
		}
		if (strcmp(expnd, url) != 0) {
			warnx("unpackURL: verification expandURL failed, '%s'!='%s'",
			      expnd, url);
			return -1;
		}
	}
	
	pkg=strrchr(url, '/');
	if (pkg == NULL){
		warnx("unpackURL: no '/' in URL %s?!", url);
		return -1;
	}
	(void) snprintf(base, sizeof(base), "%.*s/", (int)(pkg-url), url);
	(void) snprintf(pkg_path, sizeof(pkg_path), "%.*s",
                 (int)(pkg-url), url); /* no trailing '/' */
	pkg++;

	/* Leave a hint for any depending pkgs that may need it */
	if (getenv("PKG_PATH") == NULL) {
		setenv("PKG_PATH", pkg_path, 1);
#if 0
		path_create(pkg_path); /* XXX */
#endif
		if (Verbose)
			printf("setenv PKG_PATH='%s'\n", pkg_path);
	}

	if (strncmp(url, "http://", 7) == 0)
	    return http_fetch(url, dir);

	rc = ftp_start(base);
	if (rc == -1) {
		warnx("ftp_start() failed");
		return -1; /* error */
	}

	{
		char cmd[1024];
		const char *decompress_cmd = NULL;
		const char *suf;

		if (Verbose)
			printf("unpackURL '%s' to '%s'\n", url, dir);

		suf = suffix_of(pkg);
		if (!strcmp(suf, "tbz") || !strcmp(suf, "bz2"))
			decompress_cmd = BZIP2_CMD;
		else if (!strcmp(suf, "tgz") || !strcmp(suf, "gz"))
			decompress_cmd = GZIP_CMD;
		else if (!strcmp(suf, "tar"))
			; /* do nothing */
		else
			errx(EXIT_FAILURE, "don't know how to decompress %s, sorry", pkg);

		/* yes, this is gross, but needed for borken ftp(1) */
		(void) snprintf(cmd, sizeof(cmd), "get %s \"| ( cd %s; " TAR_CMD " %s %s -vvxp -f - | tee %s )\"\n",
		    pkg, dir,
		    decompress_cmd != NULL ? "--use-compress-program" : "",
		    decompress_cmd != NULL ? decompress_cmd : "",
		    Verbose ? "/dev/stderr" : "/dev/null");

		rc = ftp_cmd(cmd, "\n(226|550).*\n");
		if (rc != 226) {
			warnx("Cannot fetch file (%d!=226)!", rc);
			return -1;
		}
	}

	return 0;
}


#ifdef STANDALONE
static void
usage(void)
{
	errx(EXIT_FAILURE, "Usage: foo [-v] ftp://-pattern");
}


int
main(int argc, char *argv[])
{
	int rc, ch;
	char *argv0 = argv[0];

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			Verbose=1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc<1)
		usage();

	while(argv[0] != NULL) {
		char newurl[MaxPathSize];
	    
		printf("Expand %s:\n", argv[0]);
		rc = expandURL(newurl, argv[0]);
		if (rc==-1)
			warnx("Cannot expand %s", argv[0]);
		else
			printf("Expanded URL: %s\n", newurl);

		/* test out connection caching */
		if (1) {
			char *s, buf[MaxPathSize];
		    
			if ((s=getenv(PKG_FTPIO_CNT)) && atoi(s)>0){
				(void) snprintf(buf, sizeof(buf),"%d", atoi(s)-1);
				setenv(PKG_FTPIO_CNT, buf, 1);
			    
				printf("%s>>> %s -v %s\n", s, argv0, argv[0]);
				fexec(argv0, "-v", argv[0], NULL);
			}
		}
	    
		printf("\n\n\n");
		argv++;
	}

	ftp_stop();

	return 0;
}

void
cleanup(int i)
{
}
#endif /* STANDALONE */
