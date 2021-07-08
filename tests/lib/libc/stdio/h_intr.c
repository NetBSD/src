/*	$NetBSD: h_intr.c,v 1.1 2021/07/08 09:07:46 christos Exp $	*/

/**
 * Test of interrupted writes to popen()'ed commands.
 *
 * Example 1:
 * ./h_fwrite -c "gzip -t" *.gz
 *
 * Example 2:
 * while :; do ./h_fwrite -b $((12*1024)) -t 10 -c "bzip2 -t" *.bz2; sleep 2; done
 *
 * Example 3:
 * Create checksum file:
 * find /mnt -type f -exec sha512 -n {} + >SHA512
 *
 * Check program:
 * find /mnt -type f -exec ./h_fwrite -b 512 -c run.sh {} +
 * 
 * ./run.sh:
	#!/bin/sh
	set -eu
	grep -q "^$(sha512 -q)" SHA512
 *
 * Author: RVP at sdf.org
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: h_intr.c,v 1.1 2021/07/08 09:07:46 christos Exp $");

#include <time.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int process(const char *fn);
ssize_t maxread(FILE *fp, void *buf, size_t size);
ssize_t smaxread(FILE *fp, void *buf, size_t size);
ssize_t maxwrite(FILE *fp, const void *buf, size_t size);
ssize_t smaxwrite(FILE *fp, const void *buf, size_t size);
static sig_t xsignal(int signo, sig_t handler);
static void alarmtimer(int wait);
static void pr_star(int signo);
static bool isvalid(const char *s);
static int do_opts(int argc, char* argv[]);
static void usage(FILE* fp);

/* Globals */
static struct options {
	size_t bsize;
	size_t ssize;
	int btype;
	int tmout;
	const char *cmd;
} opts;

static const struct {
	const char *name;
	int value;
} btypes[] = {
	{ "IONBF", _IONBF },
	{ "IOLBF", _IOLBF },
	{ "IOFBF", _IOFBF },
};

enum {
	MB = 1024 * 1024,
	BSIZE = 16 * 1024,
	DEF_MS = 100,
	MS = 1000,
};



int
main(int argc, char* argv[])
{
	int i, rc = EXIT_SUCCESS;

	i = do_opts(argc, argv);
	argc -= i;
	argv += i;

	if (argc == 0) {
		usage(stderr);
		return rc;
	}

	xsignal(SIGPIPE, SIG_IGN);
	for (i = 0; i < argc; i++) {
		char *s = strdup(argv[i]);
		printf("%s...", basename(s));
		fflush(stdout);
		free(s);

		sig_t osig = xsignal(SIGALRM, pr_star);

		if (process(argv[i]) == 0)
			printf("ok\n");
		else
			rc = EXIT_FAILURE;

		xsignal(SIGALRM, osig);
	}

	return rc;
}

static int
process(const char *fn)
{
	FILE *ifp, *ofp;
	char *buf;
	size_t nw = 0;
	int rc = EXIT_FAILURE;
	ssize_t n;

	if ((buf = malloc(opts.bsize)) == NULL)
		err(rc, "buffer alloc failed");

	if ((ifp = fopen(fn, "r")) == NULL) {
		warn("fopen failed: %s", fn);
		return rc;
	}

	if ((ofp = popen(opts.cmd, "w")) == NULL)
		err(rc, "popen failed `%s'", opts.cmd);

	setvbuf(ofp, NULL, opts.btype, opts.ssize);
	setvbuf(ifp, NULL, opts.btype, opts.ssize);

	alarmtimer(opts.tmout);
	while ((n = maxread(ifp, buf, opts.bsize)) > 0) {
		ssize_t i;
		if ((i = maxwrite(ofp, buf, n)) == -1) {
			warn("write failed");
			break;
		}
		nw += i;
	}
	alarmtimer(0);
	// printf("%lu\n", nw);

	fclose(ifp);
	if (pclose(ofp) != 0)
		warn("command failed `%s'", opts.cmd);
	else
		rc = EXIT_SUCCESS;

	return rc;
}

/**
 * maxread - syscall version
 */
ssize_t
smaxread(FILE* fp, void* buf, size_t size)
{
	char* p = buf;
	ssize_t nrd = 0;
	ssize_t n;

	while (size > 0) {
		n = read(fileno(fp), p, size);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (n == 0)
			break;
		p += n;
		nrd += n;
		size -= n;
	}
	return nrd;
}

/**
 * maxread - stdio version
 */
ssize_t
maxread(FILE* fp, void* buf, size_t size)
{
	char* p = buf;
	ssize_t nrd = 0;
	size_t n;

	while (size > 0) {
		errno = 0;
		n = fread(p, 1, size, fp);
		if (n == 0) {
			printf("ir.");
			fflush(stdout);
			if (errno == EINTR)
				continue;
			if (feof(fp) || nrd > 0)
				break;
			return -1;
		}
		if (n != size)
			clearerr(fp);
		p += n;
		nrd += n;
		size -= n;
	}
	return nrd;
}

/**
 * maxwrite - syscall version
 */
