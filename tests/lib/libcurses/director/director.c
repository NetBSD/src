/*	$NetBSD: director.c,v 1.3 2011/04/19 20:13:55 martin Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
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
 *    derived from this software withough specific prior written permission
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
 *
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "returns.h"

#define DEF_TERMPATH "."
#define DEF_TERM "atf"
#define DEF_SLAVE "./slave"

char *def_check_path = "./"; /* default check path */
char *def_include_path = "./"; /* default include path */

extern size_t nvars;	/* In testlang_conf.y */
saved_data_t  saved_output;	/* In testlang_conf.y */
int cmdpipe[2];		/* command pipe between director and slave */
int slvpipe[2];		/* reply pipe back from slave */
int master;		/* pty to the slave */
int verbose;		/* control verbosity of tests */
char *check_path;	/* path to prepend to check files for output
			   validation */
char *include_path;	/* path to prepend to include files */
char *cur_file;		/* name of file currently being read */

void init_parse_variables(int); /* in testlang_parse.y */

/*
 * Handle the slave exiting unexpectedly, try to recover the exit message
 * and print it out.
 */
void
slave_died(int param)
{
	char last_words[256];
	int count;

	fprintf(stderr, "ERROR: Slave has exited\n");
	if (saved_output.count > 0) {
		fprintf(stderr, "output from slave: ");
		for (count = 0; count < saved_output.count; count ++) {
			if (isprint(saved_output.data[count]))
			    fprintf(stderr, "%c", saved_output.data[count]);
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
usage(char *name)
{
	fprintf(stderr, "Curses automated test director\n");
	fprintf(stderr, "%s [-v] [-p termcappath] [-s pathtoslave] [-t term]"
		" commandfile\n", name);
	fprintf(stderr, " where:\n");
	fprintf(stderr, "    -v enables verbose test output\n");
	fprintf(stderr, "    termcappath is the path to the directory"
		"holding the termpcap file\n");
	fprintf(stderr, "    pathtoslave is the path to the slave exectuable\n");
	fprintf(stderr, "    term is value to set TERM to for the test\n");
	fprintf(stderr, "    commandfile is a file of test directives\n");
	exit(2);
}


int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	char *termpath, *term, *slave;
	int ch;
	pid_t slave_pid;
	extern FILE *yyin;
	char *arg1, *arg2, *arg3, *arg4;
	struct termios term_attr;
	int slavefd, on;

	termpath = term = slave = NULL;
	verbose = 0;

	while ((ch = getopt(argc, argv, "vp:s:t:")) != -1) {
		switch(ch) {
		case 'p':
			asprintf(&termpath, "%s", optarg);
			break;
		case 's':
			asprintf(&slave, "%s", optarg);
			break;
		case 't':
			asprintf(&term, "%s", optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage(argv[0]);
			break;
		}
	}

	if (termpath == NULL)
		asprintf(&termpath, "%s", DEF_TERMPATH);

	if (slave == NULL)
		asprintf(&slave, "%s", DEF_SLAVE);

	if (term == NULL)
		asprintf(&term, "%s", DEF_TERM);

	argc -= optind;
	if (argc < 1)
		usage(argv[0]);

	signal(SIGCHLD, slave_died);

	argv += optind;

	if (setenv("TERM", term, 1) != 0)
		err(2, "Failed to set TERM variable");

	check_path = getenv("CHECK_PATH");
	if ((check_path == NULL) || (check_path[0] == '\0')) {
		fprintf(stderr,
			"WARNING: CHECK_PATH not set, defaulting to %s\n",
			def_check_path);
		check_path = def_check_path;
	}

	include_path = getenv("INCLUDE_PATH");
	if ((include_path == NULL) || (include_path[0] == '\0')) {
		fprintf(stderr,
			"WARNING: INCLUDE_PATH not set, defaulting to %s\n",
			def_include_path);
		include_path = def_include_path;
	}

	if (pipe(cmdpipe) < 0) {
		fprintf(stderr, "Command pipe creation failed: ");
		perror(NULL);
		exit(2);
	}

	if (pipe(slvpipe) < 0) {
		fprintf(stderr, "Slave pipe creation failed: ");
		perror(NULL);
		exit(2);
	}

	/*
	 * Create default termios settings for later use
	 */
	memset(&term_attr, 0, sizeof(term_attr));
	term_attr.c_iflag = TTYDEF_IFLAG;
	term_attr.c_oflag = TTYDEF_OFLAG;
	term_attr.c_cflag = TTYDEF_CFLAG;
	term_attr.c_lflag = TTYDEF_LFLAG;
	cfsetspeed(&term_attr, TTYDEF_SPEED);

	if ((slave_pid = forkpty(&master, NULL, &term_attr, NULL)) < 0) {
		fprintf(stderr, "Fork of pty for slave failed\n");
		exit(2);
	}

	if (slave_pid == 0) {
		/* slave side, just exec the slave process */
		if (asprintf(&arg1, "%d", cmdpipe[0]) < 0)
			err(1, "arg1 conversion failed");

		if (asprintf(&arg2, "%d", cmdpipe[1]) < 0)
			err(1, "arg2 conversion failed");

		if (asprintf(&arg3, "%d", slvpipe[0]) < 0)
			err(1, "arg3 conversion failed");

		if (asprintf(&arg4, "%d", slvpipe[1]) < 0)
			err(1, "arg4 conversion failed");

		if (execl(slave, slave, arg1, arg2, arg3, arg4, NULL) < 0) {
			fprintf(stderr, "Exec of slave %s failed: ", slave);
			perror(NULL);
			exit(2);
		}

		/* NOT REACHED */
	}

	fcntl(master, F_SETFL, O_NONBLOCK);

	if ((yyin = fopen(argv[0], "r")) == NULL) {
		fprintf(stderr, "Cannot open command file %s: ", argv[0]);
		perror(NULL);
		exit(2);
	}

	if ((cur_file = malloc(strlen(argv[0]) + 1)) == NULL)
		err(2, "Failed to alloc memory for test file name");

	strlcpy(cur_file, argv[0], strlen(argv[0]) + 1);

	init_parse_variables(1);

	yyparse();
	fclose(yyin);

	exit(0);
}
