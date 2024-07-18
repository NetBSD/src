/*	$NetBSD: director.c,v 1.30 2024/07/18 22:10:51 blymn Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <err.h>
#include "returns.h"
#include "director.h"

void yyparse(void);
#define DEF_TERMPATH "."
#define DEF_TERM "atf"
#define DEF_SLAVE "./slave"

const char *def_check_path = "./"; /* default check path */

extern size_t nvars;		/* In testlang_conf.y */
saved_data_t  saved_output;	/* In testlang_conf.y */
int to_slave;
int from_slave;
int master;			/* pty to the slave */
int nofail;			/* don't exit on check file fail */
int verbose;			/* control verbosity of tests */
int check_file_flag;		/* control check-file generation */
const char *check_path;		/* path to prepend to check files for output
				   validation */
char *cur_file;			/* name of file currently being read */

void init_parse_variables(int);	/* in testlang_parse.y */

/*
 * Handle the slave exiting unexpectedly, try to recover the exit message
 * and print it out.
 *
 * FIXME: Must not use stdio in a signal handler.  This leads to incomplete
 * output in verbose mode, truncating the useful part of the error message.
 */
static void
slave_died(int signo)
{
	char last_words[256];
	size_t count;

	fprintf(stderr, "ERROR: Slave has exited\n");
	if (saved_output.count > 0) {
		fprintf(stderr, "output from slave: ");
		for (count = 0; count < saved_output.count; count ++) {
			unsigned char b = saved_output.data[count];
			if (isprint(b))
				fprintf(stderr, "%c", b);
			else
				fprintf(stderr, "\\x%02x", b);
		}
		fprintf(stderr, "\n");
	}

	if ((count = read(master, &last_words, 255)) > 0) {
		last_words[count] = '\0';
		fprintf(stderr, "slave exited with message \"%s\"\n",
			last_words);
	}

	exit(2);
}


static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-vgf] [-I include-path] [-C check-path] "
	    "[-T terminfo-file] [-s pathtoslave] [-t term] "
	    "commandfile\n", getprogname());
	fprintf(stderr, " where:\n");
	fprintf(stderr, "    -v enables verbose test output\n");
	fprintf(stderr, "    -g generates check-files if they do not exist\n");
	fprintf(stderr, "    -f overwrites check-files with the actual data\n");
	fprintf(stderr, "    -T is a directory containing the terminfo.cdb "
	    "file, or a file holding the terminfo description\n");
	fprintf(stderr, "    -s is the path to the slave executable\n");
	fprintf(stderr, "    -t is value to set TERM to for the test\n");
	fprintf(stderr, "    -C is the directory for check-files\n");
	fprintf(stderr, "    commandfile is a file of test directives\n");
	exit(1);
}


int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	const char *termpath, *term, *slave;
	int ch;
	pid_t slave_pid;
	extern FILE *yyin;
	char *arg1, *arg2;
	struct termios term_attr;
	struct stat st;
	int pipe_to_slave[2], pipe_from_slave[2];

	termpath = term = slave = NULL;
	nofail = 0;
	verbose = 0;
	check_file_flag = 0;

	while ((ch = getopt(argc, argv, "nvgfC:s:t:T:")) != -1) {
		switch (ch) {
		case 'C':
			check_path = optarg;
			break;
		case 'T':
			termpath = optarg;
			break;
		case 'n':
			nofail = 1;
			break;
		case 's':
			slave = optarg;
			break;
		case 't':
			term = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'g':
			check_file_flag |= GEN_CHECK_FILE;
			break;
		case 'f':
			check_file_flag |= FORCE_GEN;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}

	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();

	if (termpath == NULL)
		termpath = DEF_TERMPATH;

	if (slave == NULL)
		slave = DEF_SLAVE;

	if (term == NULL)
		term = DEF_TERM;

	if (check_path == NULL)
		check_path = getenv("CHECK_PATH");
	if ((check_path == NULL) || (check_path[0] == '\0')) {
		warnx("$CHECK_PATH not set, defaulting to %s", def_check_path);
		check_path = def_check_path;
	}

	signal(SIGCHLD, slave_died);

	if (setenv("TERM", term, 1) != 0)
		err(2, "Failed to set TERM variable");

	if (unsetenv("ESCDELAY") != 0)
		err(2, "Failed to unset ESCDELAY variable");

	if (stat(termpath, &st) == -1)
		err(1, "Cannot stat %s", termpath);

	if (S_ISDIR(st.st_mode)) {
		char tinfo[MAXPATHLEN];
		int l = snprintf(tinfo, sizeof(tinfo), "%s/%s", termpath,
		    "terminfo.cdb");
		if (stat(tinfo, &st) == -1)
			err(1, "Cannot stat `%s'", tinfo);
		if (l >= 4)
			tinfo[l - 4] = '\0';
		if (setenv("TERMINFO", tinfo, 1) != 0)
			err(1, "Failed to set TERMINFO variable");
	} else {
		int fd;
		char *tinfo;
		if ((fd = open(termpath, O_RDONLY)) == -1)
			err(1, "Cannot open `%s'", termpath);
		if ((tinfo = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_FILE,
			fd, 0)) == MAP_FAILED)
			err(1, "Cannot map `%s'", termpath);
		if (setenv("TERMINFO", tinfo, 1) != 0)
			err(1, "Failed to set TERMINFO variable");
		close(fd);
		munmap(tinfo, (size_t)st.st_size);
	}

	if (pipe(pipe_to_slave) < 0)
		err(1, "Command pipe creation failed");
	to_slave = pipe_to_slave[1];

	if (pipe(pipe_from_slave) < 0)
		err(1, "Slave pipe creation failed");
	from_slave = pipe_from_slave[0];

	/*
	 * Create default termios settings for later use
	 */
	memset(&term_attr, 0, sizeof(term_attr));
	term_attr.c_iflag = TTYDEF_IFLAG;
	term_attr.c_oflag = TTYDEF_OFLAG;
	term_attr.c_cflag = TTYDEF_CFLAG;
	term_attr.c_lflag = TTYDEF_LFLAG;
	cfsetspeed(&term_attr, TTYDEF_SPEED);
	term_attr.c_cc[VERASE] = '\b';
	term_attr.c_cc[VKILL] = '\025'; /* ^U */

	if ((slave_pid = forkpty(&master, NULL, &term_attr, NULL)) < 0)
		err(1, "Fork of pty for slave failed\n");

	if (slave_pid == 0) {
		/* slave side, just exec the slave process */
		if (asprintf(&arg1, "%d", pipe_to_slave[0]) < 0)
			err(1, "arg1 conversion failed");
		close(pipe_to_slave[1]);

		close(pipe_from_slave[0]);
		if (asprintf(&arg2, "%d", pipe_from_slave[1]) < 0)
			err(1, "arg2 conversion failed");

		if (execl(slave, slave, arg1, arg2, (char *)0) < 0)
			err(1, "Exec of slave %s failed", slave);

		/* NOT REACHED */
	}

	(void)close(pipe_to_slave[0]);
	(void)close(pipe_from_slave[1]);

	fcntl(master, F_SETFL, O_NONBLOCK);

	if ((yyin = fopen(argv[0], "r")) == NULL)
		err(1, "Cannot open command file %s", argv[0]);

	if ((cur_file = strdup(argv[0])) == NULL)
		err(2, "Failed to alloc memory for test file name");

	init_parse_variables(1);

	yyparse();
	fclose(yyin);

	signal(SIGCHLD, SIG_DFL);
	(void)close(to_slave);
	(void)close(from_slave);

	int status;
	(void)waitpid(slave_pid, &status, 0);

	exit(0);
}
