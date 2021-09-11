/*	$NetBSD: h_intr.c,v 1.6 2021/09/11 18:18:28 rillig Exp $	*/

/**
 * Test of interrupted I/O to popen()ed commands.
 *
 * Example 1:
 * ./h_intr -c "gzip -t" *.gz
 *
 * Example 2:
 * while :; do ./h_intr -b $((12*1024)) -t 10 -c "bzip2 -t" *.bz2; sleep 2; done
 *
 * Example 3:
 * Create checksum file:
 * find /mnt -type f -exec sha512 -n {} + >SHA512
 *
 * Check program:
 * find /mnt -type f -exec ./h_intr -b 512 -c run.sh {} +
 *
 * ./run.sh:
	#!/bin/sh
	set -eu
	grep -q "^$(sha512 -q)" SHA512
 *
 * Author: RVP at sdf.org
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: h_intr.c,v 1.6 2021/09/11 18:18:28 rillig Exp $");

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

static bool process(const char *fn);
ssize_t maxread(FILE *fp, void *buf, size_t size);
ssize_t smaxread(FILE *fp, void *buf, size_t size);
ssize_t maxwrite(FILE *fp, const void *buf, size_t size);
ssize_t smaxwrite(FILE *fp, const void *buf, size_t size);
static int rndbuf(void);
static int rndmode(void);
static sig_t xsignal(int signo, sig_t handler);
static void alarmtimer(int wait);
static void pr_star(int signo);
static int do_opts(int argc, char *argv[]);
static void usage(FILE *fp);

/* Globals */
static struct options {
	const char *cmd;	/* cmd to run (which must read from stdin) */
	size_t bsize;		/* block size to use */
	size_t asize;		/* alt. stdio buffer size */
	int btype;		/* buffering type: _IONBF, ... */
	int tmout;		/* alarm timeout */
	int flush;		/* call fflush() after write if 1 */
	int rndbuf;		/* switch buffer randomly if 1 */
	int rndmod;		/* switch buffering modes randomly if 1 */
} opts;

static const struct {
	const char *name;
	int value;
} btypes[] = {
	{ "IONBF", _IONBF },
	{ "IOLBF", _IOLBF },
	{ "IOFBF", _IOFBF },
};

static void (*alarm_fn)(int);				/* real/dummy alarm fn. */
static int (*sintr_fn)(int, int);			/*  " siginterrupt fn. */
static ssize_t (*rd_fn)(FILE *, void *, size_t);	/* read fn. */
static ssize_t (*wr_fn)(FILE *, const void *, size_t);	/* write fn. */

enum {
	MB = 1024 * 1024,	/* a megabyte */
	BSIZE = 16 * 1024,	/* default RW buffer size */
	DEF_MS = 100,		/* interrupt 10x a second */
	MS = 1000,		/* msecs. in a second */
};




/**
 * M A I N
 */
int
main(int argc, char *argv[])
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

		if (process(argv[i]) == true)
			printf(" OK\n");
		else
			rc = EXIT_FAILURE;

		xsignal(SIGALRM, osig);
	}

	return rc;
}

static bool
process(const char *fn)
{
	FILE *ifp, *ofp;
	char *buf, *abuf;
	int rc = false;
	size_t nw = 0;
	ssize_t n;

	abuf = NULL;

	if ((buf = malloc(opts.bsize)) == NULL) {
		warn("buffer alloc failed");
		return rc;
	}

	if ((abuf = malloc(opts.asize)) == NULL) {
		warn("alt. buffer alloc failed");
		goto fail;
	}

	if ((ifp = fopen(fn, "r")) == NULL) {
		warn("fopen failed: %s", fn);
		goto fail;
	}

	if ((ofp = popen(opts.cmd, "w")) == NULL) {
		warn("popen failed `%s'", opts.cmd);
		goto fail;
	}

	setvbuf(ofp, NULL, opts.btype, opts.asize);
	setvbuf(ifp, NULL, opts.btype, opts.asize);

	alarm_fn(opts.tmout);

	while ((n = rd_fn(ifp, buf, opts.bsize)) > 0) {
		ssize_t i;

		if (opts.rndbuf || opts.rndmod) {
			int r = rndbuf();
			setvbuf(ofp, r ? abuf : NULL,
				rndmode(), r ? opts.asize : 0);
		}

		sintr_fn(SIGALRM, 0);

		if ((i = wr_fn(ofp, buf, n)) == -1) {
			sintr_fn(SIGALRM, 1);
			warn("write failed");
			break;
		}

		if (opts.flush)
			if (fflush(ofp))
				warn("fflush failed");

		sintr_fn(SIGALRM, 1);
		nw += i;
	}

	alarm_fn(0);
	// printf("%zu\n", nw);

	fclose(ifp);
	if (pclose(ofp) != 0)
		warn("command failed `%s'", opts.cmd);
	else
		rc = true;

fail:
	free(abuf);
	free(buf);

	return rc;
}

/**
 * maxread - syscall version
 */
