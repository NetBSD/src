/*	$NetBSD: sftp-connect.c,v 1.2 2006/11/21 23:09:23 pooka Exp $	*/

/*	NetBSD: sftp.c,v 1.21 2006/09/28 21:22:15 christos Exp */
/* $OpenBSD: sftp.c,v 1.91 2006/08/03 03:34:42 deraadt Exp $ */
/*
 * Copyright (c) 2001-2004 Damien Miller <djm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "includes.h"
__RCSID("$NetBSD: sftp-connect.c,v 1.2 2006/11/21 23:09:23 pooka Exp $");
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <errno.h>
#include <glob.h>
#include <histedit.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "log.h"
#include "pathnames.h"
#include "misc.h"

#include "sftp.h"
#include "buffer.h"
#include "sftp-common.h"
#include "sftp-client.h"

/* File to read commands from */
FILE* infile;

/* Are we in batchfile mode? */
int batchmode = 0;

/* Size of buffer used when copying files */
size_t copy_buffer_len = 32768;

/* Number of concurrent outstanding requests */
size_t num_requests = 16;

/* PID of ssh transport process */
static pid_t sshpid = -1;

/* This is set to 0 if the progressmeter is not desired. */
int showprogress = 1;

/* SIGINT received during command processing */
volatile sig_atomic_t interrupted = 0;

static void
killchild(int signo)
{
	if (sshpid > 1) {
		kill(sshpid, SIGTERM);
		waitpid(sshpid, NULL, 0);
	}

	_exit(1);
}

static void
connect_to_server(char *path, char **args, int *in, int *out)
{
	int c_in, c_out;

	int inout[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, inout) == -1)
		fatal("socketpair: %s", strerror(errno));
	*in = *out = inout[0];
	c_in = c_out = inout[1];

	if ((sshpid = fork()) == -1)
		fatal("fork: %s", strerror(errno));
	else if (sshpid == 0) {
		if ((dup2(c_in, STDIN_FILENO) == -1) ||
		    (dup2(c_out, STDOUT_FILENO) == -1)) {
			fprintf(stderr, "dup2: %s\n", strerror(errno));
			_exit(1);
		}
		close(*in);
		close(*out);
		close(c_in);
		close(c_out);

		/*
		 * The underlying ssh is in the same process group, so we must
		 * ignore SIGINT if we want to gracefully abort commands,
		 * otherwise the signal will make it to the ssh process and
		 * kill it too
		 */
		signal(SIGINT, SIG_IGN);
		execvp(path, args);
		fprintf(stderr, "exec: %s: %s\n", path, strerror(errno));
		_exit(1);
	}

	signal(SIGTERM, killchild);
	signal(SIGINT, killchild);
	signal(SIGHUP, killchild);
	close(c_in);
	close(c_out);
}

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr,
	    "usage: %s [-1Cv] [-B buffer_size] [-b batchfile] [-F ssh_config]\n"
	    "            [-o ssh_option] [-P sftp_server_path] [-R num_requests]\n"
	    "            [-S program] [-s subsystem | sftp_server] host\n"
	    "       %s [[user@]host[:file [file]]]\n"
	    "       %s [[user@]host[:dir[/]]]\n"
	    "       %s -b batchfile [user@]host\n", __progname, __progname, __progname, __progname);
	exit(1);
}