ssize_t
smaxwrite(FILE* fp, const void* buf, size_t size)
{
	const char* p = buf;
	ssize_t nwr = 0;
	ssize_t n;

	while (size > 0) {
		n = write(fileno(fp), p, size);
		if (n <= 0) {
			if (errno == EINTR)
				n = 0;
			else
				return -1;
		}
		p += n;
		nwr += n;
		size -= n;
	}
	return nwr;
}

/**
 * maxwrite - stdio version (substrate is buggy)
 */
ssize_t
maxwrite(FILE* fp, const void* buf, size_t size)
{
	const char* p = buf;
	ssize_t nwr = 0;
	size_t n;

	while (size > 0) {
		errno = 0;
		n = fwrite(p, 1, size, fp);
		if (n == 0) {
			printf("iw.");
			fflush(stdout);
			if (errno == EINTR)
				continue;
			if (nwr > 0)
				break;
			return -1;
		}
		if (n != size)
			clearerr(fp);
		p += n;
		nwr += n;
		size -= n;
	}
	return nwr;
}

/**
 * wrapper around sigaction() because we want POSIX semantics:
 * no auto-restarting of interrupted slow syscalls.
 */
static sig_t
xsignal(int signo, sig_t handler)
{
	struct sigaction sa, osa;

	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(signo, &sa, &osa) < 0)
		return SIG_ERR;
	return osa.sa_handler;
}

static void
alarmtimer(int wait)
{               
	struct itimerval itv;

	itv.it_value.tv_sec = wait / MS;
	itv.it_value.tv_usec = (wait - itv.it_value.tv_sec * MS) * MS;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL); 
}

/**
 * Print a `*' each time an alarm signal occurs.
 */
static void
pr_star(int signo)
{
	int oe = errno;
	(void)signo;

#if 0
	write(1, "*", 1);
#endif
	errno = oe;
}

/**
 * return true if not empty or blank; FAIL otherwise.
 */
static bool
isvalid(const char *s)
{
	if (*s == '\0')
		return false;
	return strspn(s, " \t") != strlen(s);
}

static const char *
getbtype(int val) {
	for (size_t i = 0; i < __arraycount(btypes); i++)
		if (btypes[i].value == val)
			return btypes[i].name;
	return "*invalid*";
}

/**
 * Print usage information.
 */
static void
usage(FILE* fp)
{
	fprintf(fp, "Usage: %s [-b SIZE] [-h] [-t TMOUT] -c CMD FILE...\n",
	    getprogname());
	fprintf(fp, "%s: Test interrupted writes to popen()ed CMD.\n",
	    getprogname());
	fprintf(fp, "\n");
	fprintf(fp, "  -b SIZE   Buffer size (%lu)\n", opts.bsize);
	fprintf(fp, "  -c CMD    Command to run on each FILE.\n");
	fprintf(fp, "  -h        This message.\n");
	fprintf(fp, "  -p        Buffering type %s.\n", getbtype(opts.btype));
	fprintf(fp, "  -s SIZE   stdio buffer size (%lu)\n", opts.ssize);
	fprintf(fp, "  -t TMOUT  Interrupt writing to CMD every (%d) ms\n",
	    opts.tmout);
}

/**
 * Process program options.
 */
static int
do_opts(int argc, char *argv[])
{
	int opt;
	int i;
	size_t j;

	/* defaults */
	opts.btype = _IONBF;
	opts.ssize = BSIZE;		/* 16K */
	opts.bsize = BSIZE;		/* 16K */
	opts.tmout = DEF_MS;		/* 100ms */
	opts.cmd = "";

	while ((opt = getopt(argc, argv, "b:c:hp:s:t:")) != -1) {
		switch (opt) {
		case 'b':
			i = atoi(optarg);
			if (i <= 0 || i > MB)
				errx(EXIT_FAILURE,
				    "buffer size not in range (1 - %d): %d",
				    MB, i);
			opts.bsize = i;
			break;
		case 'c':
			opts.cmd = optarg;
			break;
		case 'h':
			usage(stdout);
			exit(EXIT_SUCCESS);
		case 'p':
			for (j = 0; j < __arraycount(btypes); j++)
				if (strcmp(btypes[j].name, optarg) == 0) {
					opts.btype = btypes[j].value;
					break;
				}
			if (j == __arraycount(btypes))
				errx(EXIT_FAILURE,
				    "unknown buffering type: `%s'", optarg);
			break;
		case 's':
			i = atoi(optarg);
			if (i <= 0 || i > MB)
				errx(EXIT_FAILURE,
				    "buffer size not in range (1 - %d): %d",
				    MB, i);
			opts.ssize = i;
			break;
		case 't':
			i = atoi(optarg);
			if ((i < 10 || i > 10000) && i != 0)
				errx(EXIT_FAILURE,
				    "timeout not in range (10ms - 10s): %d", i);
			opts.tmout = i;
			break;
		default:
			usage(stderr);
			exit(EXIT_FAILURE);
		}
	}

	if (!isvalid(opts.cmd))
		errx(EXIT_FAILURE, "Please specify a valid command with -c");

	return optind;
}