ssize_t
smaxread(FILE* fp, void *buf, size_t size)
{
	char *p = buf;
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
maxread(FILE* fp, void *buf, size_t size)
{
	char *p = buf;
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
smaxwrite(FILE* fp, const void *buf, size_t size)
{
	const char *p = buf;
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
 * maxwrite - stdio version (warning: substrate may be buggy)
 */
ssize_t
maxwrite(FILE* fp, const void *buf, size_t size)
{
	const char *p = buf;
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

static int
rndbuf(void)
{
	if (opts.rndbuf == 0)
		return 0;
	return arc4random_uniform(2);
}

static int
rndmode(void)
{
	if (opts.rndmod == 0)
		return opts.btype;

	switch (arc4random_uniform(3)) {
	case 0:	return _IONBF;
	case 1: return _IOLBF;
	case 2: return _IOFBF;
	default: errx(EXIT_FAILURE, "programmer error!");
	}
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

static void
dummytimer(int dummy)
{
	(void)dummy;
}

static int
dummysintr(int dum1, int dum2)
{
	(void)dum1;
	(void)dum2;
	return 0;	/* OK */
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
 * return true if not empty or blank; false otherwise.
 */
static bool
isvalid(const char *s)
{
	return strspn(s, " \t") != strlen(s);
}

static const char *
btype2str(int val)
{
	for (size_t i = 0; i < __arraycount(btypes); i++)
		if (btypes[i].value == val)
			return btypes[i].name;
	return "*invalid*";
}

static int
str2btype(const char *s)
{
	for (size_t i = 0; i < __arraycount(btypes); i++)
		if (strcmp(btypes[i].name, s) == 0)
			return btypes[i].value;
	return EOF;
}

/**
 * Print usage information.
 */
static void
usage(FILE* fp)
{
	fprintf(fp, "Usage: %s [-a SIZE] [-b SIZE] [-fihmnrsw]"
		    " [-p TYPE] [-t TMOUT] -c CMD FILE...\n",
		getprogname());
	fprintf(fp, "%s: Test interrupted writes to popen()ed CMD.\n",
		getprogname());
	fprintf(fp, "\n");
	fprintf(fp, "Usual options:\n");
	fprintf(fp, "  -a SIZE   Alt. stdio buffer size (%zu)\n", opts.asize);
	fprintf(fp, "  -b SIZE   Program buffer size (%zu)\n", opts.bsize);
	fprintf(fp, "  -c CMD    Command to run on each FILE\n");
	fprintf(fp, "  -h        This message\n");
	fprintf(fp, "  -p TYPE   Buffering type (%s)\n", btype2str(opts.btype));
	fprintf(fp, "  -t TMOUT  Interrupt writing to CMD every (%d) ms\n",
		opts.tmout);
	fprintf(fp, "Debug options:\n");
	fprintf(fp, "  -f        Do fflush() after writing each block\n");
	fprintf(fp, "  -i        Use siginterrupt to block interrupts\n");
	fprintf(fp, "  -m        Use random buffering modes\n");
	fprintf(fp, "  -n        No interruptions (turns off -i)\n");
	fprintf(fp, "  -r        Use read() instead of fread()\n");
	fprintf(fp, "  -s        Switch between own/stdio buffers at random\n");
	fprintf(fp, "  -w        Use write() instead of fwrite()\n");
}

/**
 * Process program options.
 */
static int
do_opts(int argc, char *argv[])
{
	int opt, i;

	/* defaults */
	opts.cmd = "";
	opts.btype = _IONBF;
	opts.asize = BSIZE;		/* 16K */
	opts.bsize = BSIZE;		/* 16K */
	opts.tmout = DEF_MS;		/* 100ms */
	opts.flush = 0;			/* no fflush() after each write */
	opts.rndbuf = 0;		/* no random buffer switching */
	opts.rndmod = 0;		/* no random mode    " */
	alarm_fn = alarmtimer;
	sintr_fn = dummysintr;		/* don't protect writes with siginterrupt() */
	rd_fn = maxread;		/* read using stdio funcs. */
	wr_fn = maxwrite;		/* write   "   */

	while ((opt = getopt(argc, argv, "a:b:c:fhimnp:rst:w")) != -1) {
		switch (opt) {
		case 'a':
			i = atoi(optarg);
			if (i <= 0 || i > MB)
				errx(EXIT_FAILURE,
				     "alt. buffer size not in range (1 - %d): %d",
				     MB, i);
			opts.asize = i;
			break;
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
		case 'f':
			opts.flush = 1;
			break;
		case 'i':
			sintr_fn = siginterrupt;
			break;
		case 'm':
			opts.rndmod = 1;
			break;
		case 'n':
			alarm_fn = dummytimer;
			break;
		case 'p':
			i = str2btype(optarg);
			if (i == EOF)
				errx(EXIT_FAILURE,
				     "unknown buffering type: `%s'", optarg);
			opts.btype = i;
			break;
		case 'r':
			rd_fn = smaxread;
			break;
		case 'w':
			wr_fn = smaxwrite;
			break;
		case 's':
			opts.rndbuf = 1;
			break;
		case 't':
			i = atoi(optarg);
			if ((i < 10 || i > 10000) && i != 0)
				errx(EXIT_FAILURE,
				    "timeout not in range (10ms - 10s): %d", i);
			opts.tmout = i;
			break;
		case 'h':
			usage(stdout);
			exit(EXIT_SUCCESS);
		default:
			usage(stderr);
			exit(EXIT_FAILURE);
		}
	}

	if (!isvalid(opts.cmd))
		errx(EXIT_FAILURE, "Please specify a valid command with -c");

	/* don't call siginterrupt() if not interrupting */
	if (alarm_fn == dummytimer)
		sintr_fn = dummysintr;

	return optind;
}