int sftp_main(int, char *[]);
int
sftp_main(int argc, char **argv)
{
	int ch;
	extern int in, out;
	char *host, *userhost, *cp, *file2 = NULL;
	int debug_level = 0, sshver = 2;
	char *sftp_server = NULL;
	char *ssh_program = _PATH_SSH_PROGRAM, *sftp_direct = NULL;
	LogLevel ll = SYSLOG_LEVEL_INFO;
	arglist args;
	extern int optind;
	extern char *optarg;
	extern char *argpath;

	/* Ensure that fds 0, 1 and 2 are open or directed to /dev/null */
	sanitise_stdfd();

	memset(&args, '\0', sizeof(args));
	args.list = NULL;
	addargs(&args, "%s", ssh_program);
	addargs(&args, "-oForwardX11 no");
	addargs(&args, "-oForwardAgent no");
	addargs(&args, "-oPermitLocalCommand no");
	addargs(&args, "-oClearAllForwardings yes");

	ll = SYSLOG_LEVEL_INFO;
	infile = stdin;

	while ((ch = getopt(argc, argv, "1hvCo:s:S:b:B:F:P:R:")) != -1) {
		switch (ch) {
		case 'C':
			addargs(&args, "-C");
			break;
		case 'v':
			if (debug_level < 3) {
				addargs(&args, "-v");
				ll = SYSLOG_LEVEL_DEBUG1 + debug_level;
			}
			debug_level++;
			break;
		case 'F':
		case 'o':
			addargs(&args, "-%c%s", ch, optarg);
			break;
		case '1':
			sshver = 1;
			if (sftp_server == NULL)
				sftp_server = _PATH_SFTP_SERVER;
			break;
		case 's':
			sftp_server = optarg;
			break;
		case 'S':
			ssh_program = optarg;
			replacearg(&args, 0, "%s", ssh_program);
			break;
		case 'b':
			if (batchmode)
				fatal("Batch file already specified.");

			/* Allow "-" as stdin */
			if (strcmp(optarg, "-") != 0 &&
			    (infile = fopen(optarg, "r")) == NULL)
				fatal("%s (%s).", strerror(errno), optarg);
			showprogress = 0;
			batchmode = 1;
			addargs(&args, "-obatchmode yes");
			break;
		case 'P':
			sftp_direct = optarg;
			break;
		case 'B':
			copy_buffer_len = strtol(optarg, &cp, 10);
			if (copy_buffer_len == 0 || *cp != '\0')
				fatal("Invalid buffer size \"%s\"", optarg);
			break;
		case 'R':
			num_requests = strtol(optarg, &cp, 10);
			if (num_requests == 0 || *cp != '\0')
				fatal("Invalid number of requests \"%s\"",
				    optarg);
			break;
		case 'h':
		default:
			break;
		}
	}

	if (!isatty(STDERR_FILENO))
		showprogress = 0;

	log_init(argv[0], ll, SYSLOG_FACILITY_USER, 1);

	if (sftp_direct == NULL) {
#if 0
		if (optind == argc || argc > (optind + 2))
			usage();
#endif

		userhost = xstrdup(argv[optind]);
		file2 = argv[optind+1];

		if ((host = strrchr(userhost, '@')) == NULL)
			host = userhost;
		else {
			*host++ = '\0';
			if (!userhost[0]) {
				fprintf(stderr, "Missing username\n");
				usage();
			}
			addargs(&args, "-l%s",userhost);
		}

		if ((cp = colon(host)) != NULL) {
			*cp++ = '\0';
			argpath = cp;
		}

		host = cleanhostname(host);
		if (!*host) {
			fprintf(stderr, "Missing hostname\n");
			usage();
		}

		addargs(&args, "-oProtocol %d", sshver);

		/* no subsystem if the server-spec contains a '/' */
		if (sftp_server == NULL || strchr(sftp_server, '/') == NULL)
			addargs(&args, "-s");

		addargs(&args, "%s", host);
		addargs(&args, "%s", (sftp_server != NULL ?
		    sftp_server : "sftp"));

		if (!batchmode)
			fprintf(stderr, "Connecting to %s...\n", host);
		connect_to_server(ssh_program, args.list, &in, &out);
	} else {
		args.list = NULL;
		addargs(&args, "sftp-server");

		if (!batchmode)
			fprintf(stderr, "Attaching to %s...\n", sftp_direct);
		connect_to_server(sftp_direct, args.list, &in, &out);
	}
	freeargs(&args);

	return 0;
}
