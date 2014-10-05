/*	$NetBSD: crash.c,v 1.7 2014/10/05 23:08:01 wiz Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: crash.c,v 1.7 2014/10/05 23:08:01 wiz Exp $");
#endif /* not lint */

#include <ddb/ddb.h>

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <machine/frame.h>

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <histedit.h>
#include <paths.h>
#include <kvm.h>
#include <err.h>
#include <ctype.h>
#include <util.h>

#include "extern.h"

#define	MAXSTAB	(16 * 1024 * 1024)

db_regs_t	ddb_regs;

static kvm_t		*kd;
static History		*hist;
static HistEvent	he;
static EditLine		*elptr;
static char		imgrelease[16];
static FILE		*ofp;

static struct nlist nl[] = {
#define	X_OSRELEASE	0
	{ .n_name = "_osrelease" },
#define	X_PANICSTR	1
	{ .n_name = "_panicstr" },
	{ .n_name = NULL },
};

static void
cleanup(void)
{
	if (ofp != stdout) {
		(void)fflush(ofp);
		(void)pclose(ofp);
		ofp = stdout;
	}
	el_end(elptr);
	history_end(hist);
}

void
db_vprintf(const char *fmt, va_list ap)
{
	char buf[1024];
	int b, c;

	c = vsnprintf(buf, sizeof(buf), fmt, ap);
	for (b = 0; b < c; b++) {
		db_putchar(buf[b]);
	} 
}

void
db_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	db_vprintf(fmt, ap);
	va_end(ap);
}

void
db_write_bytes(db_addr_t addr, size_t size, const char *str)
{

	if ((size_t)kvm_write(kd, addr, str, size) != size) {
		warnx("kvm_write(%p, %zd): %s", (void *)addr, size,
		    kvm_geterr(kd));
		longjmp(db_recover);
	}
}

void
db_read_bytes(db_addr_t addr, size_t size, char *str)
{

	if ((size_t)kvm_read(kd, addr, str, size) != size) {
		warnx("kvm_read(%p, %zd): %s", (void *)addr, size,
		    kvm_geterr(kd));
		longjmp(db_recover);
	}
}

void *
db_alloc(size_t sz)
{

	return emalloc(sz);
}

void *
db_zalloc(size_t sz)
{

	return ecalloc(1, sz);
}

void
db_free(void *p, size_t sz)
{

	free(p);
}

static void
punt(void)
{

	db_printf("This command can only be used in-kernel.\n");
}

void
db_breakpoint_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}

void
db_continue_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	db_cmd_loop_done = true;
}

void
db_delete_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}

void
db_deletewatch_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}


void
db_trace_until_matching_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}


void
db_single_step_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}


void
db_trace_until_call_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}


void
db_watchpoint_cmd(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{

	punt();
}

int
db_readline(char *lstart, int lsize)
{
	const char *el;
	char *pcmd;
	int cnt;

	db_force_whitespace();

	/* Close any open pipe. */
	if (ofp != stdout) {
		(void)fflush(ofp);
		(void)pclose(ofp);
		ofp = stdout;
	}

	/* Read next command. */
	el = el_gets(elptr, &cnt);
	if (el == NULL) {	/* EOF */
		exit(EXIT_SUCCESS);
	}

	/* Save to history, and copy to caller's buffer. */
	history(hist, &he, H_ENTER, el);
	strlcpy(lstart, el, lsize);
	if (cnt >= lsize) {
		cnt = lsize - 1;
	}
	lstart[cnt] = '\0';
	if (cnt > 0 && lstart[cnt - 1] == '\n') {
		lstart[cnt - 1] = '\0';
	}

	/* Need to open a pipe? If not, return now. */
	pcmd = strchr(lstart, '|');
	if (pcmd == NULL) {
		return strlen(lstart);
	}

	/* Open a pipe to specified command, redirect output. */
	assert(ofp == stdout);
	for (*pcmd++ = '\0'; isspace((unsigned char)*pcmd); pcmd++) {
		/* nothing */
	}
	errno = 0;
	ofp = popen(pcmd, "w");
	if (ofp == NULL) {
		warn("opening pipe for command `%s'", pcmd);
		*lstart = '\0';
	}
	return strlen(lstart);
}

void
db_check_interrupt(void)
{

}

int
cngetc(void)
{

	fprintf(stderr, "cngetc\n");
	abort();
}

void
cnputc(int c)
{

	putc(c, ofp);
}

__dead static void
usage(void)
{

	fprintf(stderr,
	    "usage: %s [-w] [-M core] [-N kernel]\n\n"
	    "-M core\tspecify memory file\n"
	    "-N kernel\tspecify name list file (default /dev/ksyms)\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static const char *
prompt(void)
{

	return "crash> ";
}

int
main(int argc, char **argv)
{
	const char *nlistf, *memf;
	uintptr_t panicstr;
	struct winsize ws;
	struct stat sb;
	size_t sz;
	void *elf;
	int fd, ch, flags;
	char c;

	nlistf = _PATH_KSYMS;
	memf = _PATH_MEM;
	ofp = stdout;
	flags = O_RDONLY;

	setprogname(argv[0]);

	/*
	 * Parse options.
	 */
	while ((ch = getopt(argc, argv, "M:N:w")) != -1) {
		switch (ch) {
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'w':
			flags = O_RDWR;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * Print a list of images, and allow user to select.
	 */
	/* XXX */

	/*
	 * Open the images (crash dump and symbol table).
	 */
	kd = kvm_open(nlistf, memf, NULL, flags, getprogname());
	if (kd == NULL) {
		return EXIT_FAILURE;
	}
	fd = open(nlistf, O_RDONLY);
	if (fd == -1)  {
		err(EXIT_FAILURE, "open `%s'", nlistf);
	}
	if (fstat(fd, &sb) == -1) {
		err(EXIT_FAILURE, "stat `%s'", nlistf);
	}
	if ((sb.st_mode & S_IFMT) != S_IFREG) {	/* XXX ksyms */
		sz = MAXSTAB;
		elf = malloc(sz);
		if (elf == NULL) {
			err(EXIT_FAILURE, "malloc(%zu)", sz);
		}
		sz = read(fd, elf, sz);
		if ((ssize_t)sz == -1) {
			err(EXIT_FAILURE, "read `%s'", nlistf);
		}
		if (sz == MAXSTAB) {
			errx(EXIT_FAILURE, "symbol table > %d bytes", MAXSTAB);
		}
	} else {
		sz = sb.st_size;
		elf = mmap(NULL, sz, PROT_READ, MAP_PRIVATE|MAP_FILE, fd, 0);
		if (elf == MAP_FAILED) {
			err(EXIT_FAILURE, "mmap `%s'", nlistf);
		}
	}

	/*
	 * Print kernel & crash versions.
	 */
	if (kvm_nlist(kd, nl) == -1) {
		errx(EXIT_FAILURE, "kvm_nlist: %s", kvm_geterr(kd));
	}
	if ((size_t)kvm_read(kd, nl[X_OSRELEASE].n_value, imgrelease,
	    sizeof(imgrelease)) != sizeof(imgrelease)) {
		errx(EXIT_FAILURE, "cannot read osrelease: %s",
		    kvm_geterr(kd));
	}
	printf("Crash version %s, image version %s.\n", osrelease, imgrelease);
	if (strcmp(osrelease, imgrelease) != 0) {
		printf("WARNING: versions differ, you may not be able to "
		    "examine this image.\n");
	}

	/*
	 * Print the panic string, if any.
	 */
	if ((size_t)kvm_read(kd, nl[X_PANICSTR].n_value, &panicstr,
	    sizeof(panicstr)) != sizeof(panicstr)) {
		errx(EXIT_FAILURE, "cannot read panicstr: %s",
		    kvm_geterr(kd));
	}
	if (strcmp(memf, _PATH_MEM) == 0) {
		printf("Output from a running system is unreliable.\n");
	} else if (panicstr == 0) {
		printf("System does not appear to have panicked.\n");
	} else {
		printf("System panicked: ");
		for (;;) {
			if ((size_t)kvm_read(kd, panicstr, &c, sizeof(c)) !=
			    sizeof(c)) {
				errx(EXIT_FAILURE, "cannot read *panicstr: %s",
				    kvm_geterr(kd));
			}
			if (c == '\0') {
				break;
			}
			putchar(c);
			panicstr++;
		}
		putchar('\n');
	}

	/*
	 * Initialize line editing.
	 */
	hist = history_init();
	history(hist, &he, H_SETSIZE, 100);
	elptr = el_init(getprogname(), stdin, stdout, stderr);
	el_set(elptr, EL_EDITOR, "emacs");
	el_set(elptr, EL_SIGNAL, 1);
	el_set(elptr, EL_HIST, history, hist);
	el_set(elptr, EL_PROMPT, prompt);
	el_source(elptr, NULL);

	atexit(cleanup);

	/*
	 * Initialize ddb.
	 */
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
		db_max_width = ws.ws_col;
	}
	db_mach_init(kd);
	ddb_init(sz, elf, (char *)elf + sz);

	/*
	 * Debug it!
	 */
	db_command_loop();

	/*
	 * Finish.
	 */
	return EXIT_SUCCESS;
}
