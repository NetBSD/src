/*	$NetBSD: audiotest.c,v 1.20 2022/05/17 05:05:20 andvar Exp $	*/

/*
 * Copyright (C) 2019 Tetsuya Isaki. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: audiotest.c,v 1.20 2022/05/17 05:05:20 andvar Exp $");

#include <errno.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS	/* for PRIx64 */
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <sys/audioio.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#if !defined(NO_RUMP)
#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#endif

#if !defined(AUDIO_ENCODING_SLINEAR_NE)
#if BYTE_ORDER == LITTLE_ENDIAN
#define AUDIO_ENCODING_SLINEAR_NE AUDIO_ENCODING_SLINEAR_LE
#define AUDIO_ENCODING_ULINEAR_NE AUDIO_ENCODING_ULINEAR_LE
#define AUDIO_ENCODING_SLINEAR_OE AUDIO_ENCODING_SLINEAR_BE
#define AUDIO_ENCODING_ULINEAR_OE AUDIO_ENCODING_ULINEAR_BE
#else
#define AUDIO_ENCODING_SLINEAR_NE AUDIO_ENCODING_SLINEAR_BE
#define AUDIO_ENCODING_ULINEAR_NE AUDIO_ENCODING_ULINEAR_BE
#define AUDIO_ENCODING_SLINEAR_OE AUDIO_ENCODING_SLINEAR_LE
#define AUDIO_ENCODING_ULINEAR_OE AUDIO_ENCODING_ULINEAR_LE
#endif
#endif

struct testentry {
	const char *name;
	void (*func)(void);
};

void usage(void) __dead;
void xp_err(int, int, const char *, ...) __printflike(3, 4) __dead;
void xp_errx(int, int, const char *, ...) __printflike(3, 4) __dead;
bool match(const char *, const char *);
void xxx_close_wait(void);
int mixer_get_outputs_master(int);
void do_test(int);
int rump_or_open(const char *, int);
int rump_or_write(int, const void *, size_t);
int rump_or_read(int, void *, size_t);
int rump_or_ioctl(int, u_long, void *);
int rump_or_close(int);
int rump_or_fcntl(int, int, ...);
int rump_or_poll(struct pollfd *, nfds_t, int);
int rump_or_kqueue(void);
int rump_or_kevent(int, const struct kevent *, size_t,
	struct kevent *, size_t, const struct timespec *);
int hw_canplay(void);
int hw_canrec(void);
int hw_bidir(void);
int hw_fulldup(void);
void init(int);
void *consumer_thread(void *);
void cleanup_audiofd(void);
void TEST(const char *, ...) __printflike(1, 2);
bool xp_fail(int, const char *, ...) __printflike(2, 3);
void xp_skip(int, const char *, ...) __printflike(2, 3);
bool xp_eq(int, int, int, const char *);
bool xp_eq_str(int, const char *, const char *, const char *);
bool xp_ne(int, int, int, const char *);
bool xp_if(int, bool, const char *);
bool xp_sys_eq(int, int, int, const char *);
bool xp_sys_ok(int, int, const char *);
bool xp_sys_ng(int, int, int, const char *);
bool xp_sys_ptr(int, int, void *, const char *);
int debug_open(int, const char *, int);
int debug_write(int, int, const void *, size_t);
int debug_read(int, int, void *, size_t);
int debug_ioctl(int, int, u_long, const char *, void *, const char *, ...)
	__printflike(6, 7);
int debug_fcntl(int, int, int, const char *, ...) __printflike(4, 5);
int debug_close(int, int);
void *debug_mmap(int, void *, size_t, int, int, int, off_t);
int debug_munmap(int, void *, int);
const char *event_tostr(int);
int debug_poll(int, struct pollfd *, int, int);
int debug_kqueue(int);
int debug_kevent_set(int, int, const struct kevent *, size_t);
int debug_kevent_poll(int, int, struct kevent *, size_t,
	const struct timespec *);
void debug_kev(int, const char *, const struct kevent *);
uid_t debug_getuid(int);
int debug_seteuid(int, uid_t);
int debug_sysctlbyname(int, const char *, void *, size_t *, const void *,
	size_t);

int openable_mode(void);
int mode2aumode(int);
int mode2play(int);
int mode2rec(int);
void reset_after_mmap(void);

/* from audio.c */
static const char *encoding_names[] __unused = {
	"none",
	AudioEmulaw,
	AudioEalaw,
	"pcm16",
	"pcm8",
	AudioEadpcm,
	AudioEslinear_le,
	AudioEslinear_be,
	AudioEulinear_le,
	AudioEulinear_be,
	AudioEslinear,
	AudioEulinear,
	AudioEmpeg_l1_stream,
	AudioEmpeg_l1_packets,
	AudioEmpeg_l1_system,
	AudioEmpeg_l2_stream,
	AudioEmpeg_l2_packets,
	AudioEmpeg_l2_system,
	AudioEac3,
};

int debug;
int props;
int hwfull;
int netbsd;
bool opt_atf;
char testname[64];
int testcount;
int failcount;
int skipcount;
int unit;
bool use_rump;
bool use_pad;
bool exact_match;
int padfd;
int maxfd;
pthread_t th;
char devicename[16];	/* "audioN" */
char devaudio[16];	/* "/dev/audioN" */
char devsound[16];	/* "/dev/soundN" */
char devaudioctl[16];	/* "/dev/audioctlN" */
char devmixer[16];	/* "/dev/mixerN" */
extern struct testentry testtable[];

void
usage(void)
{
	fprintf(stderr, "usage:\t%s [<options>] [<testname>...]\n",
	    getprogname());
	fprintf(stderr, "\t-A        : make output suitable for ATF\n");
	fprintf(stderr, "\t-a        : Test all\n");
	fprintf(stderr, "\t-d        : Increase debug level\n");
	fprintf(stderr, "\t-e        : Use exact match for testnames "
	    "(default is forward match)\n");
	fprintf(stderr, "\t-l        : List all tests\n");
	fprintf(stderr, "\t-p        : Open pad\n");
#if !defined(NO_RUMP)
	fprintf(stderr, "\t-R        : Use rump (implies -p)\n");
#endif
	fprintf(stderr, "\t-u <unit> : Use audio<unit> (default:0)\n");
	exit(1);
}

/* Customized err(3) */
void
xp_err(int code, int line, const char *fmt, ...)
{
	va_list ap;
	int backup_errno;

	backup_errno = errno;
	printf("%s %d: ", (opt_atf ? "Line" : " ERROR:"), line);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(": %s\n", strerror(backup_errno));

	exit(code);
}

/* Customized errx(3) */
void
xp_errx(int code, int line, const char *fmt, ...)
{
	va_list ap;

	printf("%s %d: ", (opt_atf ? "Line" : " ERROR:"), line);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	exit(code);
}

int
main(int argc, char *argv[])
{
	int i;
	int j;
	int c;
	enum {
		CMD_TEST,
		CMD_ALL,
		CMD_LIST,
	} cmd;
	bool found;

	props = -1;
	hwfull = 0;
	unit = 0;
	cmd = CMD_TEST;
	use_pad = false;
	padfd = -1;
	exact_match = false;

	while ((c = getopt(argc, argv, "AadelpRu:")) != -1) {
		switch (c) {
		case 'A':
			opt_atf = true;
			break;
		case 'a':
			cmd = CMD_ALL;
			break;
		case 'd':
			debug++;
			break;
		case 'e':
			exact_match = true;
			break;
		case 'l':
			cmd = CMD_LIST;
			break;
		case 'p':
			use_pad = true;
			break;
		case 'R':
#if !defined(NO_RUMP)
			use_rump = true;
			use_pad = true;
#else
			usage();
#endif
			break;
		case 'u':
			unit = atoi(optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (cmd == CMD_LIST) {
		/* List all */
		for (i = 0; testtable[i].name != NULL; i++)
			printf("%s\n", testtable[i].name);
		return 0;
	}

	init(unit);

	if (cmd == CMD_ALL) {
		/* Test all */
		if (argc > 0)
			usage();
		for (i = 0; testtable[i].name != NULL; i++)
			do_test(i);
	} else {
		/* Test only matched */
		if (argc == 0)
			usage();

		found = false;
		for (j = 0; j < argc; j++) {
			for (i = 0; testtable[i].name != NULL; i++) {
				if (match(argv[j], testtable[i].name)) {
					do_test(i);
					found = true;
				}
			}
		}
		if (!found) {
			printf("test not found\n");
			exit(1);
		}
	}

	if (opt_atf == false) {
		printf("Result: %d tests, %d success",
		    testcount,
		    testcount - failcount - skipcount);
		if (failcount > 0)
			printf(", %d failed", failcount);
		if (skipcount > 0)
			printf(", %d skipped", skipcount);
		printf("\n");
	}

	if (skipcount > 0)
		return 2;
	if (failcount > 0)
		return 1;

	return 0;
}

bool
match(const char *arg, const char *name)
{
	if (exact_match) {
		/* Exact match */
		if (strcmp(arg, name) == 0)
			return true;
	} else {
		/* Forward match */
		if (strncmp(arg, name, strlen(arg)) == 0)
			return true;
	}
	return false;
}

/*
 * XXX
 * Some hardware drivers (e.g. hdafg(4)) require a little "rest" between
 * close(2) and re-open(2).
 * audio(4) uses hw_if->close() to tell the hardware to close.  However,
 * there is no agreement to wait for completion between MI and MD layer.
 * audio(4) immediately shifts the "closed" state, and that is, the next
 * open() will be acceptable immediately in audio layer.  But the real
 * hardware may not have been closed actually at that point.
 * It's troublesome issue but should be fixed...
 *
 * However, the most frequently used pad(4) (for ATF tests) doesn't have
 * such problem, so avoids it to reduce time.
 */
void
xxx_close_wait(void)
{

	if (!use_pad)
		usleep(500 * 1000);
}

void
do_test(int testnumber)
{
	/* Sentinel */
	strlcpy(testname, "<NoName>", sizeof(testname));
	/* Do test */
	testtable[testnumber].func();

	cleanup_audiofd();
	xxx_close_wait();
}

/*
 * system call wrappers for rump.
 */

/* open(2) or rump_sys_open(3) */
int
rump_or_open(const char *filename, int flag)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_open(filename, flag);
	else
#endif
		r = open(filename, flag);

	if (r > maxfd)
		maxfd = r;
	return r;
}

/* write(2) or rump_sys_write(3) */
int
rump_or_write(int fd, const void *buf, size_t len)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_write(fd, buf, len);
	else
#endif
		r = write(fd, buf, len);
	return r;
}

/* read(2) or rump_sys_read(3) */
int
rump_or_read(int fd, void *buf, size_t len)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_read(fd, buf, len);
	else
#endif
		r = read(fd, buf, len);
	return r;
}

/* ioctl(2) or rump_sys_ioctl(3) */
int
rump_or_ioctl(int fd, u_long cmd, void *arg)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_ioctl(fd, cmd, arg);
	else
#endif
		r = ioctl(fd, cmd, arg);
	return r;
}

/* close(2) or rump_sys_close(3) */
int
rump_or_close(int fd)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_close(fd);
	else
#endif
		r = close(fd);

	/* maxfd-1 may not valid fd but no matter */
	if (fd == maxfd)
		maxfd--;
	return r;
}

/* fcntl(2) or rump_sys_fcntl(3) */
/* XXX Supported only with no arguments for now */
int
rump_or_fcntl(int fd, int cmd, ...)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_fcntl(fd, cmd);
	else
#endif
		r = fcntl(fd, cmd);
	return r;
}

/* poll(2) or rump_sys_poll(3) */
int
rump_or_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_poll(fds, nfds, timeout);
	else
#endif
		r = poll(fds, nfds, timeout);
	return r;
}

/* kqueue(2) or rump_sys_kqueue(3) */
int
rump_or_kqueue(void)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_kqueue();
	else
#endif
		r = kqueue();
	return r;
}

/* kevent(2) or rump_sys_kevent(3) */
int
rump_or_kevent(int kq, const struct kevent *chlist, size_t nch,
	struct kevent *evlist, size_t nev,
	const struct timespec *timeout)
{
	int r;

#if !defined(NO_RUMP)
	if (use_rump)
		r = rump_sys_kevent(kq, chlist, nch, evlist, nev, timeout);
	else
#endif
		r = kevent(kq, chlist, nch, evlist, nev, timeout);
	return r;
}

int
hw_canplay(void)
{
	return (props & AUDIO_PROP_PLAYBACK) ? 1 : 0;
}

int
hw_canrec(void)
{
	return (props & AUDIO_PROP_CAPTURE) ? 1 : 0;
}

int
hw_bidir(void)
{
	return hw_canplay() & hw_canrec();
}

int
hw_fulldup(void)
{
	return (props & AUDIO_PROP_FULLDUPLEX) ? 1 : 0;
}

#define DPRINTF(fmt...) do {	\
	if (debug)		\
		printf(fmt);	\
} while (0)

#define DPRINTFF(line, fmt...) do {		\
	if (debug) {				\
		printf("  > %d: ", line);	\
		DPRINTF(fmt);			\
		fflush(stdout);			\
	}					\
} while (0)

#define DRESULT(r) do {				\
	int backup_errno = errno;		\
	if (r == -1) {				\
		DPRINTF(" = %d, err#%d %s\n",	\
		    r, backup_errno,		\
		    strerror(backup_errno));	\
	} else {				\
		DPRINTF(" = %d\n", r);		\
	}					\
	errno = backup_errno;			\
	return r;				\
} while (0)

/* pointer variants for mmap */
#define DRESULT_PTR(r) do {			\
	int backup_errno = errno;		\
	if (r == (void *)-1) {			\
		DPRINTF(" = -1, err#%d %s\n",	\
		    backup_errno,		\
		    strerror(backup_errno));	\
	} else {				\
		DPRINTF(" = %p\n", r);		\
	}					\
	errno = backup_errno;			\
	return r;				\
} while (0)


/*
 * requnit <  0: Use auto by pad (not implemented).
 * requnit >= 0: Use audio<requnit>.
 */
void
init(int requnit)
{
	struct audio_device devinfo;
	size_t len;
	int rel;
	int fd;
	int r;

	/* XXX */
	atexit(cleanup_audiofd);

	if (requnit < 0) {
		xp_errx(1, __LINE__, "requnit < 0 not implemented.");
	} else {
		unit = requnit;
	}

	/* Set device name */
	snprintf(devicename, sizeof(devicename), "audio%d", unit);
	snprintf(devaudio, sizeof(devaudio), "/dev/audio%d", unit);
	snprintf(devsound, sizeof(devsound), "/dev/sound%d", unit);
	snprintf(devaudioctl, sizeof(devaudioctl), "/dev/audioctl%d", unit);
	snprintf(devmixer, sizeof(devmixer), "/dev/mixer%d", unit);

	/*
	 * version
	 * audio2 is merged in 8.99.39.
	 */
	len = sizeof(rel);
	r = sysctlbyname("kern.osrevision", &rel, &len, NULL, 0);
	if (r == -1)
		xp_err(1, __LINE__, "sysctl kern.osrevision");
	netbsd = rel / 100000000;
	if (rel >=  899003900)
		netbsd = 9;

#if !defined(NO_RUMP)
	if (use_rump) {
		DPRINTF("  use rump\n");
		rump_init();
	}
#endif

	/*
	 * Open pad device before all accesses (including /dev/audioctl).
	 */
	if (use_pad) {
		padfd = rump_or_open("/dev/pad0", O_RDONLY);
		if (padfd == -1)
			xp_err(1, __LINE__, "rump_or_open");

		/* Create consumer thread */
		pthread_create(&th, NULL, consumer_thread, NULL);
		/* Set this thread's name */
		pthread_setname_np(pthread_self(), "main", NULL);
	}

	/*
	 * Get device properties, etc.
	 */
	fd = rump_or_open(devaudioctl, O_RDONLY);
	if (fd == -1)
		xp_err(1, __LINE__, "open %s", devaudioctl);
	r = rump_or_ioctl(fd, AUDIO_GETPROPS, &props);
	if (r == -1)
		xp_err(1, __LINE__, "AUDIO_GETPROPS");
	r = rump_or_ioctl(fd, AUDIO_GETDEV, &devinfo);
	if (r == -1)
		xp_err(1, __LINE__, "AUDIO_GETDEV");
	rump_or_close(fd);

	if (debug) {
		printf("  device = %s, %s, %s\n",
		    devinfo.name, devinfo.version, devinfo.config);
		printf("  hw props =");
		if (hw_canplay())
			printf(" playback");
		if (hw_canrec())
			printf(" capture");
		if (hw_fulldup())
			printf(" fullduplex");
		printf("\n");
	}

}

/* Consumer thread used by pad */
void *
consumer_thread(void *arg)
{
	char buf[1024];
	int r;

	pthread_setname_np(pthread_self(), "consumer", NULL);
	pthread_detach(pthread_self());

	/* throw away data anyway */
	for (;;) {
		r = read(padfd, buf, sizeof(buf));
		if (r < 1)
			break;
	}

	pthread_exit(NULL);
}

/*
 * XXX
 * Closing pad descriptor before audio descriptor causes panic (PR kern/54427).
 * To avoid this, close non-pad descriptor first using atexit(3) for now.
 * This is just a workaround and this function should be removed.
 */
void cleanup_audiofd()
{
	int fd;

	for (fd = 3; fd <= maxfd; fd++) {
		if (fd != padfd)
			close(fd);
	}
	maxfd = 3;
}

/*
 * Support functions
 */

/* Set testname */
void
TEST(const char *name, ...)
{
	va_list ap;

	va_start(ap, name);
	vsnprintf(testname, sizeof(testname), name, ap);
	va_end(ap);
	if (opt_atf == false) {
		printf("%s\n", testname);
		fflush(stdout);
	}
}

/*
 * XP_FAIL() should be called when this test fails.
 * If caller already count up testcount, call xp_fail() instead.
 */
#define XP_FAIL(fmt...)	do {	\
	testcount++;	\
	xp_fail(__LINE__, fmt);	\
} while (0)
bool xp_fail(int line, const char *fmt, ...)
{
	va_list ap;

	printf("%s %d: ", (opt_atf ? "Line" : " FAIL:"), line);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fflush(stdout);
	failcount++;

	return false;
}

/*
 * XP_SKIP() should be called when you want to skip this test.
 * If caller already count up testcount, call xp_skip() instead.
 */
#define XP_SKIP(fmt...)	do { \
	testcount++;	\
	xp_skip(__LINE__, fmt);	\
} while (0)
void xp_skip(int line, const char *fmt, ...)
{
	va_list ap;

	printf("%s %d: ", (opt_atf ? "Line" : " SKIP:"), line);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fflush(stdout);
	skipcount++;
}

#define XP_EQ(exp, act)	xp_eq(__LINE__, exp, act, #act)
bool xp_eq(int line, int exp, int act, const char *varname)
{
	bool r = true;

	testcount++;
	if (exp != act) {
		r = xp_fail(line, "%s expects %d but %d", varname, exp, act);
	}
	return r;
}
#define XP_EQ_STR(exp, act) xp_eq_str(__LINE__, exp, act, #act)
bool xp_eq_str(int line, const char *exp, const char *act, const char *varname)
{
	bool r = true;

	testcount++;
	if (strcmp(exp, act) != 0) {
		r = xp_fail(line, "%s expects \"%s\" but \"%s\"",
		    varname, exp, act);
	}
	return r;
}

#define XP_NE(exp, act)	xp_ne(__LINE__, exp, act, #act)
bool xp_ne(int line, int exp, int act, const char *varname)
{
	bool r = true;

	testcount++;
	if (exp == act) {
		r = xp_fail(line, "%s expects != %d but %d", varname, exp, act);
	}
	return r;
}

/* This expects that result is expressed in expr. */
/* GCC extension */
#define XP_IF(expr) xp_if(__LINE__, (expr), #expr)
bool xp_if(int line, bool expr, const char *exprname)
{
	bool r = true;
	testcount++;
	if (!expr) {
		r = xp_fail(__LINE__, "(%s) is expected but not met", exprname);
	}
	return r;
}

/* This expects that the system call returns 'exp'. */
#define XP_SYS_EQ(exp, act)	xp_sys_eq(__LINE__, exp, act, #act)
bool xp_sys_eq(int line, int exp, int act, const char *varname)
{
	bool r = true;

	testcount++;
	if (act == -1) {
		r = xp_fail(line, "%s expects %d but -1,err#%d(%s)",
		    varname, exp, errno, strerror(errno));
	} else {
		r = xp_eq(line, exp, act, varname);
	}
	return r;
}

/*
 * This expects that system call succeeds.
 * This is useful when you expect the system call succeeds but don't know
 * the expected return value, such as open(2).
 */
#define XP_SYS_OK(act)	xp_sys_ok(__LINE__, act, #act)
bool xp_sys_ok(int line, int act, const char *varname)
{
	bool r = true;

	testcount++;
	if (act == -1) {
		r = xp_fail(line, "%s expects success but -1,err#%d(%s)",
		    varname, errno, strerror(errno));
	}
	return r;
}

/* This expects that the system call fails with 'experrno'. */
#define XP_SYS_NG(experrno, act) xp_sys_ng(__LINE__, experrno, act, #act)
bool xp_sys_ng(int line, int experrno, int act, const char *varname)
{
	bool r = true;

	testcount++;
	if (act != -1) {
		r = xp_fail(line, "%s expects -1,err#%d but %d",
		    varname, experrno, act);
	} else if (experrno != errno) {
		char acterrbuf[100];
		int acterrno = errno;
		strlcpy(acterrbuf, strerror(acterrno), sizeof(acterrbuf));
		r = xp_fail(line, "%s expects -1,err#%d(%s) but -1,err#%d(%s)",
		    varname, experrno, strerror(experrno),
		    acterrno, acterrbuf);
	}
	return r;
}

/*
 * When exp == 0, this expects that the system call succeeds with returned
 * pointer is not -1.
 * When exp != 0, this expects that the system call fails with returned
 * pointer is -1 and its errno is exp.
 * It's only for mmap().
 */
#define XP_SYS_PTR(exp, act) xp_sys_ptr(__LINE__, exp, act, #act)
bool xp_sys_ptr(int line, int exp, void *act, const char *varname)
{
	char errbuf[256];
	int actual_errno;
	bool r = true;

	testcount++;
	if (exp == 0) {
		/* expects to succeed */
		if (act == (void *)-1) {
			r = xp_fail(line,
			    "%s expects success but -1,err#%d(%s)",
			    varname, errno, strerror(errno));
		}
	} else {
		/* expects to fail */
		if (act != (void *)-1) {
			r = xp_fail(line,
			    "%s expects -1,err#%d(%s) but success",
			    varname, exp, strerror(exp));
		} else if (exp != errno) {
			actual_errno = errno;
			strerror_r(actual_errno, errbuf, sizeof(errbuf));
			r = xp_fail(line,
			    "%s expects -1,err#%d(%s) but -1,err#%d(%s)",
			    varname, exp, strerror(exp), actual_errno, errbuf);
		}
	}
	return r;
}


/*
 * REQUIRED_* return immediately if condition does not meet.
 */
#define REQUIRED_EQ(e, a) do { if (!XP_EQ(e, a)) return; } while (0)
#define REQUIRED_NE(e, a) do { if (!XP_NE(e, a)) return; } while (0)
#define REQUIRED_IF(expr) do { if (!XP_IF(expr)) return; } while (0)
#define REQUIRED_SYS_EQ(e, a) do { if (!XP_SYS_EQ(e, a)) return; } while (0)
#define REQUIRED_SYS_OK(a)    do { if (!XP_SYS_OK(a))    return; } while (0)


static const char *openmode_str[] = {
	"O_RDONLY",
	"O_WRONLY",
	"O_RDWR",
};


/*
 * All system calls in following tests should be called with these macros.
 */

#define OPEN(name, mode)	\
	debug_open(__LINE__, name, mode)
int debug_open(int line, const char *name, int mode)
{
	char modestr[32];
	int n;

	if ((mode & 3) != 3) {
		n = snprintf(modestr, sizeof(modestr), "%s",
		    openmode_str[mode & 3]);
	} else {
		n = snprintf(modestr, sizeof(modestr), "%d", mode & 3);
	}
	if ((mode & O_NONBLOCK))
		n += snprintf(modestr + n, sizeof(modestr) - n, "|O_NONBLOCK");

	DPRINTFF(line, "open(\"%s\", %s)", name, modestr);
	int r = rump_or_open(name, mode);
	DRESULT(r);
}

#define WRITE(fd, addr, len)	\
	debug_write(__LINE__, fd, addr, len)
int debug_write(int line, int fd, const void *addr, size_t len)
{
	DPRINTFF(line, "write(%d, %p, %zd)", fd, addr, len);
	int r = rump_or_write(fd, addr, len);
	DRESULT(r);
}

#define READ(fd, addr, len)	\
	debug_read(__LINE__, fd, addr, len)
int debug_read(int line, int fd, void *addr, size_t len)
{
	DPRINTFF(line, "read(%d, %p, %zd)", fd, addr, len);
	int r = rump_or_read(fd, addr, len);
	DRESULT(r);
}

/*
 * addrstr is the comment for debug message.
 *   int onoff = 0;
 *   ioctl(fd, SWITCH, onoff); -> IOCTL(fd, SWITCH, onoff, "off");
 */
#define IOCTL(fd, name, addr, addrfmt...)	\
	debug_ioctl(__LINE__, fd, name, #name, addr, addrfmt)
int debug_ioctl(int line, int fd, u_long name, const char *namestr,
	void *addr, const char *addrfmt, ...)
{
	char addrbuf[100];
	va_list ap;

	va_start(ap, addrfmt);
	vsnprintf(addrbuf, sizeof(addrbuf), addrfmt, ap);
	va_end(ap);
	DPRINTFF(line, "ioctl(%d, %s, %s)", fd, namestr, addrbuf);
	int r = rump_or_ioctl(fd, name, addr);
	DRESULT(r);
}

#define FCNTL(fd, name...)	\
	debug_fcntl(__LINE__, fd, name, #name)
int debug_fcntl(int line, int fd, int name, const char *namestr, ...)
{
	int r;

	switch (name) {
	 case F_GETFL:	/* no arguments */
		DPRINTFF(line, "fcntl(%d, %s)", fd, namestr);
		r = rump_or_fcntl(fd, name);
		break;
	 default:
		__unreachable();
	}
	DRESULT(r);
	return r;
}

#define CLOSE(fd)	\
	debug_close(__LINE__, fd)
int debug_close(int line, int fd)
{
	DPRINTFF(line, "close(%d)", fd);
	int r = rump_or_close(fd);
	DRESULT(r);
}

#define MMAP(ptr, len, prot, flags, fd, offset)	\
	debug_mmap(__LINE__, ptr, len, prot, flags, fd, offset)
void *debug_mmap(int line, void *ptr, size_t len, int prot, int flags, int fd,
	off_t offset)
{
	char protbuf[256];
	char flagbuf[256];
	int n;

#define ADDFLAG(buf, var, name)	do {				\
	if (((var) & (name)))					\
		n = strlcat(buf, "|" #name, sizeof(buf));	\
		var &= ~(name);					\
} while (0)

	n = 0;
	protbuf[n] = '\0';
	if (prot == 0) {
		strlcpy(protbuf, "|PROT_NONE", sizeof(protbuf));
	} else {
		ADDFLAG(protbuf, prot, PROT_EXEC);
		ADDFLAG(protbuf, prot, PROT_WRITE);
		ADDFLAG(protbuf, prot, PROT_READ);
		if (prot != 0) {
			snprintf(protbuf + n, sizeof(protbuf) - n,
			    "|prot=0x%x", prot);
		}
	}

	n = 0;
	flagbuf[n] = '\0';
	if (flags == 0) {
		strlcpy(flagbuf, "|MAP_FILE", sizeof(flagbuf));
	} else {
		ADDFLAG(flagbuf, flags, MAP_SHARED);
		ADDFLAG(flagbuf, flags, MAP_PRIVATE);
		ADDFLAG(flagbuf, flags, MAP_FIXED);
		ADDFLAG(flagbuf, flags, MAP_INHERIT);
		ADDFLAG(flagbuf, flags, MAP_HASSEMAPHORE);
		ADDFLAG(flagbuf, flags, MAP_TRYFIXED);
		ADDFLAG(flagbuf, flags, MAP_WIRED);
		ADDFLAG(flagbuf, flags, MAP_ANON);
		if (flags != 0) {
			n += snprintf(flagbuf + n, sizeof(flagbuf) - n,
			    "|flag=0x%x", flags);
		}
	}

	DPRINTFF(line, "mmap(%p, %zd, %s, %s, %d, %jd)",
	    ptr, len, protbuf + 1, flagbuf + 1, fd, offset);
	void *r = mmap(ptr, len, prot, flags, fd, offset);
	DRESULT_PTR(r);
}

#define MUNMAP(ptr, len)	\
	debug_munmap(__LINE__, ptr, len)
int debug_munmap(int line, void *ptr, int len)
{
#if !defined(NO_RUMP)
	if (use_rump)
		xp_errx(1, __LINE__, "rump doesn't support munmap");
#endif
	DPRINTFF(line, "munmap(%p, %d)", ptr, len);
	int r = munmap(ptr, len);
	DRESULT(r);
}

const char *
event_tostr(int events)
{
	static char buf[64];

	snprintb(buf, sizeof(buf),
	    "\177\020" \
	    "b\10WRBAND\0" \
	    "b\7RDBAND\0" "b\6RDNORM\0" "b\5NVAL\0" "b\4HUP\0" \
	    "b\3ERR\0" "b\2OUT\0" "b\1PRI\0" "b\0IN\0",
	    events);
	return buf;
}

#define POLL(pfd, nfd, timeout)	\
	debug_poll(__LINE__, pfd, nfd, timeout)
int debug_poll(int line, struct pollfd *pfd, int nfd, int timeout)
{
	char buf[256];
	int n = 0;
	buf[n] = '\0';
	for (int i = 0; i < nfd; i++) {
		n += snprintf(buf + n, sizeof(buf) - n, "{fd=%d,events=%s}",
		    pfd[i].fd, event_tostr(pfd[i].events));
	}
	DPRINTFF(line, "poll(%s, %d, %d)", buf, nfd, timeout);
	int r = rump_or_poll(pfd, nfd, timeout);
	DRESULT(r);
}

#define KQUEUE()	\
	debug_kqueue(__LINE__)
int debug_kqueue(int line)
{
	DPRINTFF(line, "kqueue()");
	int r = rump_or_kqueue();
	DRESULT(r);
}

#define KEVENT_SET(kq, kev, nev)	\
	debug_kevent_set(__LINE__, kq, kev, nev)
int debug_kevent_set(int line, int kq, const struct kevent *kev, size_t nev)
{
	DPRINTFF(line, "kevent_set(%d, %p, %zd)", kq, kev, nev);
	int r = rump_or_kevent(kq, kev, nev, NULL, 0, NULL);
	DRESULT(r);
}

#define KEVENT_POLL(kq, kev, nev, ts) \
	debug_kevent_poll(__LINE__, kq, kev, nev, ts)
int debug_kevent_poll(int line, int kq, struct kevent *kev, size_t nev,
	const struct timespec *ts)
{
	char tsbuf[32];

	if (ts == NULL) {
		snprintf(tsbuf, sizeof(tsbuf), "NULL");
	} else if (ts->tv_sec == 0 && ts->tv_nsec == 0) {
		snprintf(tsbuf, sizeof(tsbuf), "0.0");
	} else {
		snprintf(tsbuf, sizeof(tsbuf), "%d.%09ld",
			(int)ts->tv_sec, ts->tv_nsec);
	}
	DPRINTFF(line, "kevent_poll(%d, %p, %zd, %s)", kq, kev, nev, tsbuf);
	int r = rump_or_kevent(kq, NULL, 0, kev, nev, ts);
	DRESULT(r);
}

#define DEBUG_KEV(name, kev)	\
	debug_kev(__LINE__, name, kev)
void debug_kev(int line, const char *name, const struct kevent *kev)
{
	char flagbuf[256];
	const char *filterbuf;
	uint32_t v;
	int n;

	n = 0;
	flagbuf[n] = '\0';
	if (kev->flags == 0) {
		strcpy(flagbuf, "|0?");
	} else {
		v = kev->flags;
		ADDFLAG(flagbuf, v, EV_ADD);
		if (v != 0)
			snprintf(flagbuf + n, sizeof(flagbuf)-n, "|0x%x", v);
	}

	switch (kev->filter) {
	 case EVFILT_READ:	filterbuf = "EVFILT_READ";	break;
	 case EVFILT_WRITE:	filterbuf = "EVFILT_WRITE";	break;
	 default:		filterbuf = "EVFILT_?";		break;
	}

	DPRINTFF(line,
	    "%s={id:%d,%s,%s,fflags:0x%x,data:0x%" PRIx64 ",udata:0x%x}\n",
	    name,
	    (int)kev->ident,
	    flagbuf + 1,
	    filterbuf,
	    kev->fflags,
	    kev->data,
	    (int)(intptr_t)kev->udata);
}

/* XXX rump? */
#define GETUID()	\
	debug_getuid(__LINE__)
uid_t debug_getuid(int line)
{
	DPRINTFF(line, "getuid");
	uid_t r = getuid();
	/* getuid() never fails */
	DPRINTF(" = %u\n", r);
	return r;
}

/* XXX rump? */
#define SETEUID(id)	\
	debug_seteuid(__LINE__, id)
int debug_seteuid(int line, uid_t id)
{
	DPRINTFF(line, "seteuid(%d)", (int)id);
	int r = seteuid(id);
	DRESULT(r);
}

#define SYSCTLBYNAME(name, oldp, oldlenp, newp, newlen)	\
	debug_sysctlbyname(__LINE__, name, oldp, oldlenp, newp, newlen)
int debug_sysctlbyname(int line, const char *name, void *oldp, size_t *oldlenp,
	const void *newp, size_t newlen)
{
	DPRINTFF(line, "sysctlbyname(\"%s\")", name);
	int r = sysctlbyname(name, oldp, oldlenp, newp, newlen);
	DRESULT(r);
}


/* Return openable mode on this hardware property */
int
openable_mode(void)
{
	if (hw_bidir())
		return O_RDWR;
	if (hw_canplay())
		return O_WRONLY;
	else
		return O_RDONLY;
}

int mode2aumode_full[] = {
	                                AUMODE_RECORD,	/* O_RDONLY */
	AUMODE_PLAY | AUMODE_PLAY_ALL,			/* O_WRONLY */
	AUMODE_PLAY | AUMODE_PLAY_ALL | AUMODE_RECORD,	/* O_RDWR   */
};

/* Convert openmode(O_*) to AUMODE_*, with hardware property */
int
mode2aumode(int mode)
{
	int aumode;

	aumode = mode2aumode_full[mode];
	if (hw_canplay() == 0)
		aumode &= ~(AUMODE_PLAY | AUMODE_PLAY_ALL);
	if (hw_canrec() == 0)
		aumode &= ~AUMODE_RECORD;

	if (netbsd >= 9) {
		/* half-duplex treats O_RDWR as O_WRONLY */
		if (mode == O_RDWR && hw_bidir() && hw_fulldup() == 0)
			aumode &= ~AUMODE_RECORD;
	}

	return aumode;
}

/* Is this mode + hardware playable? */
int
mode2play(int mode)
{
	int aumode;

	aumode = mode2aumode(mode);
	return ((aumode & AUMODE_PLAY)) ? 1 : 0;
}

/* Is this mode + hardware recordable? */
int
mode2rec(int mode)
{
	int aumode;

	aumode = mode2aumode(mode);
	return ((aumode & AUMODE_RECORD)) ? 1 : 0;
}

/*
 * On NetBSD7, open() after-closing-mmap fails due to a bug.
 * It happens once every two times like flip-flop, so the workaround is
 * to open it again.
 */
void
reset_after_mmap(void)
{
	int fd;

	if (netbsd < 8) {
		fd = OPEN(devaudio, O_WRONLY);
		if (fd != -1)
			CLOSE(fd);
	}
}

/*
 * Lookup "outputs.master" and return its mixer device index.
 * It may not be strict but I'm not sure.
 */
int
mixer_get_outputs_master(int mixerfd)
{
	const char * const typename[] = { "CLASS", "ENUM", "SET", "VALUE" };
	mixer_devinfo_t di;
	int class_outputs;
	int i;
	int r;

	class_outputs = -1;
	for (i = 0; ; i++) {
		memset(&di, 0, sizeof(di));
		di.index = i;
		r = IOCTL(mixerfd, AUDIO_MIXER_DEVINFO, &di, "index=%d", i);
		if (r < 0)
			break;
		DPRINTF("  > type=%s(%d) mixer_class=%d name=%s\n",
		    (0 <= di.type && di.type <= 3) ? typename[di.type] : "",
		    di.type, di.mixer_class, di.label.name);
		if (di.type == AUDIO_MIXER_CLASS &&
		    strcmp(di.label.name, "outputs") == 0) {
			class_outputs = di.mixer_class;
			DPRINTF("  > class_output=%d\n", class_outputs);
			continue;
		}
		if (di.type == AUDIO_MIXER_VALUE &&
		    di.mixer_class == class_outputs &&
		    strcmp(di.label.name, "master") == 0) {
			return i;
		}
	}
	/* Not found */
	return -1;
}

/*
 * Tests
 */

void test_open_mode(int);
void test_open(const char *, int);
void test_open_simul(int, int);
void try_open_multiuser(bool);
void test_open_multiuser(bool);
void test_rdwr_fallback(int, bool, bool);
void test_rdwr_two(int, int);
void test_mmap_mode(int, int);
void test_poll_mode(int, int, int);
void test_poll_in_open(const char *);
void test_kqueue_mode(int, int, int);
volatile int sigio_caught;
void signal_FIOASYNC(int);
void test_AUDIO_SETFD_xxONLY(int);
void test_AUDIO_SETINFO_mode(int, int, int, int);
void test_AUDIO_SETINFO_params_set(int, int, int);
void test_AUDIO_SETINFO_pause(int, int, int);
int getenc_make_table(int, int[][5]);
void xp_getenc(int[][5], int, int, int, struct audio_prinfo *);
void getenc_check_encodings(int, int[][5]);
void test_AUDIO_ERROR(int);
void test_AUDIO_GETIOFFS_one(int);
void test_AUDIO_GETOOFFS_one(int);
void test_AUDIO_GETOOFFS_wrap(int);
void test_AUDIO_GETOOFFS_flush(int);
void test_AUDIO_GETOOFFS_set(int);
void test_audioctl_open_1(int, int);
void test_audioctl_open_2(int, int);
void try_audioctl_open_multiuser(const char *, const char *);
void test_audioctl_open_multiuser(bool, const char *, const char *);
void test_audioctl_rw(int);

#define DEF(name) \
	void test__ ## name (void); \
	void test__ ## name (void)

/*
 * Whether it can be open()ed with specified mode.
 */
void
test_open_mode(int mode)
{
	int fd;
	int r;

	TEST("open_mode_%s", openmode_str[mode] + 2);

	fd = OPEN(devaudio, mode);
	if (mode2aumode(mode) != 0) {
		XP_SYS_OK(fd);
	} else {
		XP_SYS_NG(ENXIO, fd);
	}

	if (fd >= 0) {
		r = CLOSE(fd);
		XP_SYS_EQ(0, r);
	}
}
DEF(open_mode_RDONLY)	{ test_open_mode(O_RDONLY); }
DEF(open_mode_WRONLY)	{ test_open_mode(O_WRONLY); }
DEF(open_mode_RDWR)	{ test_open_mode(O_RDWR);   }

/*
 * Check the initial parameters and stickiness.
 * /dev/audio
 *	The initial parameters are always the same whenever you open.
 * /dev/sound and /dev/audioctl
 *	The initial parameters are inherited from the last /dev/sound or
 *	/dev/audio.
 */
void
test_open(const char *devname, int mode)
{
	struct audio_info ai;
	struct audio_info ai0;
	char devfile[16];
	int fd;
	int r;
	int can_play;
	int can_rec;
	int exp_mode;
	int exp_encoding;
	int exp_precision;
	int exp_channels;
	int exp_sample_rate;
	int exp_pause;
	int exp_popen;
	int exp_ropen;

	TEST("open_%s_%s", devname, openmode_str[mode] + 2);

	snprintf(devfile, sizeof(devfile), "/dev/%s%d", devname, unit);
	can_play = mode2play(mode);
	can_rec  = mode2rec(mode);
	if (strcmp(devname, "audioctl") != 0) {
		if (can_play + can_rec == 0) {
			/* Check whether it cannot be opened */
			fd = OPEN(devaudio, mode);
			XP_SYS_NG(ENXIO, fd);
			return;
		}
	}

	/* /dev/audio is always initialized */
	if (strcmp(devname, "audio") == 0) {
		exp_encoding = AUDIO_ENCODING_ULAW;
		exp_precision = 8;
		exp_channels = 1;
		exp_sample_rate = 8000;
		exp_pause = 0;
	} else {
		exp_encoding = AUDIO_ENCODING_SLINEAR_LE;
		exp_precision = 16;
		exp_channels = 2;
		exp_sample_rate = 11025;
		exp_pause = 1;
	}

	/* /dev/audioctl is always "not opened" */
	if (strcmp(devname, "audioctl") == 0) {
		exp_mode = 0;
		exp_popen = 0;
		exp_ropen = 0;
	} else {
		exp_mode = mode2aumode(mode);
		exp_popen = can_play;
		exp_ropen = can_rec;
	}


	/*
	 * At first, initialize the sticky parameters both of play and rec.
	 * This uses /dev/audio to verify /dev/audio.  It's not good way but
	 * I don't have better one...
	 */
	fd = OPEN(devaudio, openable_mode());
	REQUIRED_SYS_OK(fd);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	/*
	 * Open target device and check the initial parameters
	 * At this moment, all devices are initialized by default.
	 */
	fd = OPEN(devfile, mode);
	REQUIRED_SYS_OK(fd);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);

	XP_NE(0, ai.blocksize);
		/* hiwat/lowat */
	XP_EQ(exp_mode, ai.mode);
	/* ai.play */
	XP_EQ(8000, ai.play.sample_rate);
	XP_EQ(1, ai.play.channels);
	XP_EQ(8, ai.play.precision);
	XP_EQ(AUDIO_ENCODING_ULAW, ai.play.encoding);
		/* gain */
		/* port */
	XP_EQ(0, ai.play.seek);
		/* avail_ports */
	XP_NE(0, ai.play.buffer_size);
	XP_EQ(0, ai.play.samples);
	XP_EQ(0, ai.play.eof);
	XP_EQ(0, ai.play.pause);
	XP_EQ(0, ai.play.error);
	XP_EQ(0, ai.play.waiting);
		/* balance */
	XP_EQ(exp_popen, ai.play.open);
	XP_EQ(0, ai.play.active);
	/* ai.record */
	XP_EQ(8000, ai.record.sample_rate);
	XP_EQ(1, ai.record.channels);
	XP_EQ(8, ai.record.precision);
	XP_EQ(AUDIO_ENCODING_ULAW, ai.record.encoding);
		/* gain */
		/* port */
	XP_EQ(0, ai.record.seek);
		/* avail_ports */
	XP_NE(0, ai.record.buffer_size);
	XP_EQ(0, ai.record.samples);
	XP_EQ(0, ai.record.eof);
	XP_EQ(0, ai.record.pause);
	XP_EQ(0, ai.record.error);
	XP_EQ(0, ai.record.waiting);
		/* balance */
	XP_EQ(exp_ropen, ai.record.open);
	if (netbsd < 9 && strcmp(devname, "sound") == 0) {
		/*
		 * On NetBSD7/8, it doesn't seem to start recording on open
		 * for /dev/sound.  It should be a bug.
		 */
		XP_EQ(0, ai.record.active);
	} else {
		XP_EQ(exp_ropen, ai.record.active);
	}
	/* Save it */
	ai0 = ai;

	/*
	 * Change much as possible
	 */
	AUDIO_INITINFO(&ai);
	ai.mode = ai0.mode ^ AUMODE_PLAY_ALL;
	ai.play.sample_rate = 11025;
	ai.play.channels = 2;
	ai.play.precision = 16;
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.play.pause = 1;
	ai.record.sample_rate = 11025;
	ai.record.channels = 2;
	ai.record.precision = 16;
	ai.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.record.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "ai");
	REQUIRED_SYS_EQ(0, r);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	/*
	 * Open the same target device again and check
	 */
	fd = OPEN(devfile, mode);
	REQUIRED_SYS_OK(fd);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);

	XP_NE(0, ai.blocksize);
		/* hiwat/lowat */
	if (netbsd < 8) {
		/*
		 * On NetBSD7, the behavior when changing ai.mode on
		 * /dev/audioctl can not be explained yet but I won't
		 * verify it more over.
		 */
	} else {
		/* On NetBSD9, changing mode never affects other fds */
		XP_EQ(exp_mode, ai.mode);
	}
	/* ai.play */
	XP_EQ(exp_sample_rate, ai.play.sample_rate);
	XP_EQ(exp_channels, ai.play.channels);
	XP_EQ(exp_precision, ai.play.precision);
	XP_EQ(exp_encoding, ai.play.encoding);
		/* gain */
		/* port */
	XP_EQ(0, ai.play.seek);
		/* avail_ports */
	XP_NE(0, ai.play.buffer_size);
	XP_EQ(0, ai.play.samples);
	XP_EQ(0, ai.play.eof);
	XP_EQ(exp_pause, ai.play.pause);
	XP_EQ(0, ai.play.error);
	XP_EQ(0, ai.play.waiting);
		/* balance */
	XP_EQ(exp_popen, ai.play.open);
	XP_EQ(0, ai.play.active);
	/* ai.record */
	XP_EQ(exp_sample_rate, ai.record.sample_rate);
	XP_EQ(exp_channels, ai.record.channels);
	XP_EQ(exp_precision, ai.record.precision);
	XP_EQ(exp_encoding, ai.record.encoding);
		/* gain */
		/* port */
	XP_EQ(0, ai.record.seek);
		/* avail_ports */
	XP_NE(0, ai.record.buffer_size);
	XP_EQ(0, ai.record.samples);
	XP_EQ(0, ai.record.eof);
	XP_EQ(exp_pause, ai.record.pause);
	XP_EQ(0, ai.record.error);
	XP_EQ(0, ai.record.waiting);
		/* balance */
	XP_EQ(exp_ropen, ai.record.open);
	if (netbsd < 9 && strcmp(devname, "sound") == 0) {
		/*
		 * On NetBSD7/8, it doesn't seem to start recording on open
		 * for /dev/sound.  It should be a bug.
		 */
		XP_EQ(0, ai.record.active);
	} else {
		XP_EQ(exp_ropen, ai.record.active);
	}

	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);
}
DEF(open_audio_RDONLY)		{ test_open("audio", O_RDONLY); }
DEF(open_audio_WRONLY)		{ test_open("audio", O_WRONLY); }
DEF(open_audio_RDWR)		{ test_open("audio", O_RDWR);   }
DEF(open_sound_RDONLY)		{ test_open("sound", O_RDONLY); }
DEF(open_sound_WRONLY)		{ test_open("sound", O_WRONLY); }
DEF(open_sound_RDWR)		{ test_open("sound", O_RDWR);   }
DEF(open_audioctl_RDONLY)	{ test_open("audioctl", O_RDONLY); }
DEF(open_audioctl_WRONLY)	{ test_open("audioctl", O_WRONLY); }
DEF(open_audioctl_RDWR)		{ test_open("audioctl", O_RDWR);   }

/*
 * Open (1) /dev/sound -> (2) /dev/audio -> (3) /dev/sound,
 * Both of /dev/audio and /dev/sound share the sticky parameters,
 * /dev/sound inherits and use it but /dev/audio initialize and use it.
 * So 2nd audio descriptor affects 3rd sound descriptor.
 */
DEF(open_sound_sticky)
{
	struct audio_info ai;
	int fd;
	int r;
	int openmode;

	TEST("open_sound_sticky");

	openmode = openable_mode();

	/* First, open /dev/sound and change encoding as a delegate */
	fd = OPEN(devsound, openmode);
	REQUIRED_SYS_OK(fd);
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	/* Next, open /dev/audio.  It makes the encoding mulaw */
	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	/* And then, open /dev/sound again */
	fd = OPEN(devsound, openmode);
	REQUIRED_SYS_OK(fd);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	XP_EQ(AUDIO_ENCODING_ULAW, ai.play.encoding);
	XP_EQ(AUDIO_ENCODING_ULAW, ai.record.encoding);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);
}

/*
 * /dev/audioctl has stickiness like /dev/sound.
 */
DEF(open_audioctl_sticky)
{
	struct audio_info ai;
	int fd;
	int r;
	int openmode;

	TEST("open_audioctl_sticky");

	openmode = openable_mode();

	/* First, open /dev/audio and change encoding */
	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.play.precision = 16;
	ai.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.record.precision = 16;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "SLINEAR_LE");
	REQUIRED_SYS_EQ(0, r);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	/* Next, open /dev/audioctl.  It should be affected */
	fd = OPEN(devaudioctl, openmode);
	REQUIRED_SYS_OK(fd);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	XP_EQ(AUDIO_ENCODING_SLINEAR_LE, ai.play.encoding);
	XP_EQ(16, ai.play.precision);
	XP_EQ(AUDIO_ENCODING_SLINEAR_LE, ai.record.encoding);
	XP_EQ(16, ai.record.precision);

	/* Then, change /dev/audioctl */
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_ULAW;
	ai.play.precision = 8;
	ai.record.encoding = AUDIO_ENCODING_ULAW;
	ai.record.precision = 8;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "ULAW");
	REQUIRED_SYS_EQ(0, r);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	/* Finally, open /dev/sound.  It also should be affected  */
	fd = OPEN(devsound, openmode);
	REQUIRED_SYS_OK(fd);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	XP_EQ(AUDIO_ENCODING_ULAW, ai.play.encoding);
	XP_EQ(8, ai.play.precision);
	XP_EQ(AUDIO_ENCODING_ULAW, ai.record.encoding);
	XP_EQ(8, ai.record.precision);
	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);
}

/*
 * Open two descriptors simultaneously.
 */
void
test_open_simul(int mode0, int mode1)
{
	struct audio_info ai;
	int fd0, fd1;
	int i;
	int r;
	int actmode;
#define AUMODE_BOTH (AUMODE_PLAY | AUMODE_RECORD)
	struct {
		int mode0;
		int mode1;
	} expfulltable[] = {
		/* expected fd0		expected fd1 (-errno expects error) */
		{ AUMODE_RECORD,	AUMODE_RECORD },	// REC, REC
		{ AUMODE_RECORD,	AUMODE_PLAY },		// REC, PLAY
		{ AUMODE_RECORD,	AUMODE_BOTH },		// REC, BOTH
		{ AUMODE_PLAY,		AUMODE_RECORD },	// PLAY, REC
		{ AUMODE_PLAY,		AUMODE_PLAY },		// PLAY, PLAY
		{ AUMODE_PLAY,		AUMODE_BOTH },		// PLAY, BOTH
		{ AUMODE_BOTH,		AUMODE_RECORD },	// BOTH, REC
		{ AUMODE_BOTH,		AUMODE_PLAY },		// BOTH, PLAY
		{ AUMODE_BOTH,		AUMODE_BOTH },		// BOTH, BOTH
	},
	exphalftable[] = {
		/* expected fd0		expected fd1 (-errno expects error) */
		{ AUMODE_RECORD,	AUMODE_RECORD },	// REC, REC
		{ AUMODE_RECORD,	-ENODEV },		// REC, PLAY
		{ AUMODE_RECORD,	-ENODEV },		// REC, BOTH
		{ AUMODE_PLAY,		-ENODEV },		// PLAY, REC
		{ AUMODE_PLAY,		AUMODE_PLAY },		// PLAY, PLAY
		{ AUMODE_PLAY,		AUMODE_PLAY },		// PLAY, BOTH
		{ AUMODE_PLAY,		-ENODEV },		// BOTH, REC
		{ AUMODE_PLAY,		AUMODE_PLAY },		// BOTH, PLAY
		{ AUMODE_PLAY,		AUMODE_PLAY },		// BOTH, BOTH
	}, *exptable;

	/* The expected values are different in half-duplex or full-duplex */
	if (hw_fulldup()) {
		exptable = expfulltable;
	} else {
		exptable = exphalftable;
	}

	TEST("open_simul_%s_%s",
	    openmode_str[mode0] + 2,
	    openmode_str[mode1] + 2);

	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}

	if (mode2aumode(mode0) == 0 || mode2aumode(mode1) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	i = mode0 * 3 + mode1;

	/* Open first one */
	fd0 = OPEN(devaudio, mode0);
	REQUIRED_SYS_OK(fd0);
	r = IOCTL(fd0, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	actmode = ai.mode & AUMODE_BOTH;
	XP_EQ(exptable[i].mode0, actmode);

	/* Open second one */
	fd1 = OPEN(devaudio, mode1);
	if (exptable[i].mode1 >= 0) {
		/* Case to expect to be able to open */
		REQUIRED_SYS_OK(fd1);
		r = IOCTL(fd1, AUDIO_GETBUFINFO, &ai, "");
		XP_SYS_EQ(0, r);
		if (r == 0) {
			actmode = ai.mode & AUMODE_BOTH;
			XP_EQ(exptable[i].mode1, actmode);
		}
	} else {
		/* Case to expect not to be able to open */
		XP_SYS_NG(ENODEV, fd1);
		if (fd1 == -1) {
			XP_EQ(-exptable[i].mode1, errno);
		} else {
			r = IOCTL(fd1, AUDIO_GETBUFINFO, &ai, "");
			XP_SYS_EQ(0, r);
			if (r == 0) {
				actmode = ai.mode & AUMODE_BOTH;
				XP_FAIL("expects error but %d", actmode);
			}
		}
	}

	if (fd1 >= 0) {
		r = CLOSE(fd1);
		XP_SYS_EQ(0, r);
	}

	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);
}
DEF(open_simul_RDONLY_RDONLY)	{ test_open_simul(O_RDONLY, O_RDONLY);	}
DEF(open_simul_RDONLY_WRONLY)	{ test_open_simul(O_RDONLY, O_WRONLY);	}
DEF(open_simul_RDONLY_RDWR)	{ test_open_simul(O_RDONLY, O_RDWR);	}
DEF(open_simul_WRONLY_RDONLY)	{ test_open_simul(O_WRONLY, O_RDONLY);	}
DEF(open_simul_WRONLY_WRONLY)	{ test_open_simul(O_WRONLY, O_WRONLY);	}
DEF(open_simul_WRONLY_RDWR)	{ test_open_simul(O_WRONLY, O_RDWR);	}
DEF(open_simul_RDWR_RDONLY)	{ test_open_simul(O_RDWR, O_RDONLY);	}
DEF(open_simul_RDWR_WRONLY)	{ test_open_simul(O_RDWR, O_WRONLY);	}
DEF(open_simul_RDWR_RDWR)	{ test_open_simul(O_RDWR, O_RDWR);	}

/*
 * /dev/audio can be opened by other user who opens /dev/audio.
 */
void
try_open_multiuser(bool multiuser)
{
	int fd0;
	int fd1;
	int r;
	uid_t ouid;

	/*
	 * Test1: Open as root first and then unprivileged user.
	 */

	/* At first, open as root */
	fd0 = OPEN(devaudio, openable_mode());
	REQUIRED_SYS_OK(fd0);

	ouid = GETUID();
	r = SETEUID(1);
	REQUIRED_SYS_EQ(0, r);

	/* Then, open as unprivileged user */
	fd1 = OPEN(devaudio, openable_mode());
	if (multiuser) {
		/* If multiuser, another user also can open */
		XP_SYS_OK(fd1);
	} else {
		/* If not multiuser, another user cannot open */
		XP_SYS_NG(EPERM, fd1);
	}
	if (fd1 != -1) {
		r = CLOSE(fd1);
		XP_SYS_EQ(0, r);
	}

	r = SETEUID(ouid);
	REQUIRED_SYS_EQ(0, r);

	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);

	/*
	 * Test2: Open as unprivileged user first and then root.
	 */

	/* At first, open as unprivileged user */
	ouid = GETUID();
	r = SETEUID(1);
	REQUIRED_SYS_EQ(0, r);

	fd0 = OPEN(devaudio, openable_mode());
	REQUIRED_SYS_OK(fd0);

	/* Then open as root */
	r = SETEUID(ouid);
	REQUIRED_SYS_EQ(0, r);

	/* root always can open */
	fd1 = OPEN(devaudio, openable_mode());
	XP_SYS_OK(fd1);
	if (fd1 != -1) {
		r = CLOSE(fd1);
		XP_SYS_EQ(0, r);
	}

	/* Close first one as unprivileged user */
	r = SETEUID(1);
	REQUIRED_SYS_EQ(0, r);
	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);
	r = SETEUID(ouid);
	REQUIRED_SYS_EQ(0, r);
}
/*
 * This is a wrapper for open_multiuser.
 * XXX XP_* macros are not compatible with on-error-goto, we need try-catch...
 */
void
test_open_multiuser(bool multiuser)
{
	char mibname[32];
	bool oldval;
	size_t oldlen;
	int r;

	TEST("open_multiuser_%d", multiuser);
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if (netbsd < 9) {
		/* NetBSD8 has no way (difficult) to determine device name */
		XP_SKIP("NetBSD8 cannot determine device name");
		return;
	}
	if (geteuid() != 0) {
		XP_SKIP("Must be run as a privileged user");
		return;
	}

	/* Get current multiuser mode (and save it) */
	snprintf(mibname, sizeof(mibname), "hw.%s.multiuser", devicename);
	oldlen = sizeof(oldval);
	r = SYSCTLBYNAME(mibname, &oldval, &oldlen, NULL, 0);
	REQUIRED_SYS_EQ(0, r);
	DPRINTF("  > multiuser=%d\n", oldval);

	/* Change if necessary */
	if (oldval != multiuser) {
		r = SYSCTLBYNAME(mibname, NULL, NULL, &multiuser,
		    sizeof(multiuser));
		REQUIRED_SYS_EQ(0, r);
		DPRINTF("  > new multiuser=%d\n", multiuser);
	}

	/* Do test */
	try_open_multiuser(multiuser);

	/* Restore multiuser mode */
	if (oldval != multiuser) {
		DPRINTF("  > restore multiuser to %d\n", oldval);
		r = SYSCTLBYNAME(mibname, NULL, NULL, &oldval, sizeof(oldval));
		REQUIRED_SYS_EQ(0, r);
	}
}
DEF(open_multiuser_0)	{ test_open_multiuser(false); }
DEF(open_multiuser_1)	{ test_open_multiuser(true); }

/*
 * Normal playback (with PLAY_ALL).
 * It does not verify real playback data.
 */
DEF(write_PLAY_ALL)
{
	char buf[8000];
	int fd;
	int r;

	TEST("write_PLAY_ALL");

	fd = OPEN(devaudio, O_WRONLY);
	if (hw_canplay()) {
		REQUIRED_SYS_OK(fd);
	} else {
		XP_SYS_NG(ENXIO, fd);
		return;
	}

	/* mulaw 1sec silence */
	memset(buf, 0xff, sizeof(buf));
	r = WRITE(fd, buf, sizeof(buf));
	XP_SYS_EQ(sizeof(buf), r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Normal playback (without PLAY_ALL).
 * It does not verify real playback data.
 */
DEF(write_PLAY)
{
	struct audio_info ai;
	char *wav;
	int wavsize;
	int totalsize;
	int fd;
	int r;

	TEST("write_PLAY");

	fd = OPEN(devaudio, O_WRONLY);
	if (hw_canplay()) {
		REQUIRED_SYS_OK(fd);
	} else {
		XP_SYS_NG(ENXIO, fd);
		return;
	}

	/* Drop PLAY_ALL */
	AUDIO_INITINFO(&ai);
	ai.mode = AUMODE_PLAY;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "mode");
	REQUIRED_SYS_EQ(0, r);

	/* Check mode and get blocksize */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	XP_EQ(AUMODE_PLAY, ai.mode);

	wavsize = ai.blocksize;
	wav = (char *)malloc(wavsize);
	REQUIRED_IF(wav != NULL);
	memset(wav, 0xff, wavsize);

	/* Write blocks until 1sec */
	for (totalsize = 0; totalsize < 8000; ) {
		r = WRITE(fd, wav, wavsize);
		XP_SYS_EQ(wavsize, r);
		if (r == -1)
			break;	/* XXX */
		totalsize += r;
	}

	/* XXX What should I test it? */
	/* Check ai.play.error */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	XP_EQ(0, ai.play.error);

	/* Playback data is no longer necessary */
	r = IOCTL(fd, AUDIO_FLUSH, NULL, "");
	REQUIRED_SYS_EQ(0, r);

	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);

	free(wav);
}

/*
 * Normal recording.
 * It does not verify real recorded data.
 */
DEF(read)
{
	char buf[8000];
	int fd;
	int r;

	TEST("read");

	fd = OPEN(devaudio, O_RDONLY);
	if (hw_canrec()) {
		REQUIRED_SYS_OK(fd);
	} else {
		XP_SYS_NG(ENXIO, fd);
		return;
	}

	/* mulaw 1sec */
	r = READ(fd, buf, sizeof(buf));
	XP_SYS_EQ(sizeof(buf), r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Repeat open-write-close cycle.
 */
DEF(rept_write)
{
	struct timeval start, end, result;
	double res;
	char buf[8000];	/* 1sec in 8bit-mulaw,1ch,8000Hz */
	int fd;
	int r;
	int n;

	TEST("rept_write");

	if (hw_canplay() == 0) {
		XP_SKIP("This test is only for playable device");
		return;
	}

	/* XXX It may timeout on some hardware driver. */
	XP_SKIP("not yet");
	return;

	memset(buf, 0xff, sizeof(buf));
	n = 3;
	gettimeofday(&start, NULL);
	for (int i = 0; i < n; i++) {
		fd = OPEN(devaudio, O_WRONLY);
		REQUIRED_SYS_OK(fd);

		r = WRITE(fd, buf, sizeof(buf));
		XP_SYS_EQ(sizeof(buf), r);

		r = CLOSE(fd);
		XP_SYS_EQ(0, r);
	}
	gettimeofday(&end, NULL);
	timersub(&end, &start, &result);
	res = (double)result.tv_sec + (double)result.tv_usec / 1000000;
	/* Make judgement but not too strict */
	if (res >= n * 1.5) {
		XP_FAIL("expects %d sec but %4.1f sec", n, res);
		return;
	}
}

/*
 * Repeat open-read-close cycle.
 */
DEF(rept_read)
{
	struct timeval start, end, result;
	double res;
	char buf[8000];	/* 1sec in 8bit-mulaw,1ch,8000Hz */
	int fd;
	int r;
	int n;

	TEST("rept_read");

	if (hw_canrec() == 0) {
		XP_SKIP("This test is only for recordable device");
		return;
	}

	/* XXX It may timeout on some hardware driver. */
	XP_SKIP("not yet");
	return;

	n = 3;
	gettimeofday(&start, NULL);
	for (int i = 0; i < n; i++) {
		fd = OPEN(devaudio, O_RDONLY);
		REQUIRED_SYS_OK(fd);

		r = READ(fd, buf, sizeof(buf));
		XP_SYS_EQ(sizeof(buf), r);

		r = CLOSE(fd);
		XP_SYS_EQ(0, r);
	}
	gettimeofday(&end, NULL);
	timersub(&end, &start, &result);
	res = (double)result.tv_sec + (double)result.tv_usec / 1000000;
	/* Make judgement but not too strict */
	if (res >= n * 1.5) {
		XP_FAIL("expects %d sec but %4.1f sec", n, res);
		return;
	}
}

/*
 * Opening with O_RDWR on half-duplex hardware falls back to O_WRONLY.
 * expwrite: expected to be able to play.
 * expread : expected to be able to record.
 */
void
test_rdwr_fallback(int openmode, bool expwrite, bool expread)
{
	struct audio_info ai;
	char buf[10];
	int fd;
	int r;

	TEST("rdwr_fallback_%s", openmode_str[openmode] + 2);

	if (hw_bidir() == 0) {
		XP_SKIP("This test is only for bi-directional device");
		return;
	}

	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	ai.record.pause = 1;

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/* Set pause not to play noise */
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause");
	REQUIRED_SYS_EQ(0, r);

	memset(buf, 0xff, sizeof(buf));
	r = WRITE(fd, buf, sizeof(buf));
	if (expwrite) {
		XP_SYS_EQ(sizeof(buf), r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	r = READ(fd, buf, 0);
	if (expread) {
		XP_SYS_EQ(0, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	r = CLOSE(fd);
	REQUIRED_SYS_EQ(0, r);
}
DEF(rdwr_fallback_RDONLY) { test_rdwr_fallback(O_RDONLY, false, true); }
DEF(rdwr_fallback_WRONLY) { test_rdwr_fallback(O_WRONLY, true, false); }
DEF(rdwr_fallback_RDWR) {
	bool expread;
	/*
	 * On NetBSD7, O_RDWR on half-duplex is accepted. It's possible to
	 * read and write if they don't occur at the same time.
	 * On NetBSD9, O_RDWR on half-duplex falls back O_WRONLY.
	 */
	if (netbsd < 8) {
		expread = true;
	} else {
		expread = hw_fulldup() ? true : false;
	}
	test_rdwr_fallback(O_RDWR, true, expread);
}

/*
 * On full-duplex hardware, the second descriptor's readablity/writability
 * is not depend on the first descriptor('s open mode).
 * On half-duplex hardware, it depends on the first descriptor's open mode.
 */
void
test_rdwr_two(int mode0, int mode1)
{
	struct audio_info ai;
	char wbuf[100];	/* 1/80sec in 8bit-mulaw,1ch,8000Hz */
	char rbuf[100];	/* 1/80sec in 8bit-mulaw,1ch,8000Hz */
	bool canopen;
	bool canwrite;
	bool canread;
	int fd0;
	int fd1;
	int r;
	struct {
		bool canopen;
		bool canwrite;
		bool canread;
	} exptable_full[] = {
	/*	open write read	   1st, 2nd mode */
		{ 1, 0, 1 },	/* REC, REC */
		{ 1, 1, 0 },	/* REC, PLAY */
		{ 1, 1, 1 },	/* REC, BOTH */
		{ 1, 0, 1 },	/* PLAY, REC */
		{ 1, 1, 0 },	/* PLAY, PLAY */
		{ 1, 1, 1 },	/* PLAY, BOTH */
		{ 1, 0, 1 },	/* BOTH, REC */
		{ 1, 1, 0 },	/* BOTH, PLAY */
		{ 1, 1, 1 },	/* BOTH, BOTH */
	},
	exptable_half[] = {
		{ 1, 0, 1 },	/* REC, REC */
		{ 0, 0, 0 },	/* REC, PLAY */
		{ 0, 0, 0 },	/* REC, BOTH */
		{ 0, 0, 0 },	/* PLAY, REC */
		{ 1, 1, 0 },	/* PLAY, PLAY */
		{ 1, 1, 0 },	/* PLAY, BOTH */
		{ 0, 0, 0 },	/* BOTH, REC */
		{ 1, 1, 0 },	/* BOTH, PLAY */
		{ 0, 0, 0 },	/* BOTH, BOTH */
	}, *exptable;

	TEST("rdwr_two_%s_%s",
	    openmode_str[mode0] + 2,
	    openmode_str[mode1] + 2);

	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if (hw_bidir() == 0) {
		XP_SKIP("This test is only for bi-directional device");
		return;
	}

	exptable = hw_fulldup() ? exptable_full : exptable_half;

	canopen  = exptable[mode0 * 3 + mode1].canopen;
	canwrite = exptable[mode0 * 3 + mode1].canwrite;
	canread  = exptable[mode0 * 3 + mode1].canread;

	if (!canopen) {
		XP_SKIP("This combination is not openable on half-duplex");
		return;
	}

	fd0 = OPEN(devaudio, mode0);
	REQUIRED_SYS_OK(fd0);

	fd1 = OPEN(devaudio, mode1);
	REQUIRED_SYS_OK(fd1);

	/* Silent data to make no sound */
	memset(&wbuf, 0xff, sizeof(wbuf));
	/* Pause to make no sound */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd0, AUDIO_SETINFO, &ai, "pause");
	XP_SYS_EQ(0, r);

	/* write(fd1) */
	r = WRITE(fd1, wbuf, sizeof(wbuf));
	if (canwrite) {
		XP_SYS_EQ(100, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	/* read(fd1) */
	r = READ(fd1, rbuf, sizeof(rbuf));
	if (canread) {
		XP_SYS_EQ(100, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);
	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);
}
DEF(rdwr_two_RDONLY_RDONLY)	{ test_rdwr_two(O_RDONLY, O_RDONLY);	}
DEF(rdwr_two_RDONLY_WRONLY)	{ test_rdwr_two(O_RDONLY, O_WRONLY);	}
DEF(rdwr_two_RDONLY_RDWR)	{ test_rdwr_two(O_RDONLY, O_RDWR);	}
DEF(rdwr_two_WRONLY_RDONLY)	{ test_rdwr_two(O_WRONLY, O_RDONLY);	}
DEF(rdwr_two_WRONLY_WRONLY)	{ test_rdwr_two(O_WRONLY, O_WRONLY);	}
DEF(rdwr_two_WRONLY_RDWR)	{ test_rdwr_two(O_WRONLY, O_RDWR);	}
DEF(rdwr_two_RDWR_RDONLY)	{ test_rdwr_two(O_RDWR, O_RDONLY);	}
DEF(rdwr_two_RDWR_WRONLY)	{ test_rdwr_two(O_RDWR, O_WRONLY);	}
DEF(rdwr_two_RDWR_RDWR)		{ test_rdwr_two(O_RDWR, O_RDWR);	}

/*
 * Read and write different descriptors simultaneously.
 * Only on full-duplex.
 */
DEF(rdwr_simul)
{
	char wbuf[1000];	/* 1/8sec in mulaw,1ch,8kHz */
	char rbuf[1000];
	int fd0;
	int fd1;
	int r;
	int status;
	pid_t pid;

	TEST("rdwr_simul");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if (!hw_fulldup()) {
		XP_SKIP("This test is only for full-duplex device");
		return;
	}

	/* Silence data to make no sound */
	memset(wbuf, 0xff, sizeof(wbuf));

	fd0 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd0);
	fd1 = OPEN(devaudio, O_RDONLY);
	REQUIRED_SYS_OK(fd1);

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == -1)
		xp_err(1, __LINE__, "fork");

	if (pid == 0) {
		/* child (read) */
		for (int i = 0; i < 10; i++) {
			r = READ(fd1, rbuf, sizeof(rbuf));
			if (r == -1)
				xp_err(1, __LINE__, "read(i=%d)", i);
		}
		exit(0);
	} else {
		/* parent (write) */
		for (int i = 0; i < 10; i++) {
			r = WRITE(fd0, wbuf, sizeof(wbuf));
			if (r == -1)
				xp_err(1, __LINE__, "write(i=%d)", i);
		}
		waitpid(pid, &status, 0);
	}

	CLOSE(fd0);
	CLOSE(fd1);
	/* If you reach here, consider as success */
	XP_EQ(0, 0);
}

/*
 * DRAIN should work even on incomplete data left.
 */
DEF(drain_incomplete)
{
	struct audio_info ai;
	int r;
	int fd;

	TEST("drain_incomplete");

	if (hw_canplay() == 0) {
		XP_SKIP("This test is only for playable device");
		return;
	}

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	AUDIO_INITINFO(&ai);
	/* let precision > 8 */
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.play.precision = 16;
	ai.mode = AUMODE_PLAY;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	/* Write one byte and then close */
	r = WRITE(fd, &r, 1);
	XP_SYS_EQ(1, r);
	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * DRAIN should work even in pause.
 */
DEF(drain_pause)
{
	struct audio_info ai;
	int r;
	int fd;

	TEST("drain_pause");

	if (hw_canplay() == 0) {
		XP_SKIP("This test is only for playable device");
		return;
	}

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	/* Set pause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);
	/* Write some data and then close */
	r = WRITE(fd, &r, 4);
	XP_SYS_EQ(4, r);
	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * DRAIN does not affect for record-only descriptor.
 */
DEF(drain_onrec)
{
	int fd;
	int r;

	TEST("drain_onrec");

	if (hw_canrec() == 0) {
		XP_SKIP("This test is only for recordable device");
		return;
	}

	fd = OPEN(devaudio, O_RDONLY);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_DRAIN, NULL, "");
	XP_SYS_EQ(0, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Whether mmap() succeeds with specified parameter.
 */
void
test_mmap_mode(int mode, int prot)
{
	char buf[10];
	struct audio_info ai;
	const char *protstr;
	int expected;
	int fd;
	int r;
	int len;
	void *ptr;

	if (prot == PROT_NONE) {
		protstr = "NONE";
	} else if (prot == PROT_READ) {
		protstr = "READ";
	} else if (prot == PROT_WRITE) {
		protstr = "WRITE";
	} else if (prot == (PROT_READ | PROT_WRITE)) {
		protstr = "READWRITE";
	} else {
		xp_errx(1, __LINE__, "unknown prot %x\n", prot);
	}
	TEST("mmap_%s_%s", openmode_str[mode] + 2, protstr);
	if ((props & AUDIO_PROP_MMAP) == 0) {
		XP_SKIP("This test is only for mmap-able device");
		return;
	}
	if (mode2aumode(mode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}
#if !defined(NO_RUMP)
	if (use_rump) {
		XP_SKIP("rump doesn't support mmap");
		return;
	}
#endif

	/*
	 * On NetBSD7 and 8, mmap() always succeeds regardless of open mode.
	 * On NetBSD9, mmap() succeeds only for writable descriptor.
	 */
	expected = mode2play(mode);
	if (netbsd < 9) {
		expected = true;
	}

	fd = OPEN(devaudio, mode);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "get");
	REQUIRED_SYS_EQ(0, r);

	len = ai.play.buffer_size;

	/* Make it pause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause");
	REQUIRED_SYS_EQ(0, r);

	ptr = MMAP(NULL, len, prot, MAP_FILE, fd, 0);
	XP_SYS_PTR(expected ? 0 : EACCES, ptr);
	if (expected) {
		/* XXX Doing mmap(2) doesn't inhibit read(2) */
		if (mode2rec(mode)) {
			r = READ(fd, buf, 0);
			XP_SYS_EQ(0, r);
		}
		/* Doing mmap(2) inhibits write(2) */
		if (mode2play(mode)) {
			/* NetBSD9 changes errno */
			r = WRITE(fd, buf, 0);
			if (netbsd < 9) {
				XP_SYS_NG(EINVAL, r);
			} else {
				XP_SYS_NG(EPERM, r);
			}
		}
	}
	if (ptr != MAP_FAILED) {
		r = MUNMAP(ptr, len);
		XP_SYS_EQ(0, r);
	}

	/* Whether the pause is still valid */
	if (mode2play(mode)) {
		r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
		XP_SYS_EQ(0, r);
		XP_EQ(1, ai.play.pause);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	reset_after_mmap();
}
#define PROT_READWRITE	(PROT_READ | PROT_WRITE)
DEF(mmap_mode_RDONLY_NONE)	{ test_mmap_mode(O_RDONLY, PROT_NONE); }
DEF(mmap_mode_RDONLY_READ)	{ test_mmap_mode(O_RDONLY, PROT_READ); }
DEF(mmap_mode_RDONLY_WRITE)	{ test_mmap_mode(O_RDONLY, PROT_WRITE); }
DEF(mmap_mode_RDONLY_READWRITE)	{ test_mmap_mode(O_RDONLY, PROT_READWRITE); }
DEF(mmap_mode_WRONLY_NONE)	{ test_mmap_mode(O_WRONLY, PROT_NONE); }
DEF(mmap_mode_WRONLY_READ)	{ test_mmap_mode(O_WRONLY, PROT_READ); }
DEF(mmap_mode_WRONLY_WRITE)	{ test_mmap_mode(O_WRONLY, PROT_WRITE); }
DEF(mmap_mode_WRONLY_READWRITE)	{ test_mmap_mode(O_WRONLY, PROT_READWRITE); }
DEF(mmap_mode_RDWR_NONE)	{ test_mmap_mode(O_RDWR, PROT_NONE); }
DEF(mmap_mode_RDWR_READ)	{ test_mmap_mode(O_RDWR, PROT_READ); }
DEF(mmap_mode_RDWR_WRITE)	{ test_mmap_mode(O_RDWR, PROT_WRITE); }
DEF(mmap_mode_RDWR_READWRITE)	{ test_mmap_mode(O_RDWR, PROT_READWRITE); }

/*
 * Check mmap()'s length and offset.
 */
DEF(mmap_len)
{
	struct audio_info ai;
	int fd;
	int r;
	size_t len;
	off_t offset;
	void *ptr;
	int bufsize;
	int pagesize;
	int lsize;

	TEST("mmap_len");
	if ((props & AUDIO_PROP_MMAP) == 0) {
		XP_SKIP("This test is only for mmap-able device");
		return;
	}
#if !defined(NO_RUMP)
	if (use_rump) {
		XP_SKIP("rump doesn't support mmap");
		return;
	}
#endif

	len = sizeof(pagesize);
	r = SYSCTLBYNAME("hw.pagesize", &pagesize, &len, NULL, 0);
	REQUIRED_SYS_EQ(0, r);

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(r);

	/* Get buffer_size */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	bufsize = ai.play.buffer_size;

	/*
	 * XXX someone refers bufsize and another one does pagesize.
	 * I'm not sure.
	 */
	lsize = roundup2(bufsize, pagesize);
	struct {
		size_t len;
		off_t offset;
		int exp;
	} table[] = {
		/* len offset	expected */

		{ 0,	0,	0 },		/* len is 0  */
		{ 1,	0,	0 },		/* len is smaller than lsize */
		{ lsize, 0,	0 },		/* len is the same as lsize */
		{ lsize + 1, 0,	EOVERFLOW },	/* len is larger */

		{ 0, -1,	EINVAL },	/* offset is negative */
		{ 0, lsize,	0 },		/* pointless param but ok */
		{ 0, lsize + 1,	EOVERFLOW },	/* exceed */
		{ 1, lsize,	EOVERFLOW },	/* exceed */

		/*
		 * When you treat offset as 32bit, offset will be 0
		 * and thus it incorrectly succeeds.
		 */
		{ lsize,	1ULL<<32,	EOVERFLOW },
	};

	for (int i = 0; i < (int)__arraycount(table); i++) {
		len = table[i].len;
		offset = table[i].offset;
		int exp = table[i].exp;

		ptr = MMAP(NULL, len, PROT_WRITE, MAP_FILE, fd, offset);
		if (exp == 0) {
			XP_SYS_PTR(0, ptr);
		} else {
			/* NetBSD8 introduces EOVERFLOW */
			if (netbsd < 8 && exp == EOVERFLOW)
				exp = EINVAL;
			XP_SYS_PTR(exp, ptr);
		}

		if (ptr != MAP_FAILED) {
			r = MUNMAP(ptr, len);
			XP_SYS_EQ(0, r);
		}
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	reset_after_mmap();
}

/*
 * mmap() the same descriptor twice.
 */
DEF(mmap_twice)
{
	struct audio_info ai;
	int fd;
	int r;
	int len;
	void *ptr1;
	void *ptr2;

	TEST("mmap_twice");
	if ((props & AUDIO_PROP_MMAP) == 0) {
		XP_SKIP("This test is only for mmap-able device");
		return;
	}
#if !defined(NO_RUMP)
	if (use_rump) {
		XP_SKIP("rump doesn't support mmap");
		return;
	}
#endif

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "get");
	REQUIRED_SYS_EQ(0, r);
	len = ai.play.buffer_size;

	ptr1 = MMAP(NULL, len, PROT_WRITE, MAP_FILE, fd, 0);
	XP_SYS_PTR(0, ptr1);

	/* XXX I'm not sure this sucess is intended.  Anyway I follow it */
	ptr2 = MMAP(NULL, len, PROT_WRITE, MAP_FILE, fd, 0);
	XP_SYS_PTR(0, ptr2);

	if (ptr2 != MAP_FAILED) {
		r = MUNMAP(ptr2, len);
		XP_SYS_EQ(0, r);
	}
	if (ptr1 != MAP_FAILED) {
		r = MUNMAP(ptr1, len);
		XP_SYS_EQ(0, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	reset_after_mmap();
}

/*
 * mmap() different descriptors.
 */
DEF(mmap_multi)
{
	struct audio_info ai;
	int fd0;
	int fd1;
	int r;
	int len;
	void *ptr0;
	void *ptr1;

	TEST("mmap_multi");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if ((props & AUDIO_PROP_MMAP) == 0) {
		XP_SKIP("This test is only for mmap-able device");
		return;
	}
#if !defined(NO_RUMP)
	if (use_rump) {
		XP_SKIP("rump doesn't support mmap");
		return;
	}
#endif

	fd0 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd0);

	r = IOCTL(fd0, AUDIO_GETBUFINFO, &ai, "get");
	REQUIRED_SYS_EQ(0, r);
	len = ai.play.buffer_size;

	fd1 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd1);

	ptr0 = MMAP(NULL, len, PROT_WRITE, MAP_FILE, fd0, 0);
	XP_SYS_PTR(0, ptr0);

	ptr1 = MMAP(NULL, len,  PROT_WRITE, MAP_FILE, fd1, 0);
	XP_SYS_PTR(0, ptr1);

	if (ptr0 != MAP_FAILED) {
		r = MUNMAP(ptr1, len);
		XP_SYS_EQ(0, r);
	}

	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);

	if (ptr1 != MAP_FAILED) {
		r = MUNMAP(ptr0, len);
		XP_SYS_EQ(0, r);
	}

	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);

	reset_after_mmap();
}

#define IN	POLLIN
#define OUT	POLLOUT
/*
 * Whether poll() succeeds with specified mode.
 */
void
test_poll_mode(int mode, int events, int expected_revents)
{
	struct pollfd pfd;
	const char *events_str;
	int fd;
	int r;
	int expected_r;

	if (events == IN) {
		events_str = "IN";
	} else if (events == OUT) {
		events_str = "OUT";
	} else if (events == (IN | OUT)) {
		events_str = "INOUT";
	} else {
		events_str = "?";
	}
	TEST("poll_mode_%s_%s", openmode_str[mode] + 2, events_str);
	if (mode2aumode(mode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	expected_r = (expected_revents != 0) ? 1 : 0;

	fd = OPEN(devaudio, mode);
	REQUIRED_SYS_OK(fd);

	/* Wait a bit to be recorded. */
	usleep(100 * 1000);

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = events;

	r = POLL(&pfd, 1, 100);
	/* It's a bit complicated..  */
	if (r < 0 || r > 1) {
		/*
		 * Check these two cases first:
		 * - system call fails.
		 * - poll() with one nfds returns >1.  It's strange.
		 */
		XP_SYS_EQ(expected_r, r);
	} else {
		/*
		 * Otherwise, poll() returned 0 or 1.
		 */
		DPRINTF("  > pfd.revents=%s\n", event_tostr(pfd.revents));

		/* NetBSD7,8 have several strange behavior.  It must be bug. */

		XP_SYS_EQ(expected_r, r);
		XP_EQ(expected_revents, pfd.revents);
	}
	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(poll_mode_RDONLY_IN)	{ test_poll_mode(O_RDONLY, IN,     IN); }
DEF(poll_mode_RDONLY_OUT)	{ test_poll_mode(O_RDONLY, OUT,    0); }
DEF(poll_mode_RDONLY_INOUT)	{ test_poll_mode(O_RDONLY, IN|OUT, IN); }
DEF(poll_mode_WRONLY_IN)	{ test_poll_mode(O_WRONLY, IN,     0); }
DEF(poll_mode_WRONLY_OUT)	{ test_poll_mode(O_WRONLY, OUT,	   OUT); }
DEF(poll_mode_WRONLY_INOUT)	{ test_poll_mode(O_WRONLY, IN|OUT, OUT); }
DEF(poll_mode_RDWR_IN)		{
	/* On half-duplex, O_RDWR is the same as O_WRONLY. */
	if (hw_fulldup()) test_poll_mode(O_RDWR,   IN,     IN);
	else		  test_poll_mode(O_RDWR,   IN,     0);
}
DEF(poll_mode_RDWR_OUT)		{ test_poll_mode(O_RDWR,   OUT,	   OUT); }
DEF(poll_mode_RDWR_INOUT)	{
	/* On half-duplex, O_RDWR is the same as O_WRONLY. */
	if (hw_fulldup()) test_poll_mode(O_RDWR,   IN|OUT, IN|OUT);
	else		  test_poll_mode(O_RDWR,   IN|OUT,    OUT);
}

/*
 * Poll(OUT) when buffer is empty.
 */
DEF(poll_out_empty)
{
	struct pollfd pfd;
	int fd;
	int r;

	TEST("poll_out_empty");

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLOUT;

	/* Check when empty.  It should succeed even if timeout == 0 */
	r = POLL(&pfd, 1, 0);
	XP_SYS_EQ(1, r);
	XP_EQ(POLLOUT, pfd.revents);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Poll(OUT) when buffer is full.
 */
DEF(poll_out_full)
{
	struct audio_info ai;
	struct pollfd pfd;
	int fd;
	int r;
	char *buf;
	int buflen;

	TEST("poll_out_full");

	fd = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd);

	/* Pause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Get buffer size */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Write until full */
	buflen = ai.play.buffer_size;
	buf = (char *)malloc(buflen);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buflen);
	do {
		r = WRITE(fd, buf, buflen);
	} while (r == buflen);
	if (r == -1) {
		XP_SYS_NG(EAGAIN, r);
	}

	/* Do poll */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLOUT;
	r = POLL(&pfd, 1, 0);
	XP_SYS_EQ(0, r);
	XP_EQ(0, pfd.revents);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * Poll(OUT) when buffer is full but hiwat sets lower than full.
 */
DEF(poll_out_hiwat)
{
	struct audio_info ai;
	struct pollfd pfd;
	int fd;
	int r;
	char *buf;
	int buflen;
	int newhiwat;

	TEST("poll_out_hiwat");

	fd = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd);

	/* Get buffer size and hiwat */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	/* Change hiwat some different value */
	newhiwat = ai.lowat;

	/* Set pause and hiwat */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	ai.hiwat = newhiwat;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=1;hiwat");
	XP_SYS_EQ(0, r);

	/* Get the set hiwat again */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Write until full */
	buflen = ai.blocksize * ai.hiwat;
	buf = (char *)malloc(buflen);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buflen);
	do {
		r = WRITE(fd, buf, buflen);
	} while (r == buflen);
	if (r == -1) {
		XP_SYS_NG(EAGAIN, r);
	}

	/* Do poll */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLOUT;
	r = POLL(&pfd, 1, 0);
	XP_SYS_EQ(0, r);
	XP_EQ(0, pfd.revents);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * Unpause from buffer full, POLLOUT should raise.
 * XXX poll(2) on NetBSD7 is really incomplete and weird.  I don't test it.
 */
DEF(poll_out_unpause)
{
	struct audio_info ai;
	struct pollfd pfd;
	int fd;
	int r;
	char *buf;
	int buflen;
	u_int blocksize;
	int hiwat;
	int lowat;

	TEST("poll_out_unpause");
	if (netbsd < 8) {
		XP_SKIP("NetBSD7's poll() is too incomplete to test.");
		return;
	}

	/* Non-blocking open */
	fd = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd);

	/* Adjust block size and hiwat/lowat to make the test time 1sec */
	blocksize = 1000;	/* 1/8 sec in mulaw,1ch,8000Hz */
	hiwat = 12;		/* 1.5sec */
	lowat = 4;		/* 0.5sec */
	AUDIO_INITINFO(&ai);
	ai.blocksize = blocksize;
	ai.hiwat = hiwat;
	ai.lowat = lowat;
	/* and also set encoding */
	/*
	 * XXX NetBSD7 has different results depending on whether the input
	 * encoding is emulated (AUDIO_ENCODINGFLAG_EMULATED) or not.  It's
	 * not easy to ensure this situation on all hardware environment.
	 * On NetBSD9, the result is the same regardless of input encoding.
	 */
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "blocksize=%d", blocksize);
	XP_SYS_EQ(0, r);
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	if (ai.blocksize != blocksize) {
		/*
		 * NetBSD9 can not change the blocksize.  Then,
		 * adjust using hiwat/lowat.
		 */
		blocksize = ai.blocksize;
		hiwat = howmany(8000 * 1.5, blocksize);
		lowat = howmany(8000 * 0.5, blocksize);
	}
	/* Anyway, set the parameters */
	AUDIO_INITINFO(&ai);
	ai.blocksize = blocksize;
	ai.hiwat = hiwat;
	ai.lowat = lowat;
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=1");
	XP_SYS_EQ(0, r);

	/* Get the set parameters again */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Write until full */
	buflen = ai.blocksize * ai.hiwat;
	buf = (char *)malloc(buflen);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buflen);
	do {
		r = WRITE(fd, buf, buflen);
	} while (r == buflen);
	if (r == -1) {
		XP_SYS_NG(EAGAIN, r);
	}

	/* At this time, POLLOUT should not be set because buffer is full */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLOUT;
	r = POLL(&pfd, 1, 0);
	XP_SYS_EQ(0, r);
	XP_EQ(0, pfd.revents);

	/* Unpause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 0;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=0");
	XP_SYS_EQ(0, r);

	/*
	 * When unpause occurs:
	 * - NetBSD7 (emul=0) -> the buffer remains.
	 * - NetBSD7 (emul=1) -> the buffer is cleared.
	 * - NetBSD8          -> the buffer remains.
	 * - NetBSD9          -> the buffer remains.
	 */

	/* Check poll() up to 2sec */
	pfd.revents = 0;
	r = POLL(&pfd, 1, 2000);
	XP_SYS_EQ(1, r);
	XP_EQ(POLLOUT, pfd.revents);

	/*
	 * Since POLLOUT is set, it should be writable.
	 * But at this time, no all buffer may be writable.
	 */
	r = WRITE(fd, buf, buflen);
	XP_SYS_OK(r);

	/* Flush it because there is no need to play it */
	r = IOCTL(fd, AUDIO_FLUSH, NULL, "");
	XP_SYS_EQ(0, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * poll(2) must not be affected by playback of other descriptors.
 */
DEF(poll_out_simul)
{
	struct audio_info ai;
	struct pollfd pfd[2];
	int fd[2];
	int r;
	char *buf;
	u_int blocksize;
	int hiwat;
	int lowat;
	int buflen;
	int time;

	TEST("poll_out_simul");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}

	/* Make sure that it's not affected by descriptor order */
	for (int i = 0; i < 2; i++) {
		int a = i;
		int b = 1 - i;

		fd[0] = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
		REQUIRED_SYS_OK(fd[0]);
		fd[1] = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
		REQUIRED_SYS_OK(fd[1]);

		/*
		 * Adjust block size and hiwat/lowat.
		 * I want to choice suitable blocksize (if possible).
		 */
		blocksize = 1000;	/* 1/8 sec in mulaw,1ch,8000Hz */
		hiwat = 12;		/* 1.5sec */
		lowat = 4;		/* 0.5sec */
		AUDIO_INITINFO(&ai);
		ai.blocksize = blocksize;
		ai.hiwat = hiwat;
		ai.lowat = lowat;
		r = IOCTL(fd[0], AUDIO_SETINFO, &ai, "blocksize=1000");
		XP_SYS_EQ(0, r);
		r = IOCTL(fd[0], AUDIO_GETBUFINFO, &ai, "read back blocksize");
		if (ai.blocksize != blocksize) {
			/*
			 * NetBSD9 can not change the blocksize.  Then,
			 * adjust using hiwat/lowat.
			 */
			blocksize = ai.blocksize;
			hiwat = howmany(8000 * 1.5, blocksize);
			lowat = howmany(8000 * 0.5, blocksize);
		}
		/* Anyway, set the parameters */
		AUDIO_INITINFO(&ai);
		ai.blocksize = blocksize;
		ai.hiwat = hiwat;
		ai.lowat = lowat;
		/* Pause fdA */
		ai.play.pause = 1;
		r = IOCTL(fd[a], AUDIO_SETINFO, &ai, "pause=1");
		XP_SYS_EQ(0, r);
		/* Unpause fdB */
		ai.play.pause = 0;
		r = IOCTL(fd[b], AUDIO_SETINFO, &ai, "pause=0");
		XP_SYS_EQ(0, r);

		/* Get again. XXX two individual ioctls are correct */
		r = IOCTL(fd[0], AUDIO_GETBUFINFO, &ai, "");
		XP_SYS_EQ(0, r);
		DPRINTF("  > blocksize=%d lowat=%d hiwat=%d\n",
			ai.blocksize, ai.lowat, ai.hiwat);

		/* Enough long time than the playback time */
		time = (ai.hiwat - ai.lowat) * blocksize / 8;  /*[msec]*/
		time *= 2;

		/* Write fdA full */
		buflen = blocksize * ai.lowat;
		buf = (char *)malloc(buflen);
		REQUIRED_IF(buf != NULL);
		memset(buf, 0xff, buflen);
		do {
			r = WRITE(fd[a], buf, buflen);
		} while (r == buflen);
		if (r == -1) {
			XP_SYS_NG(EAGAIN, r);
		}

		/* POLLOUT should not be set, because fdA is buffer full */
		memset(pfd, 0, sizeof(pfd));
		pfd[0].fd = fd[a];
		pfd[0].events = POLLOUT;
		r = POLL(pfd, 1, 0);
		XP_SYS_EQ(0, r);
		XP_EQ(0, pfd[0].revents);

		/* Write fdB at least lowat */
		r = WRITE(fd[b], buf, buflen);
		XP_SYS_EQ(buflen, r);
		r = WRITE(fd[b], buf, buflen);
		if (r == -1) {
			XP_SYS_NG(EAGAIN, r);
		}

		/* Only fdB should become POLLOUT */
		memset(pfd, 0, sizeof(pfd));
		pfd[0].fd = fd[0];
		pfd[0].events = POLLOUT;
		pfd[1].fd = fd[1];
		pfd[1].events = POLLOUT;
		r = POLL(pfd, 2, time);
		XP_SYS_EQ(1, r);
		if (r != -1) {
			XP_EQ(0, pfd[a].revents);
			XP_EQ(POLLOUT, pfd[b].revents);
		}

		/* Drop the rest */
		r = IOCTL(fd[0], AUDIO_FLUSH, NULL, "");
		XP_SYS_EQ(0, r);
		r = IOCTL(fd[1], AUDIO_FLUSH, NULL, "");
		XP_SYS_EQ(0, r);

		r = CLOSE(fd[0]);
		XP_SYS_EQ(0, r);
		r = CLOSE(fd[1]);
		XP_SYS_EQ(0, r);
		free(buf);

		xxx_close_wait();
	}
}

/*
 * Open with READ mode starts recording immediately.
 * Of course, audioctl doesn't start.
 */
void
test_poll_in_open(const char *devname)
{
	struct audio_info ai;
	struct pollfd pfd;
	char buf[4096];
	char devfile[16];
	int fd;
	int r;
	bool is_audioctl;

	TEST("poll_in_open_%s", devname);
	if (hw_canrec() == 0) {
		XP_SKIP("This test is only for recordable device");
		return;
	}

	snprintf(devfile, sizeof(devfile), "/dev/%s%d", devname, unit);
	is_audioctl = (strcmp(devname, "audioctl") == 0);

	fd = OPEN(devfile, O_RDONLY);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	if (is_audioctl) {
		/* opening /dev/audioctl doesn't start recording. */
		XP_EQ(0, ai.record.active);
	} else {
		/* opening /dev/{audio,sound} starts recording. */
		/*
		 * On NetBSD7/8, opening /dev/sound doesn't start recording.
		 * It must be a bug.
		 */
		XP_EQ(1, ai.record.active);
	}

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLIN;
	r = POLL(&pfd, 1, 1000);
	if (is_audioctl) {
		/*
		 * poll-ing /dev/audioctl always fails.
		 * XXX Returning error instead of timeout should be better(?).
		 */
		REQUIRED_SYS_EQ(0, r);
	} else {
		/*
		 * poll-ing /dev/{audio,sound} will succeed when recorded
		 * data is arrived.
		 */
		/*
		 * On NetBSD7/8, opening /dev/sound doesn't start recording.
		 * It must be a bug.
		 */
		REQUIRED_SYS_EQ(1, r);

		/* In this case, read() should succeed. */
		r = READ(fd, buf, sizeof(buf));
		XP_SYS_OK(r);
		XP_NE(0, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(poll_in_open_audio)		{ test_poll_in_open("audio"); }
DEF(poll_in_open_sound)		{ test_poll_in_open("sound"); }
DEF(poll_in_open_audioctl)	{ test_poll_in_open("audioctl"); }

/*
 * poll(2) must not be affected by other recording descriptors even if
 * playback descriptor waits with POLLIN (though it's not normal usage).
 * In other words, two POLLIN must not interfere.
 */
DEF(poll_in_simul)
{
	struct audio_info ai;
	struct pollfd pfd;
	int fd[2];
	int r;
	char *buf;
	int blocksize;

	TEST("poll_in_simul");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if (hw_fulldup() == 0) {
		XP_SKIP("This test is only for full-duplex device");
		return;
	}

	int play = 0;
	int rec = 1;

	fd[play] = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd[play]);
	fd[rec] = OPEN(devaudio, O_RDONLY);
	REQUIRED_SYS_OK(fd[rec]);

	/* Get block size */
	r = IOCTL(fd[rec], AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	blocksize = ai.blocksize;

	buf = (char *)malloc(blocksize);
	REQUIRED_IF(buf != NULL);

	/*
	 * At first, make sure the playback one doesn't return POLLIN.
	 */
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd[play];
	pfd.events = POLLIN;
	r = POLL(&pfd, 1, 0);
	if (r == 0 && pfd.revents == 0) {
		XP_SYS_EQ(0, r);
		XP_EQ(0, pfd.revents);
	} else {
		XP_FAIL("play fd returns POLLIN");
		goto abort;
	}

	/* Start recording */
	r = READ(fd[rec], buf, blocksize);
	XP_SYS_EQ(blocksize, r);

	/* Poll()ing playback descriptor with POLLIN should not raise */
	r = POLL(&pfd, 1, 1000);
	XP_SYS_EQ(0, r);
	XP_EQ(0, pfd.revents);

	/* Poll()ing recording descriptor with POLLIN should raise */
	pfd.fd = fd[rec];
	r = POLL(&pfd, 1, 0);
	XP_SYS_EQ(1, r);
	XP_EQ(POLLIN, pfd.revents);

abort:
	r = CLOSE(fd[play]);
	XP_SYS_EQ(0, r);
	r = CLOSE(fd[rec]);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * Whether kqueue() succeeds with specified mode.
 */
void
test_kqueue_mode(int openmode, int filt, int expected)
{
	struct kevent kev;
	struct timespec ts;
	int fd;
	int kq;
	int r;

	TEST("kqueue_mode_%s_%s",
	    openmode_str[openmode] + 2,
	    (filt == EVFILT_READ) ? "READ" : "WRITE");
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	ts.tv_sec = 0;
	ts.tv_nsec = 100 * 1000 * 1000;	// 100msec

	kq = KQUEUE();
	XP_SYS_OK(kq);

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/*
	 * Check whether the specified filter can be set.
	 * Any filters can always be set, even if pointless combination.
	 * For example, EVFILT_READ can be set on O_WRONLY descriptor
	 * though it will never raise.
	 * I will not mention about good or bad of this behavior here.
	 */
	EV_SET(&kev, fd, filt, EV_ADD, 0, 0, 0);
	r = KEVENT_SET(kq, &kev, 1);
	XP_SYS_EQ(0, r);

	if (r == 0) {
		/* If the filter can be set, try kevent(poll) */
		r = KEVENT_POLL(kq, &kev, 1, &ts);
		XP_SYS_EQ(expected, r);

		/* Delete it */
		EV_SET(&kev, fd, filt, EV_DELETE, 0, 0, 0);
		r = KEVENT_SET(kq, &kev, 1);
		XP_SYS_EQ(0, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	r = CLOSE(kq);
	XP_SYS_EQ(0, r);
}
DEF(kqueue_mode_RDONLY_READ) {
	/* Should raise */
	test_kqueue_mode(O_RDONLY, EVFILT_READ, 1);
}
DEF(kqueue_mode_RDONLY_WRITE) {
	/* Should never raise (NetBSD7 has bugs) */
	int expected = (netbsd < 8) ? 1 : 0;
	test_kqueue_mode(O_RDONLY, EVFILT_WRITE, expected);
}
DEF(kqueue_mode_WRONLY_READ) {
	/* Should never raise */
	test_kqueue_mode(O_WRONLY, EVFILT_READ, 0);
}
DEF(kqueue_mode_WRONLY_WRITE) {
	/* Should raise */
	test_kqueue_mode(O_WRONLY, EVFILT_WRITE, 1);
}
DEF(kqueue_mode_RDWR_READ) {
	/* Should raise on fulldup but not on halfdup, on NetBSD9 */
	int expected = hw_fulldup() ? 1 : 0;
	test_kqueue_mode(O_RDWR, EVFILT_READ, expected);
}
DEF(kqueue_mode_RDWR_WRITE) {
	/* Should raise */
	test_kqueue_mode(O_RDWR, EVFILT_WRITE, 1);
}

/*
 * kqueue(2) when buffer is empty.
 */
DEF(kqueue_empty)
{
	struct audio_info ai;
	struct kevent kev;
	struct timespec ts;
	int kq;
	int fd;
	int r;

	TEST("kqueue_empty");

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	kq = KQUEUE();
	XP_SYS_OK(kq);

	EV_SET(&kev, fd, EV_ADD, EVFILT_WRITE, 0, 0, 0);
	r = KEVENT_SET(kq, &kev, 1);
	XP_SYS_EQ(0, r);

	/* When the buffer is empty, it should succeed even if timeout == 0 */
	memset(&ts, 0, sizeof(ts));
	r = KEVENT_POLL(kq, &kev, 1, &ts);
	XP_SYS_EQ(1, r);
	XP_EQ(fd, kev.ident);
	/*
	 * XXX According to kqueue(2) manpage, returned kev.data contains
	 * "the amount of space remaining in the write buffer".
	 * NetBSD7 returns buffer_size.  Shouldn't it be blocksize * hiwat?
	 */
	/* XP_EQ(ai.blocksize * ai.hiwat, kev.data); */
	XP_EQ(ai.play.buffer_size, kev.data);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	r = CLOSE(kq);
	XP_SYS_EQ(0, r);
}

/*
 * kqueue(2) when buffer is full.
 */
DEF(kqueue_full)
{
	struct audio_info ai;
	struct kevent kev;
	struct timespec ts;
	int kq;
	int fd;
	int r;
	char *buf;
	int buflen;

	TEST("kqueue_full");

	fd = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd);

	/* Pause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Get buffer size */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Write until full */
	buflen = ai.play.buffer_size;
	buf = (char *)malloc(buflen);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buflen);
	do {
		r = WRITE(fd, buf, buflen);
	} while (r == buflen);
	if (r == -1) {
		XP_SYS_NG(EAGAIN, r);
	}

	kq = KQUEUE();
	XP_SYS_OK(kq);

	EV_SET(&kev, fd, EV_ADD, EVFILT_WRITE, 0, 0, 0);
	r = KEVENT_SET(kq, &kev, 1);
	XP_SYS_EQ(0, r);

	/* kevent() should not raise */
	ts.tv_sec = 0;
	ts.tv_nsec = 100L * 1000 * 1000;	/* 100msec */
	r = KEVENT_POLL(kq, &kev, 1, &ts);
	XP_SYS_EQ(0, r);
	if (r > 0) {
		XP_EQ(fd, kev.ident);
		XP_EQ(0, kev.data);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	r = CLOSE(kq);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * kqueue(2) when buffer is full but hiwat sets lower than full.
 */
DEF(kqueue_hiwat)
{
	struct audio_info ai;
	struct kevent kev;
	struct timespec ts;
	int kq;
	int fd;
	int r;
	char *buf;
	int buflen;
	int newhiwat;

	TEST("kqueue_hiwat");

	fd = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd);

	/* Get buffer size and hiwat */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "hiwat");
	XP_SYS_EQ(0, r);
	/* Change hiwat some different value */
	newhiwat = ai.hiwat - 1;

	/* Set pause and hiwat */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	ai.hiwat = newhiwat;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=1;hiwat");
	XP_SYS_EQ(0, r);

	/* Get the set parameters again */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(1, ai.play.pause);
	XP_EQ(newhiwat, ai.hiwat);

	/* Write until full */
	buflen = ai.blocksize * ai.hiwat;
	buf = (char *)malloc(buflen);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buflen);
	do {
		r = WRITE(fd, buf, buflen);
	} while (r == buflen);
	if (r == -1) {
		XP_SYS_NG(EAGAIN, r);
	}

	kq = KQUEUE();
	XP_SYS_OK(kq);

	EV_SET(&kev, fd, EV_ADD, EVFILT_WRITE, 0, 0, 0);
	r = KEVENT_SET(kq, &kev, 1);
	XP_SYS_EQ(0, r);

	/* Should not raise because it's not possible to write */
	ts.tv_sec = 0;
	ts.tv_nsec = 100L * 1000 * 1000;	/* 100msec */
	r = KEVENT_POLL(kq, &kev, 1, &ts);
	if (r > 0)
		DEBUG_KEV("kev", &kev);
	XP_SYS_EQ(0, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	r = CLOSE(kq);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * Unpause from buffer full, kevent() should raise.
 */
DEF(kqueue_unpause)
{
	struct audio_info ai;
	struct kevent kev;
	struct timespec ts;
	int fd;
	int r;
	int kq;
	char *buf;
	int buflen;
	u_int blocksize;
	int hiwat;
	int lowat;

	TEST("kqueue_unpause");

	/* Non-blocking open */
	fd = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
	REQUIRED_SYS_OK(fd);

	/* Adjust block size and hiwat/lowat to make the test time 1sec */
	blocksize = 1000;	/* 1/8 sec in mulaw,1ch,8000Hz */
	hiwat = 12;		/* 1.5sec */
	lowat = 4;		/* 0.5sec */
	AUDIO_INITINFO(&ai);
	ai.blocksize = blocksize;
	ai.hiwat = hiwat;
	ai.lowat = lowat;
	/* and also set encoding */
	/*
	 * XXX NetBSD7 has different results depending on whether the input
	 * encoding is emulated (AUDIO_ENCODINGFLAG_EMULATED) or not.  It's
	 * not easy to ensure this situation on all hardware environment.
	 * On NetBSD9, the result is the same regardless of input encoding.
	 */
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "blocksize=%d", blocksize);
	XP_SYS_EQ(0, r);
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	if (ai.blocksize != blocksize) {
		/*
		 * NetBSD9 can not change the blocksize.  Then,
		 * adjust using hiwat/lowat.
		 */
		blocksize = ai.blocksize;
		hiwat = howmany(8000 * 1.5, blocksize);
		lowat = howmany(8000 * 0.5, blocksize);
	}
	/* Anyway, set the parameters */
	AUDIO_INITINFO(&ai);
	ai.blocksize = blocksize;
	ai.hiwat = hiwat;
	ai.lowat = lowat;
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=1");
	XP_SYS_EQ(0, r);

	/* Get the set parameters again */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	DPRINTF("  > blocksize=%d hiwat=%d lowat=%d buffer_size=%d\n",
	    ai.blocksize, ai.hiwat, ai.lowat, ai.play.buffer_size);

	/* Write until full */
	buflen = ai.blocksize * ai.hiwat;
	buf = (char *)malloc(buflen);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buflen);
	do {
		r = WRITE(fd, buf, buflen);
	} while (r == buflen);
	if (r == -1) {
		XP_SYS_NG(EAGAIN, r);
	}

	kq = KQUEUE();
	XP_SYS_OK(kq);

	EV_SET(&kev, fd, EV_ADD, EVFILT_WRITE, 0, 0, 0);
	r = KEVENT_SET(kq, &kev, 1);
	XP_SYS_EQ(0, r);

	/* Unpause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 0;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=0");
	XP_SYS_EQ(0, r);

	/* Check kevent() up to 2sec */
	ts.tv_sec = 2;
	ts.tv_nsec = 0;
	r = KEVENT_POLL(kq, &kev, 1, &ts);
	if (r >= 1)
		DEBUG_KEV("kev", &kev);
	if (netbsd < 8) {
		/*
		 * NetBSD7 with EMULATED_FLAG unset has bugs.  Unpausing
		 * unintentionally clears buffer (and therefore it becomes
		 * writable) but it doesn't raise EVFILT_WRITE.
		 */
	} else {
		XP_SYS_EQ(1, r);
	}

	/* Flush it because there is no need to play it */
	r = IOCTL(fd, AUDIO_FLUSH, NULL, "");
	XP_SYS_EQ(0, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	r = CLOSE(kq);
	XP_SYS_EQ(0, r);
	free(buf);
}

/*
 * kevent(2) must not be affected by other audio descriptors.
 */
DEF(kqueue_simul)
{
	struct audio_info ai;
	struct audio_info ai2;
	struct kevent kev[2];
	struct timespec ts;
	int fd[2];
	int r;
	int kq;
	u_int blocksize;
	int hiwat;
	int lowat;
	char *buf;
	int buflen;

	TEST("kqueue_simul");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}

	memset(&ts, 0, sizeof(ts));

	/* Make sure that it's not affected by descriptor order */
	for (int i = 0; i < 2; i++) {
		int a = i;
		int b = 1 - i;

		fd[0] = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
		REQUIRED_SYS_OK(fd[0]);
		fd[1] = OPEN(devaudio, O_WRONLY | O_NONBLOCK);
		REQUIRED_SYS_OK(fd[1]);

		/*
		 * Adjust block size and hiwat/lowat.
		 * I want to choice suitable blocksize (if possible).
		 */
		blocksize = 1000;	/* 1/8 sec in mulaw,1ch,8000Hz */
		hiwat = 12;		/* 1.5sec */
		lowat = 4;		/* 0.5sec */
		AUDIO_INITINFO(&ai);
		ai.blocksize = blocksize;
		ai.hiwat = hiwat;
		ai.lowat = lowat;
		r = IOCTL(fd[0], AUDIO_SETINFO, &ai, "blocksize=1000");
		XP_SYS_EQ(0, r);
		r = IOCTL(fd[0], AUDIO_GETBUFINFO, &ai, "read back blocksize");
		if (ai.blocksize != blocksize) {
			/*
			 * NetBSD9 can not change the blocksize.  Then,
			 * adjust using hiwat/lowat.
			 */
			blocksize = ai.blocksize;
			hiwat = howmany(8000 * 1.5, blocksize);
			lowat = howmany(8000 * 0.5, blocksize);
		}
		/* Anyway, set the parameters to both */
		AUDIO_INITINFO(&ai);
		ai.blocksize = blocksize;
		ai.hiwat = hiwat;
		ai.lowat = lowat;
		ai.play.pause = 1;
		r = IOCTL(fd[a], AUDIO_SETINFO, &ai, "pause=1");
		XP_SYS_EQ(0, r);
		r = IOCTL(fd[b], AUDIO_SETINFO, &ai, "pause=1");
		XP_SYS_EQ(0, r);

		/* Write both until full */
		buflen = ai.blocksize * ai.hiwat;
		buf = (char *)malloc(buflen);
		REQUIRED_IF(buf != NULL);
		memset(buf, 0xff, buflen);
		/* Write fdA */
		do {
			r = WRITE(fd[a], buf, buflen);
		} while (r == buflen);
		if (r == -1) {
			XP_SYS_NG(EAGAIN, r);
		}
		/* Write fdB */
		do {
			r = WRITE(fd[b], buf, buflen);
		} while (r == buflen);
		if (r == -1) {
			XP_SYS_NG(EAGAIN, r);
		}

		/* Get fdB's initial seek for later */
		r = IOCTL(fd[b], AUDIO_GETBUFINFO, &ai2, "");
		XP_SYS_EQ(0, r);

		kq = KQUEUE();
		XP_SYS_OK(kq);

		/* Both aren't raised at this point */
		EV_SET(&kev[0], fd[a], EV_ADD, EVFILT_WRITE, 0, 0, 0);
		EV_SET(&kev[1], fd[b], EV_ADD, EVFILT_WRITE, 0, 0, 0);
		r = KEVENT_SET(kq, kev, 2);
		XP_SYS_EQ(0, r);

		/* Unpause only fdA */
		AUDIO_INITINFO(&ai);
		ai.play.pause = 0;
		r = IOCTL(fd[a], AUDIO_SETINFO, &ai, "pause=0");
		XP_SYS_EQ(0, r);

		/* kevent() up to 2sec */
		ts.tv_sec = 2;
		ts.tv_nsec = 0;
		r = KEVENT_POLL(kq, &kev[0], 1, &ts);
		if (r >= 1)
			DEBUG_KEV("kev", &kev[0]);
		/* fdA should raise */
		XP_SYS_EQ(1, r);
		XP_EQ(fd[a], kev[0].ident);

		/* Make sure that fdB keeps whole data */
		r = IOCTL(fd[b], AUDIO_GETBUFINFO, &ai, "");
		XP_EQ(ai2.play.seek, ai.play.seek);

		/* Flush it because there is no need to play it */
		r = IOCTL(fd[0], AUDIO_FLUSH, NULL, "");
		XP_SYS_EQ(0, r);
		r = IOCTL(fd[1], AUDIO_FLUSH, NULL, "");
		XP_SYS_EQ(0, r);

		r = CLOSE(fd[0]);
		XP_SYS_EQ(0, r);
		r = CLOSE(fd[1]);
		XP_SYS_EQ(0, r);
		r = CLOSE(kq);
		XP_SYS_EQ(0, r);
		free(buf);

		xxx_close_wait();
	}
}

/* Shared data between threads for ioctl_while_write */
struct ioctl_while_write_data {
	int fd;
	struct timeval start;
	int terminated;
};

/* Test thread for ioctl_while_write */
void *thread_ioctl_while_write(void *);
void *
thread_ioctl_while_write(void *arg)
{
	struct ioctl_while_write_data *data = arg;
	struct timeval now, res;
	struct audio_info ai;
	int r;

	/* If 0.5 seconds have elapsed since writing, assume it's blocked */
	do {
		usleep(100);
		gettimeofday(&now, NULL);
		timersub(&now, &data->start, &res);
	} while (res.tv_usec < 500000);

	/* Then, do ioctl() */
	r = IOCTL(data->fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/* Terminate */
	data->terminated = 1;

	/* Resume write() by unpause */
	AUDIO_INITINFO(&ai);
	if (netbsd < 8) {
		/*
		 * XXX NetBSD7 has bugs and it cannot be unpaused.
		 * However, it also has another bug and it clears buffer
		 * when encoding is changed.  I use it. :-P
		 */
		ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	}
	ai.play.pause = 0;
	r = IOCTL(data->fd, AUDIO_SETINFO, &ai, "pause=0");
	XP_SYS_EQ(0, r);

	return NULL;
}

/*
 * ioctl(2) can be issued while write(2)-ing.
 */
DEF(ioctl_while_write)
{
	struct audio_info ai;
	struct ioctl_while_write_data data0, *data;
	char buf[8000];	/* 1sec in mulaw,1ch,8000Hz */
	pthread_t tid;
	int r;

	TEST("ioctl_while_write");

	data = &data0;
	memset(data, 0, sizeof(*data));
	memset(buf, 0xff, sizeof(buf));

	data->fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(data->fd);

	/* Pause to block write(2)ing */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(data->fd, AUDIO_SETINFO, &ai, "pause=1");
	XP_SYS_EQ(0, r);

	gettimeofday(&data->start, NULL);

	pthread_create(&tid, NULL, thread_ioctl_while_write, data);

	/* Write until blocking */
	for (;;) {
		r = WRITE(data->fd, buf, sizeof(buf));
		if (data->terminated)
			break;
		XP_SYS_EQ(sizeof(buf), r);

		/* Update written time */
		gettimeofday(&data->start, NULL);
	}

	pthread_join(tid, NULL);

	/* Flush */
	r = IOCTL(data->fd, AUDIO_FLUSH, NULL, "");
	XP_SYS_EQ(0, r);
	r = CLOSE(data->fd);
	XP_SYS_EQ(0, r);
}

volatile int sigio_caught;
void
signal_FIOASYNC(int signo)
{
	if (signo == SIGIO) {
		sigio_caught = 1;
		DPRINTF("  > %d: pid %d got SIGIO\n", __LINE__, (int)getpid());
	}
}

/*
 * FIOASYNC between two descriptors should be split.
 */
DEF(FIOASYNC_reset)
{
	int fd0, fd1;
	int r;
	int val;

	TEST("FIOASYNC_reset");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}

	/* The first one opens */
	fd0 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd0);

	/* The second one opens, enables ASYNC, and closes */
	fd1 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd1);
	val = 1;
	r = IOCTL(fd1, FIOASYNC, &val, "on");
	XP_SYS_EQ(0, r);
	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);

	/* Again, the second one opens and enables ASYNC */
	fd1 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd1);
	val = 1;
	r = IOCTL(fd1, FIOASYNC, &val, "on");
	XP_SYS_EQ(0, r);	/* NetBSD8 fails */
	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);
	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);
}

/*
 * Whether SIGIO is emitted on plyaback.
 * XXX I don't understand conditions that NetBSD7 emits signal.
 */
DEF(FIOASYNC_play_signal)
{
	struct audio_info ai;
	int r;
	int fd;
	int val;
	char *data;
	int i;

	TEST("FIOASYNC_play_signal");
	if (hw_canplay() == 0) {
		XP_SKIP("This test is only for playable device");
		return;
	}

	signal(SIGIO, signal_FIOASYNC);
	sigio_caught = 0;

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	REQUIRED_IF(ai.blocksize != 0);
	data = (char *)malloc(ai.blocksize);
	REQUIRED_IF(data != NULL);
	memset(data, 0xff, ai.blocksize);

	val = 1;
	r = IOCTL(fd, FIOASYNC, &val, "on");
	XP_SYS_EQ(0, r);

	r = WRITE(fd, data, ai.blocksize);
	XP_SYS_EQ(ai.blocksize, r);

	/* Waits signal until 1sec */
	for (i = 0; i < 100 && sigio_caught == 0; i++) {
		usleep(10000);
	}
	signal(SIGIO, SIG_IGN);
	XP_EQ(1, sigio_caught);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	free(data);
	signal(SIGIO, SIG_IGN);
	sigio_caught = 0;
}

/*
 * Whether SIGIO is emitted on recording.
 */
DEF(FIOASYNC_rec_signal)
{
	char buf[10];
	int r;
	int fd;
	int val;
	int i;

	TEST("FIOASYNC_rec_signal");
	if (hw_canrec() == 0) {
		XP_SKIP("This test is only for recordable device");
		return;
	}

	signal(SIGIO, signal_FIOASYNC);
	sigio_caught = 0;

	fd = OPEN(devaudio, O_RDONLY);
	REQUIRED_SYS_OK(fd);

	val = 1;
	r = IOCTL(fd, FIOASYNC, &val, "on");
	XP_SYS_EQ(0, r);

	r = READ(fd, buf, sizeof(buf));
	XP_SYS_EQ(sizeof(buf), r);

	/* Wait signal until 1sec */
	for (i = 0; i < 100 && sigio_caught == 0; i++) {
		usleep(10000);
	}
	signal(SIGIO, SIG_IGN);
	XP_EQ(1, sigio_caught);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	signal(SIGIO, SIG_IGN);
	sigio_caught = 0;
}

/*
 * FIOASYNC doesn't affect other descriptor.
 * For simplify, test only for playback...
 */
DEF(FIOASYNC_multi)
{
	struct audio_info ai;
	char *buf;
	char pipebuf[1];
	int r;
	int i;
	int fd1;
	int fd2;
	int pd[2];
	int val;
	pid_t pid;
	int status;

	TEST("FIOASYNC_multi");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if (hw_canplay() == 0) {
		XP_SKIP("This test is only for playable device");
		return;
	}

	/* Pipe used between parent and child */
	r = pipe(pd);
	REQUIRED_SYS_EQ(0, r);

	fd1 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd1);
	fd2 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd2);

	/* Pause fd2 */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd2, AUDIO_SETINFO, &ai, "pause");
	REQUIRED_SYS_EQ(0, r);

	/* Fill both */
	r = IOCTL(fd1, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	REQUIRED_IF(ai.blocksize != 0);
	buf = (char *)malloc(ai.blocksize);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, ai.blocksize);
	r = WRITE(fd1, buf, ai.blocksize);
	XP_SYS_EQ(ai.blocksize, r);

	sigio_caught = 0;
	val = 1;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == -1) {
		REQUIRED_SYS_OK(pid);
	}
	if (pid == 0) {
		/* Child */
		close(fd1);

		/* Child enables ASYNC on fd2 */
		signal(SIGIO, signal_FIOASYNC);
		r = IOCTL(fd2, FIOASYNC, &val, "on");
		/* It cannot count errors because here is a child process */
		/* XP_SYS_EQ(0, r); */

		/*
		 * Waits signal until 1sec.
		 * But fd2 is paused so it should never raise.
		 */
		for (i = 0; i < 100 && sigio_caught == 0; i++) {
			usleep(10000);
		}
		signal(SIGIO, SIG_IGN);
		pipebuf[0] = sigio_caught;
		/* This is not WRITE() macro here */
		write(pd[1], pipebuf, sizeof(pipebuf));

		/* XXX? */
		close(fd2);
		sleep(1);
		exit(0);
	} else {
		/* Parent */
		DPRINTF("  > fork() = %d\n", (int)pid);

		/* Parent enables ASYNC on fd1 */
		signal(SIGIO, signal_FIOASYNC);
		r = IOCTL(fd1, FIOASYNC, &val, "on");
		XP_SYS_EQ(0, r);

		/* Waits signal until 1sec */
		for (i = 0; i < 100 && sigio_caught == 0; i++) {
			usleep(10000);
		}
		signal(SIGIO, SIG_IGN);
		XP_EQ(1, sigio_caught);

		/* Then read child's result from pipe */
		r = read(pd[0], pipebuf, sizeof(pipebuf));
		if (r != 1) {
			XP_FAIL("reading from child failed");
		}
		DPRINTF("  > child's sigio_cauht = %d\n", pipebuf[0]);
		XP_EQ(0, pipebuf[0]);

		waitpid(pid, &status, 0);
	}

	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);
	r = CLOSE(fd2);
	XP_SYS_EQ(0, r);

	signal(SIGIO, SIG_IGN);
	sigio_caught = 0;
	free(buf);
}

/*
 * Check AUDIO_WSEEK behavior.
 */
DEF(AUDIO_WSEEK)
{
	char buf[4];
	struct audio_info ai;
	int r;
	int fd;
	u_long n;

	TEST("AUDIO_WSEEK");

	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	/* Pause to count sample data */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause=1");
	REQUIRED_SYS_EQ(0, r);

	/* On the initial state, it should be 0 bytes */
	n = 0;
	r = IOCTL(fd, AUDIO_WSEEK, &n, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, n);

	/* When writing 4 bytes, it should be 4 bytes */
	memset(buf, 0xff, sizeof(buf));
	r = WRITE(fd, buf, sizeof(buf));
	REQUIRED_EQ(sizeof(buf), r);
	r = IOCTL(fd, AUDIO_WSEEK, &n, "");
	XP_SYS_EQ(0, r);
	if (netbsd < 9) {
		/*
		 * On NetBSD7, it will return 0.
		 * Perhaps, WSEEK returns the number of pustream bytes but
		 * data has already advanced...
		 */
		XP_EQ(0, n);
	} else {
		/* Data less than one block remains here */
		XP_EQ(4, n);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Check AUDIO_SETFD behavior for O_*ONLY descriptor.
 * On NetBSD7, SETFD modify audio layer's state (and MD driver's state)
 * regardless of open mode.  GETFD obtains audio layer's duplex.
 * On NetBSD9, SETFD is obsoleted.  GETFD obtains hardware's duplex. 
 */
void
test_AUDIO_SETFD_xxONLY(int openmode)
{
	struct audio_info ai;
	int r;
	int fd;
	int n;

	TEST("AUDIO_SETFD_%s", openmode_str[openmode] + 2);
	if (openmode == O_RDONLY && hw_canrec() == 0) {
		XP_SKIP("This test is for recordable device");
		return;
	}
	if (openmode == O_WRONLY && hw_canplay() == 0) {
		XP_SKIP("This test is for playable device");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/*
	 * Just after open(2),
	 * - On NetBSD7, it's always half-duplex.
	 * - On NetBSD9, it's the same as hardware one regardless of openmode.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_GETFD, &n, "");
	XP_SYS_EQ(0, r);
	if (netbsd < 9) {
		XP_EQ(0, n);
	} else {
		XP_EQ(hw_fulldup(), n);
	}

	/*
	 * When trying to set to full-duplex,
	 * - On NetBSD7, it will succeed if the hardware is full-duplex, or
	 *   will fail if the hardware is half-duplex.
	 * - On NetBSD9, it will always succeed but will not be modified.
	 */
	n = 1;
	r = IOCTL(fd, AUDIO_SETFD, &n, "on");
	if (netbsd < 8) {
		if (hw_fulldup()) {
			XP_SYS_EQ(0, r);
		} else {
			XP_SYS_NG(ENOTTY, r);
		}
	} else if (netbsd == 8) {
		XP_FAIL("expected result is unknown");
	} else {
		XP_SYS_EQ(0, r);
	}

	/*
	 * When obtain it,
	 * - On NetBSD7, it will be 1 if the hardware is full-duplex or
	 *   0 if half-duplex.
	 * - On NetBSD9, it will never be changed because it's the hardware
	 *   property.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_GETFD, &n, "");
	XP_SYS_EQ(0, r);
	if (netbsd < 8) {
		XP_EQ(hw_fulldup(), n);
	} else if (netbsd == 8) {
		XP_FAIL("expected result is unknown");
	} else {
		XP_EQ(hw_fulldup(), n);
	}

	/* Some track parameters like ai.*.open should not change */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(mode2play(openmode), ai.play.open);
	XP_EQ(mode2rec(openmode), ai.record.open);

	/*
	 * When trying to set to half-duplex,
	 * - On NetBSD7, it will succeed if the hardware is full-duplex, or
	 *   it will succeed with nothing happens.
	 * - On NetBSD9, it will always succeed but nothing happens.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_SETFD, &n, "off");
	XP_SYS_EQ(0, r);

	/*
	 * When obtain it again,
	 * - On NetBSD7, it will be 0 if the hardware is full-duplex, or
	 *   still 0 if half-duplex.
	 * - On NetBSD9, it should not change.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_GETFD, &n, "");
	XP_SYS_EQ(0, r);
	if (netbsd < 9) {
		XP_EQ(0, n);
	} else {
		XP_EQ(hw_fulldup(), n);
	}

	/* Some track parameters like ai.*.open should not change */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(mode2play(openmode), ai.play.open);
	XP_EQ(mode2rec(openmode), ai.record.open);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(AUDIO_SETFD_RDONLY)	{ test_AUDIO_SETFD_xxONLY(O_RDONLY); }
DEF(AUDIO_SETFD_WRONLY)	{ test_AUDIO_SETFD_xxONLY(O_WRONLY); }

/*
 * Check AUDIO_SETFD behavior for O_RDWR descriptor.
 */
DEF(AUDIO_SETFD_RDWR)
{
	struct audio_info ai;
	int r;
	int fd;
	int n;

	TEST("AUDIO_SETFD_RDWR");
	if (!hw_fulldup()) {
		XP_SKIP("This test is only for full-duplex device");
		return;
	}

	fd = OPEN(devaudio, O_RDWR);
	REQUIRED_SYS_OK(fd);

	/*
	 * - audio(4) manpage until NetBSD7 said "If a full-duplex capable
	 *   audio device is opened for both reading and writing it will
	 *   start in half-duplex play mode", but implementation doesn't
	 *   seem to follow it.  It returns full-duplex.
	 * - On NetBSD9, it should return full-duplex on full-duplex, or
	 *   half-duplex on half-duplex.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_GETFD, &n, "");
	XP_SYS_EQ(0, r);
	XP_EQ(hw_fulldup(), n);

	/*
	 * When trying to set to full-duplex,
	 * - On NetBSD7, it will succeed with nothing happens if full-duplex,
	 *   or will fail if half-duplex.
	 * - On NetBSD9, it will always succeed with nothing happens.
	 */
	n = 1;
	r = IOCTL(fd, AUDIO_SETFD, &n, "on");
	if (netbsd < 9) {
		if (hw_fulldup()) {
			XP_SYS_EQ(0, r);
		} else {
			XP_SYS_NG(ENOTTY, r);
		}
	} else {
		XP_SYS_EQ(0, r);
	}

	/* When obtains it, it retuns half/full-duplex as is */
	n = 0;
	r = IOCTL(fd, AUDIO_GETFD, &n, "");
	XP_SYS_EQ(0, r);
	XP_EQ(hw_fulldup(), n);

	/* Some track parameters like ai.*.open should not change */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(1, ai.play.open);
	XP_EQ(mode2rec(O_RDWR), ai.record.open);

	/*
	 * When trying to set to half-duplex,
	 * - On NetBSD7, it will succeed if the hardware is full-duplex, or
	 *   it will succeed with nothing happens.
	 * - On NetBSD9, it will always succeed but nothing happens.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_SETFD, &n, "off");
	if (netbsd < 8) {
		XP_SYS_EQ(0, r);
	} else if (netbsd == 8) {
		XP_FAIL("expected result is unknown");
	} else {
		XP_SYS_EQ(0, r);
	}

	/*
	 * When obtain it again,
	 * - On NetBSD7, it will be 0 if the hardware is full-duplex, or
	 *   still 0 if half-duplex.
	 * - On NetBSD9, it should be 1 if the hardware is full-duplex, or
	 *   0 if half-duplex.
	 */
	n = 0;
	r = IOCTL(fd, AUDIO_GETFD, &n, "");
	XP_SYS_EQ(0, r);
	if (netbsd < 8) {
		XP_EQ(0, n);
	} else if (netbsd == 8) {
		XP_FAIL("expected result is unknown");
	} else {
		XP_EQ(hw_fulldup(), n);
	}

	/* Some track parameters like ai.*.open should not change */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(1, ai.play.open);
	XP_EQ(mode2rec(O_RDWR), ai.record.open);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Check AUDIO_GETINFO.eof behavior.
 */
DEF(AUDIO_GETINFO_eof)
{
	struct audio_info ai;
	char buf[4];
	int r;
	int fd, fd1;

	TEST("AUDIO_GETINFO_eof");
	if (hw_canplay() == 0) {
		XP_SKIP("This test is for playable device");
		return;
	}

	fd = OPEN(devaudio, O_RDWR);
	REQUIRED_SYS_OK(fd);

	/* Pause to make no sound */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "pause");
	REQUIRED_SYS_EQ(0, r);

	/* It should be 0 initially */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, ai.play.eof);
	XP_EQ(0, ai.record.eof);

	/* Writing zero bytes should increment it */
	r = WRITE(fd, &r, 0);
	REQUIRED_SYS_OK(r);
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(1, ai.play.eof);
	XP_EQ(0, ai.record.eof);

	/* Writing one ore more bytes should noto increment it */ 
	memset(buf, 0xff, sizeof(buf));
	r = WRITE(fd, buf, sizeof(buf));
	REQUIRED_SYS_OK(r);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(1, ai.play.eof);
	XP_EQ(0, ai.record.eof);

	/* Writing zero bytes again should increment it */
	r = WRITE(fd, buf, 0);
	REQUIRED_SYS_OK(r);
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(2, ai.play.eof);
	XP_EQ(0, ai.record.eof);

	/* Reading zero bytes should not increment it */
	if (hw_fulldup()) {
		r = READ(fd, buf, 0);
		REQUIRED_SYS_OK(r);
		memset(&ai, 0, sizeof(ai));
		r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
		XP_SYS_EQ(0, r);
		XP_EQ(2, ai.play.eof);
		XP_EQ(0, ai.record.eof);
	}

	/* should not interfere with other descriptor */
	if (netbsd >= 8) {
		fd1 = OPEN(devaudio, O_RDWR);
		REQUIRED_SYS_OK(fd1);
		memset(&ai, 0, sizeof(ai));
		r = IOCTL(fd1, AUDIO_GETBUFINFO, &ai, "");
		XP_SYS_EQ(0, r);
		XP_EQ(0, ai.play.eof);
		XP_EQ(0, ai.record.eof);
		r = CLOSE(fd1);
		XP_SYS_EQ(0, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	xxx_close_wait();

	/* When reopen, it should reset the counter */
	fd = OPEN(devaudio, O_RDWR);
	REQUIRED_SYS_OK(fd);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, ai.play.eof);
	XP_EQ(0, ai.record.eof);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Check relationship between openmode and mode set by AUDIO_SETINFO.
 */
void
test_AUDIO_SETINFO_mode(int openmode, int index, int setmode, int expected)
{
	struct audio_info ai;
	char buf[10];
	int inimode;
	int r;
	int fd;
	bool canwrite;
	bool canread;

	/* index was passed only for displaying here */
	TEST("AUDIO_SETINFO_mode_%s_%d", openmode_str[openmode] + 2, index);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	inimode = mode2aumode(openmode);

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/* When just after opening */
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	XP_EQ(inimode, ai.mode);
	XP_EQ(mode2play(openmode), ai.play.open);
	XP_EQ(mode2rec(openmode),  ai.record.open);
	XP_NE(0, ai.play.buffer_size);
	XP_NE(0, ai.record.buffer_size);

	/* Change mode (and pause here) */
	ai.mode = setmode;
	ai.play.pause = 1;
	ai.record.pause = 1;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "mode");
	XP_SYS_EQ(0, r);
	if (r == 0) {
		r = IOCTL(fd, AUDIO_GETINFO, &ai, "");
		XP_SYS_EQ(0, r);
		XP_EQ(expected, ai.mode);

		/* It seems to keep the initial openmode regardless of mode */
		XP_EQ(mode2play(openmode), ai.play.open);
		XP_EQ(mode2rec(openmode), ai.record.open);
		XP_NE(0, ai.play.buffer_size);
		XP_NE(0, ai.record.buffer_size);
	}

	/*
	 * On NetBSD7, whether writable depends openmode when open.
	 * On NetBSD9, whether writable should depend inimode when open.
	 * Modifying after open should not affect this mode.
	 */
	if (netbsd < 9) {
		canwrite = (openmode != O_RDONLY);
	} else {
		canwrite = ((inimode & AUMODE_PLAY) != 0);
	}
	r = WRITE(fd, buf, 0);
	if (canwrite) {
		XP_SYS_EQ(0, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	/*
	 * On NetBSD7, whether readable depends openmode when open.
	 * On NetBSD9, whether readable should depend inimode when open.
	 * Modifying after open should not affect this mode.
	 */
	if (netbsd < 9) {
		canread = (openmode != O_WRONLY);
	} else {
		canread = ((inimode & AUMODE_RECORD) != 0);
	}
	r = READ(fd, buf, 0);
	if (canread) {
		XP_SYS_EQ(0, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
/*
 * XXX hmm... it's too complex
 */
/* shortcut for table form */
#define P	AUMODE_PLAY
#define A	AUMODE_PLAY_ALL
#define R	AUMODE_RECORD
struct setinfo_mode_t {
	int setmode;	/* mode used in SETINFO */
	int expmode7;	/* expected mode on NetBSD7 */
	int expmode9;	/* expected mode on NetBSD9 */
};
/*
 * The following tables show this operation on NetBSD7 is almost 'undefined'.
 * In contrast, NetBSD9 never changes mode by AUDIO_SETINFO except
 * AUMODE_PLAY_ALL.
 *
 * setmode == 0 and 8 are out of range and invalid input samples.
 * But NetBSD7 seems to accept it as is.
 */
struct setinfo_mode_t table_SETINFO_mode_O_RDONLY[] = {
	/* setmode	expmode7	expmode9 */
	{     0,	     0,		 R    },
	{     P,	     P,		 R    },
	{   A  ,	   A|P,		 R    },
	{   A|P,	   A|P,		 R    },
	{ R    ,	 R    ,		 R    },
	{ R|  P,	     P,		 R    },
	{ R|A  ,	   A|P,		 R    },
	{ R|A|P,	   A|P,		 R    },
	{     8,	     8,		 R    },
};
struct setinfo_mode_t table_SETINFO_mode_O_WRONLY[] = {
	/* setmode	expmode7	expmode9 */
	{     0,	     0,		     P },
	{     P,	     P,		     P },
	{   A  ,	   A|P,		   A|P },
	{   A|P,	   A|P,		   A|P },
	{ R    ,	 R    ,		     P },
	{ R|  P,	     P,		     P },
	{ R|A  ,	   A|P,		   A|P },
	{ R|A|P,	   A|P,		   A|P },
	{     8,	     8,		     P },
};
#define f(openmode, index)	do {					\
	struct setinfo_mode_t *table = table_SETINFO_mode_##openmode;	\
	int setmode = table[index].setmode;				\
	int expected = (netbsd < 9)					\
	    ? table[index].expmode7					\
	    : table[index].expmode9;					\
	test_AUDIO_SETINFO_mode(openmode, index, setmode, expected);	\
} while (0)
DEF(AUDIO_SETINFO_mode_RDONLY_0) { f(O_RDONLY, 0); }
DEF(AUDIO_SETINFO_mode_RDONLY_1) { f(O_RDONLY, 1); }
DEF(AUDIO_SETINFO_mode_RDONLY_2) { f(O_RDONLY, 2); }
DEF(AUDIO_SETINFO_mode_RDONLY_3) { f(O_RDONLY, 3); }
DEF(AUDIO_SETINFO_mode_RDONLY_4) { f(O_RDONLY, 4); }
DEF(AUDIO_SETINFO_mode_RDONLY_5) { f(O_RDONLY, 5); }
DEF(AUDIO_SETINFO_mode_RDONLY_6) { f(O_RDONLY, 6); }
DEF(AUDIO_SETINFO_mode_RDONLY_7) { f(O_RDONLY, 7); }
DEF(AUDIO_SETINFO_mode_RDONLY_8) { f(O_RDONLY, 8); }
DEF(AUDIO_SETINFO_mode_WRONLY_0) { f(O_WRONLY, 0); }
DEF(AUDIO_SETINFO_mode_WRONLY_1) { f(O_WRONLY, 1); }
DEF(AUDIO_SETINFO_mode_WRONLY_2) { f(O_WRONLY, 2); }
DEF(AUDIO_SETINFO_mode_WRONLY_3) { f(O_WRONLY, 3); }
DEF(AUDIO_SETINFO_mode_WRONLY_4) { f(O_WRONLY, 4); }
DEF(AUDIO_SETINFO_mode_WRONLY_5) { f(O_WRONLY, 5); }
DEF(AUDIO_SETINFO_mode_WRONLY_6) { f(O_WRONLY, 6); }
DEF(AUDIO_SETINFO_mode_WRONLY_7) { f(O_WRONLY, 7); }
DEF(AUDIO_SETINFO_mode_WRONLY_8) { f(O_WRONLY, 8); }
#undef f
/*
 * The following tables also show that NetBSD7's behavior is almost
 * 'undefined'.
 */
struct setinfo_mode_t table_SETINFO_mode_O_RDWR_full[] = {
	/* setmode	expmode7	expmode9 */
	{     0,	    0,		R|  P },
	{     P,	    P,		R|  P },
	{   A  ,	  A|P,		R|A|P },
	{   A|P,	  A|P,		R|A|P },
	{ R    ,	R    ,		R|  P },
	{ R|  P,	R|  P,		R|  P },
	{ R|A  ,	R|A|P,		R|A|P },
	{ R|A|P,	R|A|P,		R|A|P },
	{     8,	    8,		R|  P },
};
struct setinfo_mode_t table_SETINFO_mode_O_RDWR_half[] = {
	/* setmode	expmode7	expmode9 */
	{     0,	    0,		    P },
	{     P,	    P,		    P },
	{   A  ,	  A|P,		  A|P },
	{   A|P,	  A|P,		  A|P },
	{ R    ,	R    ,		    P },
	{ R|  P,	    P,		    P },
	{ R|A  ,	  A|P,		  A|P },
	{ R|A|P,	  A|P,		  A|P },
	{     8,	    8,		    P },
};
#define f(index)	do {						\
	struct setinfo_mode_t *table = (hw_fulldup())			\
	    ? table_SETINFO_mode_O_RDWR_full				\
	    : table_SETINFO_mode_O_RDWR_half;				\
	int setmode = table[index].setmode;				\
	int expected = (netbsd < 9)					\
	    ? table[index].expmode7					\
	    : table[index].expmode9;					\
	test_AUDIO_SETINFO_mode(O_RDWR, index, setmode, expected);	\
} while (0)
DEF(AUDIO_SETINFO_mode_RDWR_0) { f(0); }
DEF(AUDIO_SETINFO_mode_RDWR_1) { f(1); }
DEF(AUDIO_SETINFO_mode_RDWR_2) { f(2); }
DEF(AUDIO_SETINFO_mode_RDWR_3) { f(3); }
DEF(AUDIO_SETINFO_mode_RDWR_4) { f(4); }
DEF(AUDIO_SETINFO_mode_RDWR_5) { f(5); }
DEF(AUDIO_SETINFO_mode_RDWR_6) { f(6); }
DEF(AUDIO_SETINFO_mode_RDWR_7) { f(7); }
DEF(AUDIO_SETINFO_mode_RDWR_8) { f(8); }
#undef f
#undef P
#undef A
#undef R

/*
 * Check whether encoding params can be set.
 */
void
test_AUDIO_SETINFO_params_set(int openmode, int aimode, int pause)
{
	struct audio_info ai;
	int r;
	int fd;

	/*
	 * aimode is bool value that indicates whether to change ai.mode.
	 * pause is bool value that indicates whether to change ai.*.pause.
	 */

	TEST("AUDIO_SETINFO_params_%s_%d_%d",
	    openmode_str[openmode] + 2, aimode, pause);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	/* On half-duplex, O_RDWR is the same as O_WRONLY, so skip it */
	if (!hw_fulldup() && openmode == O_RDWR) {
		XP_SKIP("This is the same with O_WRONLY on half-duplex");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	AUDIO_INITINFO(&ai);
	/*
	 * It takes time and effort to check all parameters independently,
	 * so that use sample_rate as a representative.
	 */
	ai.play.sample_rate = 11025;
	ai.record.sample_rate = 11025;
	if (aimode)
		ai.mode = mode2aumode(openmode) & ~AUMODE_PLAY_ALL;
	if (pause) {
		ai.play.pause = 1;
		ai.record.pause = 1;
	}

	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	int expmode = (aimode)
	    ? (mode2aumode(openmode) & ~AUMODE_PLAY_ALL)
	    : mode2aumode(openmode);
	XP_EQ(expmode, ai.mode);
	XP_EQ(11025, ai.play.sample_rate);
	XP_EQ(pause, ai.play.pause);
	XP_EQ(11025, ai.record.sample_rate);
	XP_EQ(pause, ai.record.pause);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
#define f(a,b,c) test_AUDIO_SETINFO_params_set(a, b, c)
DEF(AUDIO_SETINFO_params_set_RDONLY_0)	{ f(O_RDONLY, 0, 0); }
DEF(AUDIO_SETINFO_params_set_RDONLY_1)	{ f(O_RDONLY, 0, 1); }
/* On RDONLY, ai.mode is not changable
 *  AUDIO_SETINFO_params_set_RDONLY_2)	{ f(O_RDONLY, 1, 0); }
 *  AUDIO_SETINFO_params_set_RDONLY_3)	{ f(O_RDONLY, 1, 1); }
 */
DEF(AUDIO_SETINFO_params_set_WRONLY_0)	{ f(O_WRONLY, 0, 0); }
DEF(AUDIO_SETINFO_params_set_WRONLY_1)	{ f(O_WRONLY, 0, 1); }
DEF(AUDIO_SETINFO_params_set_WRONLY_2)	{ f(O_WRONLY, 1, 0); }
DEF(AUDIO_SETINFO_params_set_WRONLY_3)	{ f(O_WRONLY, 1, 1); }
DEF(AUDIO_SETINFO_params_set_RDWR_0)	{ f(O_RDWR, 0, 0); }
DEF(AUDIO_SETINFO_params_set_RDWR_1)	{ f(O_RDWR, 0, 1); }
DEF(AUDIO_SETINFO_params_set_RDWR_2)	{ f(O_RDWR, 1, 0); }
DEF(AUDIO_SETINFO_params_set_RDWR_3)	{ f(O_RDWR, 1, 1); }
#undef f

/*
 * AUDIO_SETINFO for existing track should not be interfered by other
 * descriptor.
 * AUDIO_SETINFO for non-existing track affects/is affected sticky parameters
 * for backward compatibility.
 */
DEF(AUDIO_SETINFO_params_simul)
{
	struct audio_info ai;
	int fd0;
	int fd1;
	int r;

	TEST("AUDIO_SETINFO_params_simul");
	if (netbsd < 8) {
		XP_SKIP("Multiple open is not supported");
		return;
	}
	if (hw_canplay() == 0) {
		XP_SKIP("This test is for playable device");
		return;
	}

	/* Open the 1st one as playback only */
	fd0 = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd0);

	/* Open the 2nd one as both of playback and recording */
	fd1 = OPEN(devaudio, O_RDWR);
	REQUIRED_SYS_OK(fd1);

	/* Change some parameters of both track on the 2nd one */
	AUDIO_INITINFO(&ai);
	ai.play.sample_rate = 11025;
	ai.record.sample_rate = 11025;
	r = IOCTL(fd1, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/*
	 * The 1st one doesn't have recording track so that only recording
	 * parameter is affected by sticky parameter.
	 */
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd0, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(8000, ai.play.sample_rate);
	XP_EQ(11025, ai.record.sample_rate);

	/* Next, change some parameters of both track on the 1st one */
	AUDIO_INITINFO(&ai);
	ai.play.sample_rate = 16000;
	ai.record.sample_rate = 16000;
	r = IOCTL(fd0, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	/*
	 * On full-duplex device, the 2nd one has both track so that
	 * both track are not affected by sticky parameter.
	 * Otherwise, the 2nd one has only playback track so that
	 * playback track is not affected by sticky parameter.
	 */
	memset(&ai, 0, sizeof(ai));
	r = IOCTL(fd1, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(11025, ai.play.sample_rate);
	if (hw_fulldup()) {
		XP_EQ(11025, ai.record.sample_rate);
	} else {
		XP_EQ(16000, ai.record.sample_rate);
	}

	r = CLOSE(fd0);
	XP_SYS_EQ(0, r);
	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);
}

/*
 * AUDIO_SETINFO(encoding/precision) is tested in AUDIO_GETENC_range below.
 */

/*
 * Check whether the number of channels can be set.
 */
DEF(AUDIO_SETINFO_channels)
{
	struct audio_info hwinfo;
	struct audio_info ai;
	int mode;
	int r;
	int fd;
	int i;
	unsigned int ch;
	struct {
		int ch;
		bool expected;
	} table[] = {
		{  0,	false },
		{  1,	true },	/* monaural */
		{  2,	true },	/* stereo */
	};

	TEST("AUDIO_SETINFO_channels");
	if (netbsd < 8) {
		/*
		 * On NetBSD7, the result depends the hardware and there is
		 * no way to know it.
		 */
		XP_SKIP("The test doesn't make sense on NetBSD7");
		return;
	}

	mode = openable_mode();
	fd = OPEN(devaudio, mode);
	REQUIRED_SYS_OK(fd);

	/*
	 * The audio layer always supports monaural and stereo regardless of
	 * the hardware capability.
	 */
	for (i = 0; i < (int)__arraycount(table); i++) {
		ch = table[i].ch;
		bool expected = table[i].expected;

		AUDIO_INITINFO(&ai);
		if (mode != O_RDONLY)
			ai.play.channels = ch;
		if (mode != O_WRONLY)
			ai.record.channels = ch;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "channels=%d", ch);
		if (expected) {
			/* Expects to succeed */
			XP_SYS_EQ(0, r);

			r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
			XP_SYS_EQ(0, r);
			if (mode != O_RDONLY)
				XP_EQ(ch, ai.play.channels);
			if (mode != O_WRONLY)
				XP_EQ(ch, ai.record.channels);
		} else {
			/* Expects to fail */
			XP_SYS_NG(EINVAL, r);
		}
	}

	/*
	 * The maximum number of supported channels depends the hardware.
	 */
	/* Get the number of channels that the hardware supports */
	r = IOCTL(fd, AUDIO_GETFORMAT, &hwinfo, "");
	REQUIRED_SYS_EQ(0, r);

	if ((hwinfo.mode & AUMODE_PLAY)) {
		DPRINTF("  > hwinfo.play.channels = %d\n",
		    hwinfo.play.channels);
		for (ch = 3; ch <= hwinfo.play.channels; ch++) {
			AUDIO_INITINFO(&ai);
			ai.play.channels = ch;
			r = IOCTL(fd, AUDIO_SETINFO, &ai, "channels=%d", ch);
			XP_SYS_EQ(0, r);

			r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
			XP_SYS_EQ(0, r);
			XP_EQ(ch, ai.play.channels);
		}

		AUDIO_INITINFO(&ai);
		ai.play.channels = ch;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "channels=%d", ch);
		XP_SYS_NG(EINVAL, r);
	}
	if ((hwinfo.mode & AUMODE_RECORD)) {
		DPRINTF("  > hwinfo.record.channels = %d\n",
		    hwinfo.record.channels);
		for (ch = 3; ch <= hwinfo.record.channels; ch++) {
			AUDIO_INITINFO(&ai);
			ai.record.channels = ch;
			r = IOCTL(fd, AUDIO_SETINFO, &ai, "channels=%d", ch);
			XP_SYS_EQ(0, r);

			r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
			XP_SYS_EQ(0, r);
			XP_EQ(ch, ai.record.channels);
		}

		AUDIO_INITINFO(&ai);
		ai.record.channels = ch;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "channels=%d", ch);
		XP_SYS_NG(EINVAL, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Check whether the sample rate can be set.
 */
DEF(AUDIO_SETINFO_sample_rate)
{
	struct audio_info ai;
	int mode;
	int r;
	int fd;
	int i;
	struct {
		int freq;
		bool expected;
	} table[] = {
		{    999,	false },
		{   1000,	true },	/* lower limit */
		{  48000,	true },
		{ 192000,	true },	/* upper limit */
		{ 192001,	false },
	};

	TEST("AUDIO_SETINFO_sample_rate");
	if (netbsd < 8) {
		/*
		 * On NetBSD7, the result depends the hardware and there is
		 * no way to know it.
		 */
		XP_SKIP("The test doesn't make sense on NetBSD7");
		return;
	}

	mode = openable_mode();
	fd = OPEN(devaudio, mode);
	REQUIRED_SYS_OK(fd);

	for (i = 0; i < (int)__arraycount(table); i++) {
		int freq = table[i].freq;
		bool expected = table[i].expected;

		AUDIO_INITINFO(&ai);
		if (mode != O_RDONLY)
			ai.play.sample_rate = freq;
		if (mode != O_WRONLY)
			ai.record.sample_rate = freq;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "sample_rate=%d", freq);
		if (expected) {
			/* Expects to succeed */
			XP_SYS_EQ(0, r);

			r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
			XP_SYS_EQ(0, r);
			if (mode != O_RDONLY)
				XP_EQ(freq, ai.play.sample_rate);
			if (mode != O_WRONLY)
				XP_EQ(freq, ai.record.sample_rate);
		} else {
			/* Expects to fail */
			XP_SYS_NG(EINVAL, r);
		}
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * SETINFO(sample_rate = 0) should fail correctly.
 */
DEF(AUDIO_SETINFO_sample_rate_0)
{
	struct audio_info ai;
	int mode;
	int r;
	int fd;

	TEST("AUDIO_SETINFO_sample_rate_0");
	if (netbsd < 9) {
		/*
		 * On NetBSD7,8 this will block system call and you will not
		 * even be able to shutdown...
		 */
		XP_SKIP("This will cause an infinate loop in the kernel");
		return;
	}

	mode = openable_mode();
	fd = OPEN(devaudio, mode);
	REQUIRED_SYS_OK(fd);

	AUDIO_INITINFO(&ai);
	ai.play.sample_rate = 0;
	ai.record.sample_rate = 0;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "sample_rate=0");
	/* Expects to fail */
	XP_SYS_NG(EINVAL, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Check whether the pause/unpause works.
 */
void
test_AUDIO_SETINFO_pause(int openmode, int aimode, int param)
{
	struct audio_info ai;
	int r;
	int fd;

	/*
	 * aimode is bool value that indicates whether to change ai.mode.
	 * param is bool value that indicates whether to change encoding
	 * parameters of ai.{play,record}.*.
	 */

	TEST("AUDIO_SETINFO_pause_%s_%d_%d",
	    openmode_str[openmode] + 2, aimode, param);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	/* On half-duplex, O_RDWR is the same as O_WRONLY, so skip it */
	if (!hw_fulldup() && openmode == O_RDWR) {
		XP_SKIP("This is the same with O_WRONLY on half-duplex");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/* Set pause */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 1;
	ai.record.pause = 1;
	if (aimode)
		ai.mode = mode2aumode(openmode) & ~AUMODE_PLAY_ALL;
	if (param) {
		ai.play.sample_rate = 11025;
		ai.record.sample_rate = 11025;
	}

	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	int expmode = (aimode)
	    ? (mode2aumode(openmode) & ~AUMODE_PLAY_ALL)
	    : mode2aumode(openmode);
	XP_EQ(expmode, ai.mode);
	XP_EQ(1, ai.play.pause);
	XP_EQ(param ? 11025 : 8000, ai.play.sample_rate);
	XP_EQ(1, ai.record.pause);
	XP_EQ(param ? 11025 : 8000, ai.record.sample_rate);

	/* Set unpause (?) */
	AUDIO_INITINFO(&ai);
	ai.play.pause = 0;
	ai.record.pause = 0;
	if (aimode)
		ai.mode = mode2aumode(openmode);
	if (param) {
		ai.play.sample_rate = 16000;
		ai.record.sample_rate = 16000;
	}

	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	XP_SYS_EQ(0, r);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(mode2aumode(openmode), ai.mode);
	XP_EQ(0, ai.play.pause);
	XP_EQ(0, ai.record.pause);
	if (openmode != O_RDONLY)
		XP_EQ(param ? 16000 : 8000, ai.play.sample_rate);
	if (openmode != O_WRONLY)
		XP_EQ(param ? 16000 : 8000, ai.record.sample_rate);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(AUDIO_SETINFO_pause_RDONLY_0) { test_AUDIO_SETINFO_pause(O_RDONLY, 0, 0); }
DEF(AUDIO_SETINFO_pause_RDONLY_1) { test_AUDIO_SETINFO_pause(O_RDONLY, 0, 1); }
/* On RDONLY, ai.mode is not changable
 *  AUDIO_SETINFO_pause_RDONLY_2) { test_AUDIO_SETINFO_pause(O_RDONLY, 1, 0); }
 *  AUDIO_SETINFO_pause_RDONLY_3) { test_AUDIO_SETINFO_pause(O_RDONLY, 1, 1); }
 */
DEF(AUDIO_SETINFO_pause_WRONLY_0) { test_AUDIO_SETINFO_pause(O_WRONLY, 0, 0); }
DEF(AUDIO_SETINFO_pause_WRONLY_1) { test_AUDIO_SETINFO_pause(O_WRONLY, 0, 1); }
DEF(AUDIO_SETINFO_pause_WRONLY_2) { test_AUDIO_SETINFO_pause(O_WRONLY, 1, 0); }
DEF(AUDIO_SETINFO_pause_WRONLY_3) { test_AUDIO_SETINFO_pause(O_WRONLY, 1, 1); }
DEF(AUDIO_SETINFO_pause_RDWR_0)   { test_AUDIO_SETINFO_pause(O_RDWR, 0, 0); }
DEF(AUDIO_SETINFO_pause_RDWR_1)   { test_AUDIO_SETINFO_pause(O_RDWR, 0, 1); }
DEF(AUDIO_SETINFO_pause_RDWR_2)   { test_AUDIO_SETINFO_pause(O_RDWR, 1, 0); }
DEF(AUDIO_SETINFO_pause_RDWR_3)   { test_AUDIO_SETINFO_pause(O_RDWR, 1, 1); }

/*
 * Check whether gain can be obtained/set.
 * And the gain should work with rich mixer.
 * PR kern/52781
 */
DEF(AUDIO_SETINFO_gain)
{
	struct audio_info ai;
	mixer_ctrl_t m;
	int index;
	int master;
	int master_backup;
	int gain;
	int fd;
	int mixerfd;
	int r;

	TEST("AUDIO_SETINFO_gain");

	/* Open /dev/mixer */
	mixerfd = OPEN(devmixer, O_RDWR);
	REQUIRED_SYS_OK(mixerfd);
	index = mixer_get_outputs_master(mixerfd);
	if (index == -1) {
		XP_SKIP("Hardware has no outputs.master");
		CLOSE(mixerfd);
		return;
	}

	/*
	 * Get current outputs.master.
	 * auich(4) requires class type (m.type) and number of channels
	 * (un.value.num_channels) in addition to the index (m.dev)...
	 * What is the index...?
	 */
	memset(&m, 0, sizeof(m));
	m.dev = index;
	m.type = AUDIO_MIXER_VALUE;
	m.un.value.num_channels = 1; /* dummy */
	r = IOCTL(mixerfd, AUDIO_MIXER_READ, &m, "m.dev=%d", m.dev);
	REQUIRED_SYS_EQ(0, r);
	master = m.un.value.level[0];
	DPRINTF("  > outputs.master = %d\n", master);
	master_backup = master;

	/* Open /dev/audio */
	fd = OPEN(devaudio, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	/* Check ai.play.gain */
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "");
	XP_SYS_EQ(0, r);
	XP_EQ(master, ai.play.gain);

	/* Change it some different value */
	AUDIO_INITINFO(&ai);
	if (master == 0)
		gain = 255;
	else
		gain = 0;
	ai.play.gain = gain;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "play.gain=%d", ai.play.gain);
	XP_SYS_EQ(0, r);

	/* Check gain has changed */
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "play.gain");
	XP_SYS_EQ(0, r);
	XP_NE(master, ai.play.gain);

	/* Check whether outputs.master work with gain */
	r = IOCTL(mixerfd, AUDIO_MIXER_READ, &m, "");
	XP_SYS_EQ(0, r);
	XP_EQ(ai.play.gain, m.un.value.level[0]);

	/* Restore outputs.master */
	AUDIO_INITINFO(&ai);
	ai.play.gain = master_backup;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "play.gain=%d", ai.play.gain);
	XP_SYS_EQ(0, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
	r = CLOSE(mixerfd);
	XP_SYS_EQ(0, r);
}

/*
 * Look if there are any (non-zero) gain values that can be changed.
 * If any gain can be set, it is set to gain[0].
 * If another gain can be set, it is set to gain[1], otherwise gain[1] = -1.
 * This is for AUDIO_SETINFO_gain_balance.
 */
static void
get_changeable_gain(int fd, int *gain, const char *dir, int offset)
{
	struct audio_info ai;
	int *ai_gain;
	int hi;
	int lo;
	int r;

	/* A hack to handle ai.{play,record}.gain in the same code.. */
	ai_gain = (int *)(((char *)&ai) + offset);

	/* Try to set the maximum gain */
	AUDIO_INITINFO(&ai);
	*ai_gain = AUDIO_MAX_GAIN;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "%s.gain=%d", dir, *ai_gain);
	XP_SYS_EQ(0, r);
	/* Get again.  The value you set is not always used as is. */
	AUDIO_INITINFO(&ai);
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
	XP_SYS_EQ(0, r);
	hi = *ai_gain;

	/* Look for next configurable value. */
	for (lo = hi - 1; lo >= 0; lo--) {
		AUDIO_INITINFO(&ai);
		*ai_gain = lo;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "%s.gain=%d", dir, *ai_gain);
		XP_SYS_EQ(0, r);
		/* Get again */
		r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
		XP_SYS_EQ(0, r);
		if (*ai_gain != hi) {
			lo = *ai_gain;
			break;
		}
	}

	/* Now gain is lo(=gain[0]). */

	/*
	 * hi  lo
	 * --- ---
	 *  <0  <0          : not available.
	 * >=0  <0          : available but not changeable.
	 * >=0 >=0 (hi!=lo) : available and changeable.
	 */
	if (hi < 0) {
		gain[0] = -1;
		gain[1] = -1;
		DPRINTF("  > %s.gain cannot be set\n", dir);
	} else if (lo < 0) {
		gain[0] = hi;
		gain[1] = -1;
		DPRINTF("  > %s.gain can only be set %d\n", dir, gain[0]);
	} else {
		gain[0] = lo;
		gain[1] = hi;
		DPRINTF("  > %s.gain can be set %d, %d\n",
		    dir, gain[0], gain[1]);
	}
}

/*
 * Look if there are any balance values that can be changed.
 * If any balance value can be set, it is set to balance[0].
 * If another balance value can be set, it is set to balance[1],
 * otherwise balance[1] = -1.
 * This is for AUDIO_SETINFO_gain_balance.
 */
static void
get_changeable_balance(int fd, int *balance, const char *dir, int offset)
{
	struct audio_info ai;
	u_char *ai_balance;
	u_char left;
	u_char right;
	int r;

	/* A hack to handle ai.{play,record}.balance in the same code.. */
	ai_balance = ((u_char *)&ai) + offset;

	/* Look for the right side configurable value. */
	AUDIO_INITINFO(&ai);
	*ai_balance = AUDIO_RIGHT_BALANCE;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "%s.balance=%d", dir, *ai_balance);
	XP_SYS_EQ(0, r);
	/* Get again.  The value you set is not always used as is. */
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
	XP_SYS_EQ(0, r);
	right = *ai_balance;

	/* Look for the left side configurable value. */
	AUDIO_INITINFO(&ai);
	*ai_balance = AUDIO_LEFT_BALANCE;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "%s.balance=%d", dir, *ai_balance);
	XP_SYS_EQ(0, r);
	/* Get again */
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
	XP_SYS_EQ(0, r);
	left = *ai_balance;

	/* Now balance is the left(=balance[0]). */

	if (left == right) {
		/* The driver has no balance feature. */
		balance[0] = left;
		balance[1] = -1;
		DPRINTF("  > %s.balance can only be set %d\n",
		    dir, balance[0]);
	} else {
		balance[0] = left;
		balance[1] = right;
		DPRINTF("  > %s.balance can be set %d, %d\n",
		    dir, balance[0], balance[1]);
	}
}

/*
 * Check whether gain and balance can be set at the same time.
 * PR kern/56308
 */
DEF(AUDIO_SETINFO_gain_balance)
{
	struct audio_info oai;
	struct audio_info ai;
	int i;
	int mode;
	int fd;
	int r;
	int pgain[2];
	int pbalance[2];
	int rgain[2];
	int rbalance[2];
	bool ptest;
	bool rtest;

	TEST("AUDIO_SETINFO_gain_balance");

	mode = openable_mode();
	fd = OPEN(devaudio, mode);
	REQUIRED_SYS_OK(fd);

	/* Backup current gain and balance */
	r = IOCTL(fd, AUDIO_GETINFO, &oai, "&oai");
	XP_SYS_EQ(0, r);

	if (debug) {
		printf("  > old play.gain      = %d\n", oai.play.gain);
		printf("  > old play.balance   = %d\n", oai.play.balance);
		printf("  > old record.gain    = %d\n", oai.record.gain);
		printf("  > old record.balance = %d\n", oai.record.balance);
	}

	for (i = 0; i < 2; i++) {
		pgain[i]    = -1;
		pbalance[i] = -1;
		rgain[i]    = -1;
		rbalance[i] = -1;
	}

	/*
	 * First, check each one separately can be changed.
	 *
	 * The simplest two different gain values are zero and non-zero.
	 * But some device drivers seem to process balance differently
	 * when the gain is high enough and when the gain is zero or near.
	 * So I needed to select two different "non-zero (and high if
	 * possible)" gains.
	 */
	if (hw_canplay()) {
		get_changeable_gain(fd, pgain, "play",
		    offsetof(struct audio_info, play.gain));
		get_changeable_balance(fd, pbalance, "play",
		    offsetof(struct audio_info, play.balance));
	}
	if (hw_canrec()) {
		get_changeable_gain(fd, rgain, "record",
		    offsetof(struct audio_info, record.gain));
		get_changeable_balance(fd, rbalance, "record",
		    offsetof(struct audio_info, record.balance));
	}

	/*
	 * [0] [1]
	 * --- ---
	 *  -1  *  : not available.
	 * >=0  -1 : available but not changeable.
	 * >=0 >=0 : available and changeable.  It can be tested.
	 */
	ptest = (pgain[0]    >= 0 && pgain[1]    >= 0 &&
	         pbalance[0] >= 0 && pbalance[1] >= 0);
	rtest = (rgain[0]    >= 0 && rgain[1]    >= 0 &&
	         rbalance[0] >= 0 && rbalance[1] >= 0);

	if (ptest == false && rtest == false) {
		XP_SKIP(
		    "The test requires changeable gain and changeable balance");

		/* Restore as possible */
		AUDIO_INITINFO(&ai);
		ai.play.gain      = oai.play.gain;
		ai.play.balance   = oai.play.balance;
		ai.record.gain    = oai.record.gain;
		ai.record.balance = oai.record.balance;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "restore all");
		XP_SYS_EQ(0, r);

		r = CLOSE(fd);
		XP_SYS_EQ(0, r);
		return;
	}

	/*
	 * If both play.gain and play.balance are changeable,
	 * it should be able to set both at the same time.
	 */
	if (ptest) {
		AUDIO_INITINFO(&ai);
		ai.play.gain    = pgain[1];
		ai.play.balance = pbalance[1];
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "play.gain=%d/balance=%d",
		    ai.play.gain, ai.play.balance);
		XP_SYS_EQ(0, r);

		AUDIO_INITINFO(&ai);
		r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
		XP_SYS_EQ(0, r);

		DPRINTF("  > setting play.gain=%d/balance=%d: "
		    "result gain=%d/balance=%d\n",
		    pgain[1], pbalance[1], ai.play.gain, ai.play.balance);
		XP_EQ(ai.play.gain,    pgain[1]);
		XP_EQ(ai.play.balance, pbalance[1]);
	}
	/*
	 * If both record.gain and record.balance are changeable,
	 * it should be able to set both at the same time.
	 */
	if (rtest) {
		AUDIO_INITINFO(&ai);
		ai.record.gain    = rgain[1];
		ai.record.balance = rbalance[1];
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "record.gain=%d/balance=%d",
		    ai.record.gain, ai.record.balance);
		XP_SYS_EQ(0, r);

		AUDIO_INITINFO(&ai);
		r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
		XP_SYS_EQ(0, r);

		DPRINTF("  > setting record.gain=%d/balance=%d: "
		    "result gain=%d/balance=%d\n",
		    rgain[1], rbalance[1], ai.record.gain, ai.record.balance);
		XP_EQ(ai.record.gain,    rgain[1]);
		XP_EQ(ai.record.balance, rbalance[1]);
	}

	/*
	 * Restore all values as possible at the same time.
	 * This restore is also a test.
	 */
	AUDIO_INITINFO(&ai);
	ai.play.gain      = oai.play.gain;
	ai.play.balance   = oai.play.balance;
	ai.record.gain    = oai.record.gain;
	ai.record.balance = oai.record.balance;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "restore all");
	XP_SYS_EQ(0, r);

	AUDIO_INITINFO(&ai);
	r = IOCTL(fd, AUDIO_GETINFO, &ai, "&ai");
	XP_SYS_EQ(0, r);
	XP_EQ(oai.play.gain,      ai.play.gain);
	XP_EQ(oai.play.balance,   ai.play.balance);
	XP_EQ(oai.record.gain,    ai.record.gain);
	XP_EQ(oai.record.balance, ai.record.balance);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

#define NENC	(AUDIO_ENCODING_AC3 + 1)
#define NPREC	(5)
/*
 * Make table of encoding+precision supported by this device.
 * Return last used index .
 * This function is called from test_AUDIO_GETENC_*()
 */
int
getenc_make_table(int fd, int expected[][5])
{
	audio_encoding_t ae;
	int idx;
	int p;
	int r;

	/*
	 * expected[][] is two dimensional table.
	 * encoding \ precision| 4  8  16  24  32
	 * --------------------+-----------------
	 * AUDIO_ENCODING_NONE |
	 * AUDIO_ENCODING_ULAW |
	 *  :
	 *
	 * Each cell has expected behavior.
	 *  0: the hardware doesn't support this encoding/precision.
	 *  1: the hardware supports this encoding/precision.
	 *  2: the hardware doesn't support this encoding/precision but
	 *     audio layer will respond as supported for compatibility.
	 */
	for (idx = 0; ; idx++) {
		memset(&ae, 0, sizeof(ae));
		ae.index = idx;
		r = IOCTL(fd, AUDIO_GETENC, &ae, "index=%d", idx);
		if (r != 0) {
			XP_SYS_NG(EINVAL, r);
			break;
		}

		XP_EQ(idx, ae.index);
		if (0 <= ae.encoding && ae.encoding <= AUDIO_ENCODING_AC3) {
			XP_EQ_STR(encoding_names[ae.encoding], ae.name);
		} else {
			XP_FAIL("ae.encoding %d", ae.encoding);
		}

		if (ae.precision != 4 &&
		    ae.precision != 8 &&
		    ae.precision != 16 &&
		    ae.precision != 24 &&
		    ae.precision != 32)
		{
			XP_FAIL("ae.precision %d", ae.precision);
		}
		/* Other bits should not be set */
		XP_EQ(0, (ae.flags & ~AUDIO_ENCODINGFLAG_EMULATED));

		expected[ae.encoding][ae.precision / 8] = 1;
		DPRINTF("  > encoding=%s precision=%d\n",
		    encoding_names[ae.encoding], ae.precision);
	}

	/*
	 * Backward compatibility bandaid.
	 *
	 * - Some encoding/precision pairs are obviously inconsistent
	 *   (e.g., encoding=AUDIO_ENCODING_PCM8, precision=16) but
	 *   it's due to historical reasons.
	 * - It's incomplete for NetBSD7 and NetBSD8.  I don't really
	 *   understand their rule...  This is just memo, not specification.
	 */
#define SET(x) do {	\
	if ((x) == 0)	\
		x = 2;	\
 } while (0)
#define p4 (0)
#define p8 (1)
#define p16 (2)
#define p24 (3)
#define p32 (4)

	if (expected[AUDIO_ENCODING_SLINEAR][p8]) {
		SET(expected[AUDIO_ENCODING_SLINEAR_LE][p8]);
		SET(expected[AUDIO_ENCODING_SLINEAR_BE][p8]);
	}
	if (expected[AUDIO_ENCODING_ULINEAR][p8]) {
		SET(expected[AUDIO_ENCODING_ULINEAR_LE][p8]);
		SET(expected[AUDIO_ENCODING_ULINEAR_BE][p8]);
		SET(expected[AUDIO_ENCODING_PCM8][p8]);
		SET(expected[AUDIO_ENCODING_PCM16][p8]);
	}
	for (p = p16; p <= p32; p++) {
#if !defined(AUDIO_SUPPORT_LINEAR24)
		if (p == p24)
			continue;
#endif
		if (expected[AUDIO_ENCODING_SLINEAR_NE][p]) {
			SET(expected[AUDIO_ENCODING_SLINEAR][p]);
			SET(expected[AUDIO_ENCODING_PCM16][p]);
		}
		if (expected[AUDIO_ENCODING_ULINEAR_NE][p]) {
			SET(expected[AUDIO_ENCODING_ULINEAR][p]);
		}
	}

	if (netbsd < 9) {
		if (expected[AUDIO_ENCODING_SLINEAR_LE][p16] ||
		    expected[AUDIO_ENCODING_SLINEAR_BE][p16] ||
		    expected[AUDIO_ENCODING_ULINEAR_LE][p16] ||
		    expected[AUDIO_ENCODING_ULINEAR_BE][p16])
		{
			SET(expected[AUDIO_ENCODING_PCM8][p8]);
			SET(expected[AUDIO_ENCODING_PCM16][p8]);
			SET(expected[AUDIO_ENCODING_SLINEAR_LE][p8]);
			SET(expected[AUDIO_ENCODING_SLINEAR_BE][p8]);
			SET(expected[AUDIO_ENCODING_ULINEAR_LE][p8]);
			SET(expected[AUDIO_ENCODING_ULINEAR_BE][p8]);
			SET(expected[AUDIO_ENCODING_SLINEAR][p8]);
			SET(expected[AUDIO_ENCODING_ULINEAR][p8]);
		}
	}

	/* Return last used index */
	return idx;
#undef SET
#undef p4
#undef p8
#undef p16
#undef p24
#undef p32
}

/*
 * This function is called from test_AUDIO_GETENC below.
 */
void
xp_getenc(int expected[][5], int enc, int j, int r, struct audio_prinfo *pr)
{
	int prec = (j == 0) ? 4 : j * 8;

	if (expected[enc][j]) {
		/* expect to succeed */
		XP_SYS_EQ(0, r);

		XP_EQ(enc, pr->encoding);
		XP_EQ(prec, pr->precision);
	} else {
		/* expect to fail */
		XP_SYS_NG(EINVAL, r);
	}
}

/*
 * This function is called from test_AUDIO_GETENC below.
 */
void
getenc_check_encodings(int openmode, int expected[][5])
{
	struct audio_info ai;
	int fd;
	int i, j;
	int r;

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	for (i = 0; i < NENC; i++) {
		for (j = 0; j < NPREC; j++) {
			/* precisions are 4 and 8, 16, 24, 32 */
			int prec = (j == 0) ? 4 : j * 8;

			/*
			 * AUDIO_GETENC has no way to know range of
			 * supported channels and sample_rate.
			 */
			AUDIO_INITINFO(&ai);
			ai.play.encoding = i;
			ai.play.precision = prec;
			ai.record.encoding = i;
			ai.record.precision = prec;

			r = IOCTL(fd, AUDIO_SETINFO, &ai, "%s:%d",
			    encoding_names[i], prec);
			if (mode2play(openmode))
				xp_getenc(expected, i, j, r, &ai.play);
			if (mode2rec(openmode))
				xp_getenc(expected, i, j, r, &ai.record);
		}
	}
	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * Check whether encoding+precision obtained by AUDIO_GETENC can be set.
 */
DEF(AUDIO_GETENC_range)
{
	audio_encoding_t ae;
	int fd;
	int r;
	int expected[NENC][NPREC];
	int i, j;

	TEST("AUDIO_GETENC_range");

	fd = OPEN(devaudio, openable_mode());
	REQUIRED_SYS_OK(fd);

	memset(&expected, 0, sizeof(expected));
	i = getenc_make_table(fd, expected);

	/* When error has occurred, the next index should also occur error */
	ae.index = i + 1;
	r = IOCTL(fd, AUDIO_GETENC, &ae, "index=%d", ae.index);
	XP_SYS_NG(EINVAL, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	/* For debug */
	if (debug) {
		for (i = 0; i < NENC; i++) {
			printf("expected[%2d] %15s", i, encoding_names[i]);
			for (j = 0; j < NPREC; j++) {
				printf(" %d", expected[i][j]);
			}
			printf("\n");
		}
	}

	/* Whether obtained encodings can be actually set */
	if (hw_fulldup()) {
		/* Test both R/W at once using single descriptor */
		getenc_check_encodings(O_RDWR, expected);
	} else {
		/* Test playback and recording if available */
		if (hw_canplay()) {
			getenc_check_encodings(O_WRONLY, expected);
		}
		if (hw_canplay() && hw_canrec()) {
			xxx_close_wait();
		}
		if (hw_canrec()) {
			getenc_check_encodings(O_RDONLY, expected);
		}
	}
}
#undef NENC
#undef NPREC

/*
 * Check AUDIO_GETENC out of range.
 */
DEF(AUDIO_GETENC_error)
{
	audio_encoding_t e;
	int fd;
	int r;

	TEST("AUDIO_GETENC_error");

	fd = OPEN(devaudio, openable_mode());
	REQUIRED_SYS_OK(fd);

	memset(&e, 0, sizeof(e));
	e.index = -1;
	r = IOCTL(fd, AUDIO_GETENC, &e, "index=-1");
	/* NetBSD7 may not fail depending on hardware driver */
	XP_SYS_NG(EINVAL, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * AUDIO_[PR]ERROR should be zero on the initial state even on non-existent
 * track.
 */
void
test_AUDIO_ERROR(int openmode)
{
	int fd;
	int r;
	int errors;

	TEST("AUDIO_ERROR_%s", openmode_str[openmode] + 2);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/* Check PERROR */
	errors = 0xdeadbeef;
	r = IOCTL(fd, AUDIO_PERROR, &errors, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, errors);

	/* Check RERROR */
	errors = 0xdeadbeef;
	r = IOCTL(fd, AUDIO_RERROR, &errors, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, errors);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(AUDIO_ERROR_RDONLY)	{ test_AUDIO_ERROR(O_RDONLY); }
DEF(AUDIO_ERROR_WRONLY)	{ test_AUDIO_ERROR(O_WRONLY); }
DEF(AUDIO_ERROR_RDWR)	{ test_AUDIO_ERROR(O_RDWR); }

/*
 * AUDIO_GETIOFFS at least one block.
 */
void
test_AUDIO_GETIOFFS_one(int openmode)
{
	struct audio_info ai;
	audio_offset_t o;
	int fd;
	int r;
	u_int blocksize;
	u_int blk_ms;

	TEST("AUDIO_GETIOFFS_one_%s", openmode_str[openmode] + 2);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

#if 0
	/*
	 * On NetBSD7/8, native encodings and emulated encodings behave
	 * differently.  But it's hard to identify which encoding is native.
	 * If you try other encodings, edit these parameters manually.
	 */
	AUDIO_INITINFO(&ai);
	ai.record.encoding = AUDIO_ENCODING_SLINEAR_NE;
	ai.record.precision = 16;
	ai.record.channels = 2;
	ai.record.sample_rate = 48000;
	/* ai.blocksize is shared by play and record, so set both the same. */
	*ai.play = *ai.record;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
#endif

	/* Get blocksize to calc blk_ms. */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	blocksize = ai.blocksize;
	if (netbsd < 9) {
		blk_ms = 0;
	} else {
		/* On NetBSD9, blocktime can always be calculated. */
		blk_ms = blocksize * 1000 /
		    (ai.play.precision / 8 * ai.play.channels *
		     ai.play.sample_rate);
	}
	if (blk_ms == 0)
		blk_ms = 50;
	DPRINTF("  > blocksize=%u, estimated blk_ms=%u\n", blocksize, blk_ms);

	/*
	 * Even when just opened, recording counters will start.
	 * Wait a moment, about one block time.
	 */
	usleep(blk_ms * 1000);

	r = IOCTL(fd, AUDIO_GETIOFFS, &o, "");
	XP_SYS_EQ(0, r);
	if (mode2rec(openmode)) {
		/*
		 * It's difficult to know exact values.
		 * But at least these should not be zero.
		 */
		DPRINTF("  > %d: samples=%u deltablks=%u offset=%u\n",
		    __LINE__, o.samples, o.deltablks, o.offset);
		XP_NE(0, o.samples);
		XP_NE(0, o.deltablks);
		XP_NE(0, o.offset);
	} else {
		/* All are zero on playback track. */
		XP_EQ(0, o.samples);
		XP_EQ(0, o.deltablks);
		XP_EQ(0, o.offset);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(AUDIO_GETIOFFS_one_RDONLY) { test_AUDIO_GETIOFFS_one(O_RDONLY); }
DEF(AUDIO_GETIOFFS_one_WRONLY) { test_AUDIO_GETIOFFS_one(O_WRONLY); }
DEF(AUDIO_GETIOFFS_one_RDWR)   { test_AUDIO_GETIOFFS_one(O_RDWR); }

/*
 * AUDIO_GETOOFFS for one block.
 */
void
test_AUDIO_GETOOFFS_one(int openmode)
{
	struct audio_info ai;
	audio_offset_t o;
	char *buf;
	int fd;
	int r;
	u_int blocksize;
	u_int initial_offset;
	u_int blk_ms;

	TEST("AUDIO_GETOOFFS_one_%s", openmode_str[openmode] + 2);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

#if 0
	/*
	 * On NetBSD7/8, native encodings and emulated encodings behave
	 * differently.  But it's hard to identify which encoding is native.
	 * If you try other encodings, edit these parameters manually.
	 */
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_NE;
	ai.play.precision = 16;
	ai.play.channels = 2;
	ai.play.sample_rate = 48000;
	/* ai.blocksize is shared by play and record, so set both the same. */
	*ai.record = *ai.play;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "slinear16/2ch/48000");
	REQUIRED_SYS_EQ(0, r);
#endif

	/* Get blocksize to calc blk_ms. */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	blocksize = ai.blocksize;
	if (netbsd < 9) {
		blk_ms = 0;
	} else {
		/* On NetBSD9, blocktime can always be calculated. */
		blk_ms = blocksize * 1000 /
		    (ai.play.precision / 8 * ai.play.channels *
		     ai.play.sample_rate);
	}
	if (blk_ms == 0)
		blk_ms = 50;
	DPRINTF("  > blocksize=%u, estimated blk_ms=%u\n", blocksize, blk_ms);

	buf = (char *)malloc(blocksize);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, blocksize);

	/*
	 * On NetBSD7, .offset starts from one block.  What is the block??
	 * On NetBSD9, .offset starts from zero.
	 */
	if (netbsd < 9) {
		initial_offset = blocksize;
	} else {
		initial_offset = 0;
	}

	/* When just opened, all are zero. */
	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, o.samples);
	XP_EQ(0, o.deltablks);
	XP_EQ(initial_offset, o.offset);

	/* Even if wait (at least) one block, these remain unchanged. */
	usleep(blk_ms * 1000);
	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, o.samples);
	XP_EQ(0, o.deltablks);
	XP_EQ(initial_offset, o.offset);

	/* Write one block. */
	r = WRITE(fd, buf, blocksize);
	if (mode2play(openmode)) {
		XP_SYS_EQ(blocksize, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}
	r = IOCTL(fd, AUDIO_DRAIN, NULL, "");
	REQUIRED_SYS_EQ(0, r);

	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	if (mode2play(openmode)) {
		/* All advance one block. */
		XP_EQ(blocksize, o.samples);
		XP_EQ(1, o.deltablks);
		XP_EQ(initial_offset + blocksize, o.offset);
	} else {
		/*
		 * All are zero on non-play track.
		 * On NetBSD7, the rec track has play buffer, too.
		 */
		XP_EQ(0, o.samples);
		XP_EQ(0, o.deltablks);
		XP_EQ(initial_offset, o.offset);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	free(buf);
}
DEF(AUDIO_GETOOFFS_one_RDONLY) { test_AUDIO_GETOOFFS_one(O_RDONLY); }
DEF(AUDIO_GETOOFFS_one_WRONLY) { test_AUDIO_GETOOFFS_one(O_WRONLY); }
DEF(AUDIO_GETOOFFS_one_RDWR)   { test_AUDIO_GETOOFFS_one(O_RDWR); }

/*
 * AUDIO_GETOOFFS when wrap around buffer.
 */
void
test_AUDIO_GETOOFFS_wrap(int openmode)
{
	struct audio_info ai;
	audio_offset_t o;
	char *buf;
	int fd;
	int r;
	u_int blocksize;
	u_int buffer_size;
	u_int initial_offset;
	u_int nblks;

	TEST("AUDIO_GETOOFFS_wrap_%s", openmode_str[openmode] + 2);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

#if 1
	/* To save test time, use larger format if possible. */
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_NE;
	ai.play.precision = 16;
	ai.play.channels = 2;
	ai.play.sample_rate = 48000;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "slinear16/2/48000");
	if (r != 0)
#endif
	{
		/*
		 * If it cannot be set, use common format instead.
		 * May be happened on NetBSD7/8.
		 */
		AUDIO_INITINFO(&ai);
		ai.play.encoding = AUDIO_ENCODING_ULAW;
		ai.play.precision = 8;
		ai.play.channels = 1;
		ai.play.sample_rate = 8000;
		r = IOCTL(fd, AUDIO_SETINFO, &ai, "ulaw/1/8000");
	}
	REQUIRED_SYS_EQ(0, r);

	/* Get buffer_size and blocksize. */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	buffer_size = ai.play.buffer_size;
	blocksize = ai.blocksize;
	nblks = buffer_size / blocksize;
	DPRINTF("  > buffer_size=%u blocksize=%u nblks=%u\n",
	    buffer_size, blocksize, nblks);

	buf = (char *)malloc(buffer_size);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, buffer_size);

	/*
	 * On NetBSD7, .offset starts from one block.  What is the block??
	 * On NetBSD9, .offset starts from zero.
	 */
	if (netbsd < 9) {
		initial_offset = blocksize;
	} else {
		initial_offset = 0;
	}

	/* Write full buffer. */
	r = WRITE(fd, buf, buffer_size);
	if (mode2play(openmode)) {
		XP_SYS_EQ(buffer_size, r);

		/* Then, wait. */
		r = IOCTL(fd, AUDIO_DRAIN, NULL, "");
		REQUIRED_SYS_EQ(0, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}

	/*
	 * .deltablks is number of blocks since last checked.
	 * .offset is wrapped around to zero.
	 */
	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	if (mode2play(openmode)) {
		/*
		 * On NetBSD7, samples may be blocksize * nblks or buffer_size
		 * depending on native/emulated encoding.
		 * On NetBSD9, samples is always equal to buffer_size.
		 */
		if (buffer_size != blocksize * nblks &&
		    o.samples == blocksize * nblks) {
			DPRINTF("  > %d: samples(%u) == blocksize * nblks\n",
			    __LINE__, o.samples);
		} else {
			XP_EQ(buffer_size, o.samples);
		}
		XP_EQ(nblks, o.deltablks);
		XP_EQ(initial_offset, o.offset);
	} else {
		/*
		 * On non-play track, it silently succeeds with zero.
		 * But on NetBSD7, RDONLY descriptor also has play buffer.
		 */
		XP_EQ(0, o.samples);
		XP_EQ(0, o.deltablks);
		XP_EQ(initial_offset, o.offset);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	free(buf);
}
DEF(AUDIO_GETOOFFS_wrap_RDONLY) { test_AUDIO_GETOOFFS_wrap(O_RDONLY); }
DEF(AUDIO_GETOOFFS_wrap_WRONLY) { test_AUDIO_GETOOFFS_wrap(O_WRONLY); }
DEF(AUDIO_GETOOFFS_wrap_RDWR)   { test_AUDIO_GETOOFFS_wrap(O_RDWR); }

/*
 * Check whether AUDIO_FLUSH clears AUDIO_GETOOFFS.
 */
void
test_AUDIO_GETOOFFS_flush(int openmode)
{
	struct audio_info ai;
	audio_offset_t o;
	char *buf;
	int fd;
	int r;
	u_int initial_offset;
	u_int last_offset;

	TEST("AUDIO_GETOOFFS_flush_%s", openmode_str[openmode] + 2);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

#if 0
	/* On NetBSD7/8, native encoding changes buffer behavior. */
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_NE;
	ai.play.precision = 16;
	ai.play.channels = 2;
	ai.play.sample_rate = 48000;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
#endif

	/* Get blocksize. */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);

	buf = (char *)malloc(ai.blocksize);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, ai.blocksize);

	/*
	 * On NetBSD7, .offset starts from one block.  What is the block??
	 * On NetBSD9, .offset starts from zero.
	 */
	if (netbsd < 9) {
		initial_offset = ai.blocksize;
	} else {
		initial_offset = 0;
	}

	/* Write one block. */
	r = WRITE(fd, buf, ai.blocksize);
	if (mode2play(openmode)) {
		XP_SYS_EQ(ai.blocksize, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}
	r = IOCTL(fd, AUDIO_DRAIN, NULL, "");
	XP_SYS_EQ(0, r);

	/* Obtain once. */
	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	if (mode2play(openmode)) {
		XP_EQ(ai.blocksize, o.samples);
		XP_EQ(1, o.deltablks);
		XP_EQ(initial_offset + ai.blocksize, o.offset);
	} else {
		/*
		 * On non-play track, it silently succeeds with zero.
		 * But on NetBSD7, RDONLY descriptor also has play buffer.
		 */
		XP_EQ(0, o.samples);
		XP_EQ(0, o.deltablks);
		XP_EQ(initial_offset, o.offset);
	}

	/* Write one more block to advance .offset. */
	r = WRITE(fd, buf, ai.blocksize);
	if (mode2play(openmode)) {
		XP_SYS_EQ(ai.blocksize, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}
	r = IOCTL(fd, AUDIO_DRAIN, NULL, "");
	XP_SYS_EQ(0, r);

	/* If offset remains unchanged, this is expected offset. */
	last_offset = initial_offset + ai.blocksize * 2;

	/* Then, flush. */
	r = IOCTL(fd, AUDIO_FLUSH, NULL, "");
	REQUIRED_SYS_EQ(0, r);

	/* All should be cleared. */
	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, o.samples);
	XP_EQ(0, o.deltablks);
	if (mode2play(openmode)) {
		/*
		 * On NetBSD7,
		 * offset is cleared if native encodings(?), but remains
		 * unchanged if emulated encodings(?).  Looks a bug.
		 * On NetBSD9, it should always be cleared.
		 */
		if (netbsd < 9 && o.offset == last_offset) {
			DPRINTF("  > %d: offset(%u) == last_offset\n",
			    __LINE__, o.offset);
		} else {
			XP_EQ(initial_offset, o.offset);
		}
	} else {
		XP_EQ(initial_offset, o.offset);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	free(buf);
}
DEF(AUDIO_GETOOFFS_flush_RDONLY) { test_AUDIO_GETOOFFS_flush(O_RDONLY); }
DEF(AUDIO_GETOOFFS_flush_WRONLY) { test_AUDIO_GETOOFFS_flush(O_WRONLY); }
DEF(AUDIO_GETOOFFS_flush_RDWR)   { test_AUDIO_GETOOFFS_flush(O_RDWR); }

/*
 * Check whether AUDIO_SETINFO(encoding) clears AUDIO_GETOOFFS.
 */
void
test_AUDIO_GETOOFFS_set(int openmode)
{
	struct audio_info ai;
	audio_offset_t o;
	char *buf;
	int fd;
	int r;
	u_int initial_offset;

	TEST("AUDIO_GETOOFFS_set_%s", openmode_str[openmode] + 2);
	if (mode2aumode(openmode) == 0) {
		XP_SKIP("Operation not allowed on this hardware property");
		return;
	}

	fd = OPEN(devaudio, openmode);
	REQUIRED_SYS_OK(fd);

	/* Get blocksize. */
	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	XP_SYS_EQ(0, r);

	buf = (char *)malloc(ai.blocksize);
	REQUIRED_IF(buf != NULL);
	memset(buf, 0xff, ai.blocksize);

	/*
	 * On NetBSD7, .offset starts from one block.  What is the block??
	 * On NetBSD9, .offset starts from zero.
	 */
	if (netbsd < 9) {
		initial_offset = ai.blocksize;
	} else {
		initial_offset = 0;
	}

	/* Write one block. */
	r = WRITE(fd, buf, ai.blocksize);
	if (mode2play(openmode)) {
		XP_SYS_EQ(ai.blocksize, r);
	} else {
		XP_SYS_NG(EBADF, r);
	}
	r = IOCTL(fd, AUDIO_DRAIN, NULL, "");
	XP_SYS_EQ(0, r);

	/*
	 * Then, change encoding.
	 * If we fail to change it, we cannot continue.  This may happen
	 * on NetBSD7/8.
	 */
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_NE;
	ai.play.precision = 16;
	ai.play.channels = 2;
	ai.play.sample_rate = 48000;
	r = IOCTL(fd, AUDIO_SETINFO, &ai, "slinear16/2ch/48000");
	REQUIRED_SYS_EQ(0, r);

	r = IOCTL(fd, AUDIO_GETBUFINFO, &ai, "");
	REQUIRED_SYS_EQ(0, r);
	if (netbsd < 9) {
		initial_offset = ai.blocksize;
	} else {
		initial_offset = 0;
	}

	/* Clear counters? */
	r = IOCTL(fd, AUDIO_GETOOFFS, &o, "");
	XP_SYS_EQ(0, r);
	XP_EQ(0, o.samples);
	XP_EQ(0, o.deltablks);
	XP_EQ(initial_offset, o.offset);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	free(buf);
}
DEF(AUDIO_GETOOFFS_set_RDONLY) { test_AUDIO_GETOOFFS_set(O_RDONLY); }
DEF(AUDIO_GETOOFFS_set_WRONLY) { test_AUDIO_GETOOFFS_set(O_WRONLY); }
DEF(AUDIO_GETOOFFS_set_RDWR)   { test_AUDIO_GETOOFFS_set(O_RDWR); }

/*
 * /dev/audioctl can always be opened while /dev/audio is open.
 */
void
test_audioctl_open_1(int fmode, int cmode)
{
	int fd;
	int ctl;
	int r;

	TEST("audioctl_open_1_%s_%s",
	    openmode_str[fmode] + 2, openmode_str[cmode] + 2);
	if (hw_canplay() == 0 && fmode == O_WRONLY) {
		XP_SKIP("This test is for playable device");
		return;
	}
	if (hw_canrec() == 0 && fmode == O_RDONLY) {
		XP_SKIP("This test is for recordable device");
		return;
	}

	fd = OPEN(devaudio, fmode);
	REQUIRED_SYS_OK(fd);

	ctl = OPEN(devaudioctl, cmode);
	XP_SYS_OK(ctl);

	r = CLOSE(ctl);
	XP_SYS_EQ(0, r);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(audioctl_open_1_RDONLY_RDONLY) { test_audioctl_open_1(O_RDONLY, O_RDONLY); }
DEF(audioctl_open_1_RDONLY_RWONLY) { test_audioctl_open_1(O_RDONLY, O_WRONLY); }
DEF(audioctl_open_1_RDONLY_RDWR)   { test_audioctl_open_1(O_RDONLY, O_RDWR); }
DEF(audioctl_open_1_WRONLY_RDONLY) { test_audioctl_open_1(O_WRONLY, O_RDONLY); }
DEF(audioctl_open_1_WRONLY_RWONLY) { test_audioctl_open_1(O_WRONLY, O_WRONLY); }
DEF(audioctl_open_1_WRONLY_RDWR)   { test_audioctl_open_1(O_WRONLY, O_RDWR); }
DEF(audioctl_open_1_RDWR_RDONLY)   { test_audioctl_open_1(O_RDWR, O_RDONLY); }
DEF(audioctl_open_1_RDWR_RWONLY)   { test_audioctl_open_1(O_RDWR, O_WRONLY); }
DEF(audioctl_open_1_RDWR_RDWR)     { test_audioctl_open_1(O_RDWR, O_RDWR); }

/*
 * /dev/audio can always be opened while /dev/audioctl is open.
 */
void
test_audioctl_open_2(int fmode, int cmode)
{
	int fd;
	int ctl;
	int r;

	TEST("audioctl_open_2_%s_%s",
	    openmode_str[fmode] + 2, openmode_str[cmode] + 2);
	if (hw_canplay() == 0 && fmode == O_WRONLY) {
		XP_SKIP("This test is for playable device");
		return;
	}
	if (hw_canrec() == 0 && fmode == O_RDONLY) {
		XP_SKIP("This test is for recordable device");
		return;
	}

	ctl = OPEN(devaudioctl, cmode);
	REQUIRED_SYS_OK(ctl);

	fd = OPEN(devaudio, fmode);
	XP_SYS_OK(fd);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);

	r = CLOSE(ctl);
	XP_SYS_EQ(0, r);
}
DEF(audioctl_open_2_RDONLY_RDONLY) { test_audioctl_open_2(O_RDONLY, O_RDONLY); }
DEF(audioctl_open_2_RDONLY_RWONLY) { test_audioctl_open_2(O_RDONLY, O_WRONLY); }
DEF(audioctl_open_2_RDONLY_RDWR)   { test_audioctl_open_2(O_RDONLY, O_RDWR); }
DEF(audioctl_open_2_WRONLY_RDONLY) { test_audioctl_open_2(O_WRONLY, O_RDONLY); }
DEF(audioctl_open_2_WRONLY_RWONLY) { test_audioctl_open_2(O_WRONLY, O_WRONLY); }
DEF(audioctl_open_2_WRONLY_RDWR)   { test_audioctl_open_2(O_WRONLY, O_RDWR); }
DEF(audioctl_open_2_RDWR_RDONLY)   { test_audioctl_open_2(O_RDWR, O_RDONLY); }
DEF(audioctl_open_2_RDWR_RWONLY)   { test_audioctl_open_2(O_RDWR, O_WRONLY); }
DEF(audioctl_open_2_RDWR_RDWR)     { test_audioctl_open_2(O_RDWR, O_RDWR); }

/*
 * Open multiple /dev/audioctl.
 */
DEF(audioctl_open_simul)
{
	int ctl0;
	int ctl1;
	int r;

	TEST("audioctl_open_simul");

	ctl0 = OPEN(devaudioctl, O_RDWR);
	REQUIRED_SYS_OK(ctl0);

	ctl1 = OPEN(devaudioctl, O_RDWR);
	XP_SYS_OK(ctl1);

	r = CLOSE(ctl0);
	XP_SYS_EQ(0, r);

	r = CLOSE(ctl1);
	XP_SYS_EQ(0, r);
}

/*
 * /dev/audioctl can be opened by other user who opens /dev/audioctl,
 * /dev/audioctl can be opened by other user who opens /dev/audio,
 * /dev/audio    can be opened by other user who opens /dev/audioctl,
 * regardless of multiuser mode.
 */
void
try_audioctl_open_multiuser(const char *dev1, const char *dev2)
{
	int fd1;
	int fd2;
	int r;
	uid_t ouid;

	/*
	 * At first, open dev1 as root.
	 * And then open dev2 as unprivileged user.
	 */

	fd1 = OPEN(dev1, O_RDWR);
	REQUIRED_SYS_OK(fd1);

	ouid = GETUID();
	r = SETEUID(1);
	REQUIRED_SYS_EQ(0, r);

	fd2 = OPEN(dev2, O_RDWR);
	XP_SYS_OK(fd2);

	/* Close */
	r = CLOSE(fd2);
	XP_SYS_EQ(0, r);

	r = SETEUID(ouid);
	REQUIRED_SYS_EQ(0, r);

	r = CLOSE(fd1);
	XP_SYS_EQ(0, r);
}
/*
 * This is a wrapper for audioctl_open_multiuser.
 * XXX XP_* macros are not compatible with on-error-goto, we need try-catch...
 */
void
test_audioctl_open_multiuser(bool multiuser,
	const char *dev1, const char *dev2)
{
	char mibname[32];
	bool oldval;
	size_t oldlen;
	int r;

	if (netbsd < 8 && multiuser == 1) {
		XP_SKIP("multiuser is not supported");
		return;
	}
	if (netbsd < 9) {
		/* NetBSD8 has no way (difficult) to determine device name */
		XP_SKIP("NetBSD8 cannot determine device name");
		return;
	}
	if (geteuid() != 0) {
		XP_SKIP("This test must be priviledged user");
		return;
	}

	/* Get current multiuser mode (and save it) */
	snprintf(mibname, sizeof(mibname), "hw.%s.multiuser", devicename);
	oldlen = sizeof(oldval);
	r = SYSCTLBYNAME(mibname, &oldval, &oldlen, NULL, 0);
	REQUIRED_SYS_EQ(0, r);
	DPRINTF("  > multiuser=%d\n", oldval);

	/* Change if necessary */
	if (oldval != multiuser) {
		r = SYSCTLBYNAME(mibname, NULL, NULL, &multiuser,
		    sizeof(multiuser));
		REQUIRED_SYS_EQ(0, r);
		DPRINTF("  > new multiuser=%d\n", multiuser);
	}

	/* Do test */
	try_audioctl_open_multiuser(dev1, dev2);

	/* Restore multiuser mode */
	if (oldval != multiuser) {
		DPRINTF("  > restore multiuser to %d\n", oldval);
		r = SYSCTLBYNAME(mibname, NULL, NULL, &oldval, sizeof(oldval));
		XP_SYS_EQ(0, r);
	}
}
DEF(audioctl_open_multiuser0_audio1) {
	TEST("audioctl_open_multiuser0_audio1");
	test_audioctl_open_multiuser(false, devaudio, devaudioctl);
}
DEF(audioctl_open_multiuser1_audio1) {
	TEST("audioctl_open_multiuser1_audio1");
	test_audioctl_open_multiuser(true, devaudio, devaudioctl);
}
DEF(audioctl_open_multiuser0_audio2) {
	TEST("audioctl_open_multiuser0_audio2");
	test_audioctl_open_multiuser(false, devaudioctl, devaudio);
}
DEF(audioctl_open_multiuser1_audio2) {
	TEST("audioctl_open_multiuser1_audio2");
	test_audioctl_open_multiuser(true, devaudioctl, devaudio);
}
DEF(audioctl_open_multiuser0_audioctl) {
	TEST("audioctl_open_multiuser0_audioctl");
	test_audioctl_open_multiuser(false, devaudioctl, devaudioctl);
}
DEF(audioctl_open_multiuser1_audioctl) {
	TEST("audioctl_open_multiuser1_audioctl");
	test_audioctl_open_multiuser(true, devaudioctl, devaudioctl);
}

/*
 * /dev/audioctl cannot be read/written regardless of its open mode.
 */
void
test_audioctl_rw(int openmode)
{
	char buf[1];
	int fd;
	int r;

	TEST("audioctl_rw_%s", openmode_str[openmode] + 2);

	fd = OPEN(devaudioctl, openmode);
	REQUIRED_SYS_OK(fd);

	if (mode2play(openmode)) {
		r = WRITE(fd, buf, sizeof(buf));
		XP_SYS_NG(ENODEV, r);
	}

	if (mode2rec(openmode)) {
		r = READ(fd, buf, sizeof(buf));
		XP_SYS_NG(ENODEV, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}
DEF(audioctl_rw_RDONLY)	{ test_audioctl_rw(O_RDONLY); }
DEF(audioctl_rw_WRONLY)	{ test_audioctl_rw(O_WRONLY); }
DEF(audioctl_rw_RDWR)	{ test_audioctl_rw(O_RDWR); }

/*
 * poll(2) for /dev/audioctl should never raise.
 * I'm not sure about consistency between poll(2) and kqueue(2) but
 * anyway I follow it.
 * XXX Omit checking each openmode
 */
DEF(audioctl_poll)
{
	struct pollfd pfd;
	int fd;
	int r;

	TEST("audioctl_poll");

	fd = OPEN(devaudioctl, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	pfd.fd = fd;
	pfd.events = POLLOUT;
	r = POLL(&pfd, 1, 100);
	XP_SYS_EQ(0, r);
	XP_EQ(0, pfd.revents);

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}

/*
 * kqueue(2) for /dev/audioctl fails.
 * I'm not sure about consistency between poll(2) and kqueue(2) but
 * anyway I follow it.
 * XXX Omit checking each openmode
 */
DEF(audioctl_kqueue)
{
	struct kevent kev;
	int fd;
	int kq;
	int r;

	TEST("audioctl_kqueue");

	fd = OPEN(devaudioctl, O_WRONLY);
	REQUIRED_SYS_OK(fd);

	kq = KQUEUE();
	XP_SYS_OK(kq);

	EV_SET(&kev, fd, EVFILT_WRITE, EV_ADD, 0, 0, 0);
	r = KEVENT_SET(kq, &kev, 1);
	/*
	 * NetBSD7 has a bug.  It looks to wanted to treat it as successful
	 * but returned 1(== EPERM).
	 * On NetBSD9, I decided to return ENODEV.
	 */
	if (netbsd < 8) {
		XP_SYS_NG(1/*EPERM*/, r);
	} else {
		XP_SYS_NG(ENODEV, r);
	}

	r = CLOSE(fd);
	XP_SYS_EQ(0, r);
}


/*
 * This table is processed by t_audio.awk!
 * Keep /^\tENT(testname),/ format in order to add to atf.
 */
#define ENT(x) { #x, test__ ## x }
struct testentry testtable[] = {
	ENT(open_mode_RDONLY),
	ENT(open_mode_WRONLY),
	ENT(open_mode_RDWR),
	ENT(open_audio_RDONLY),
	ENT(open_audio_WRONLY),
	ENT(open_audio_RDWR),
	ENT(open_sound_RDONLY),
	ENT(open_sound_WRONLY),
	ENT(open_sound_RDWR),
	ENT(open_audioctl_RDONLY),
	ENT(open_audioctl_WRONLY),
	ENT(open_audioctl_RDWR),
	ENT(open_sound_sticky),
	ENT(open_audioctl_sticky),
	ENT(open_simul_RDONLY_RDONLY),
	ENT(open_simul_RDONLY_WRONLY),
	ENT(open_simul_RDONLY_RDWR),
	ENT(open_simul_WRONLY_RDONLY),
	ENT(open_simul_WRONLY_WRONLY),
	ENT(open_simul_WRONLY_RDWR),
	ENT(open_simul_RDWR_RDONLY),
	ENT(open_simul_RDWR_WRONLY),
	ENT(open_simul_RDWR_RDWR),
/**/	ENT(open_multiuser_0),		// XXX TODO sysctl
/**/	ENT(open_multiuser_1),		// XXX TODO sysctl
	ENT(write_PLAY_ALL),
	ENT(write_PLAY),
	ENT(read),
	ENT(rept_write),
	ENT(rept_read),
	ENT(rdwr_fallback_RDONLY),
	ENT(rdwr_fallback_WRONLY),
	ENT(rdwr_fallback_RDWR),
	ENT(rdwr_two_RDONLY_RDONLY),
	ENT(rdwr_two_RDONLY_WRONLY),
	ENT(rdwr_two_RDONLY_RDWR),
	ENT(rdwr_two_WRONLY_RDONLY),
	ENT(rdwr_two_WRONLY_WRONLY),
	ENT(rdwr_two_WRONLY_RDWR),
	ENT(rdwr_two_RDWR_RDONLY),
	ENT(rdwr_two_RDWR_WRONLY),
	ENT(rdwr_two_RDWR_RDWR),
	ENT(rdwr_simul),
	ENT(drain_incomplete),
	ENT(drain_pause),
	ENT(drain_onrec),
/**/	ENT(mmap_mode_RDONLY_NONE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDONLY_READ),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDONLY_WRITE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDONLY_READWRITE),// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_WRONLY_NONE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_WRONLY_READ),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_WRONLY_WRITE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_WRONLY_READWRITE),// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDWR_NONE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDWR_READ),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDWR_WRITE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_mode_RDWR_READWRITE),	// XXX rump doesn't supprot mmap
/**/	ENT(mmap_len),			// XXX rump doesn't supprot mmap
/**/	ENT(mmap_twice),		// XXX rump doesn't supprot mmap
/**/	ENT(mmap_multi),		// XXX rump doesn't supprot mmap
	ENT(poll_mode_RDONLY_IN),
	ENT(poll_mode_RDONLY_OUT),
	ENT(poll_mode_RDONLY_INOUT),
	ENT(poll_mode_WRONLY_IN),
	ENT(poll_mode_WRONLY_OUT),
	ENT(poll_mode_WRONLY_INOUT),
	ENT(poll_mode_RDWR_IN),
	ENT(poll_mode_RDWR_OUT),
	ENT(poll_mode_RDWR_INOUT),
	ENT(poll_out_empty),
	ENT(poll_out_full),
	ENT(poll_out_hiwat),
/**/	ENT(poll_out_unpause),		// XXX does not seem to work on rump
/**/	ENT(poll_out_simul),		// XXX does not seem to work on rump
	ENT(poll_in_open_audio),
	ENT(poll_in_open_sound),
	ENT(poll_in_open_audioctl),
	ENT(poll_in_simul),
	ENT(kqueue_mode_RDONLY_READ),
	ENT(kqueue_mode_RDONLY_WRITE),
	ENT(kqueue_mode_WRONLY_READ),
	ENT(kqueue_mode_WRONLY_WRITE),
	ENT(kqueue_mode_RDWR_READ),
	ENT(kqueue_mode_RDWR_WRITE),
	ENT(kqueue_empty),
	ENT(kqueue_full),
	ENT(kqueue_hiwat),
/**/	ENT(kqueue_unpause),		// XXX does not seem to work on rump
/**/	ENT(kqueue_simul),		// XXX does not seem to work on rump
	ENT(ioctl_while_write),
	ENT(FIOASYNC_reset),
	ENT(FIOASYNC_play_signal),
	ENT(FIOASYNC_rec_signal),
/**/	ENT(FIOASYNC_multi),		// XXX does not seem to work on rump
	ENT(AUDIO_WSEEK),
	ENT(AUDIO_SETFD_RDONLY),
	ENT(AUDIO_SETFD_WRONLY),
	ENT(AUDIO_SETFD_RDWR),
	ENT(AUDIO_GETINFO_eof),
	ENT(AUDIO_SETINFO_mode_RDONLY_0),
	ENT(AUDIO_SETINFO_mode_RDONLY_1),
	ENT(AUDIO_SETINFO_mode_RDONLY_2),
	ENT(AUDIO_SETINFO_mode_RDONLY_3),
	ENT(AUDIO_SETINFO_mode_RDONLY_4),
	ENT(AUDIO_SETINFO_mode_RDONLY_5),
	ENT(AUDIO_SETINFO_mode_RDONLY_6),
	ENT(AUDIO_SETINFO_mode_RDONLY_7),
	ENT(AUDIO_SETINFO_mode_RDONLY_8),
	ENT(AUDIO_SETINFO_mode_WRONLY_0),
	ENT(AUDIO_SETINFO_mode_WRONLY_1),
	ENT(AUDIO_SETINFO_mode_WRONLY_2),
	ENT(AUDIO_SETINFO_mode_WRONLY_3),
	ENT(AUDIO_SETINFO_mode_WRONLY_4),
	ENT(AUDIO_SETINFO_mode_WRONLY_5),
	ENT(AUDIO_SETINFO_mode_WRONLY_6),
	ENT(AUDIO_SETINFO_mode_WRONLY_7),
	ENT(AUDIO_SETINFO_mode_WRONLY_8),
	ENT(AUDIO_SETINFO_mode_RDWR_0),
	ENT(AUDIO_SETINFO_mode_RDWR_1),
	ENT(AUDIO_SETINFO_mode_RDWR_2),
	ENT(AUDIO_SETINFO_mode_RDWR_3),
	ENT(AUDIO_SETINFO_mode_RDWR_4),
	ENT(AUDIO_SETINFO_mode_RDWR_5),
	ENT(AUDIO_SETINFO_mode_RDWR_6),
	ENT(AUDIO_SETINFO_mode_RDWR_7),
	ENT(AUDIO_SETINFO_mode_RDWR_8),
	ENT(AUDIO_SETINFO_params_set_RDONLY_0),
	ENT(AUDIO_SETINFO_params_set_RDONLY_1),
	ENT(AUDIO_SETINFO_params_set_WRONLY_0),
	ENT(AUDIO_SETINFO_params_set_WRONLY_1),
	ENT(AUDIO_SETINFO_params_set_WRONLY_2),
	ENT(AUDIO_SETINFO_params_set_WRONLY_3),
	ENT(AUDIO_SETINFO_params_set_RDWR_0),
	ENT(AUDIO_SETINFO_params_set_RDWR_1),
	ENT(AUDIO_SETINFO_params_set_RDWR_2),
	ENT(AUDIO_SETINFO_params_set_RDWR_3),
	ENT(AUDIO_SETINFO_params_simul),
	ENT(AUDIO_SETINFO_channels),
	ENT(AUDIO_SETINFO_sample_rate),
	ENT(AUDIO_SETINFO_sample_rate_0),
	ENT(AUDIO_SETINFO_pause_RDONLY_0),
	ENT(AUDIO_SETINFO_pause_RDONLY_1),
	ENT(AUDIO_SETINFO_pause_WRONLY_0),
	ENT(AUDIO_SETINFO_pause_WRONLY_1),
	ENT(AUDIO_SETINFO_pause_WRONLY_2),
	ENT(AUDIO_SETINFO_pause_WRONLY_3),
	ENT(AUDIO_SETINFO_pause_RDWR_0),
	ENT(AUDIO_SETINFO_pause_RDWR_1),
	ENT(AUDIO_SETINFO_pause_RDWR_2),
	ENT(AUDIO_SETINFO_pause_RDWR_3),
	ENT(AUDIO_SETINFO_gain),
	ENT(AUDIO_SETINFO_gain_balance),
	ENT(AUDIO_GETENC_range),
	ENT(AUDIO_GETENC_error),
	ENT(AUDIO_ERROR_RDONLY),
	ENT(AUDIO_ERROR_WRONLY),
	ENT(AUDIO_ERROR_RDWR),
	ENT(AUDIO_GETIOFFS_one_RDONLY),
	ENT(AUDIO_GETIOFFS_one_WRONLY),
	ENT(AUDIO_GETIOFFS_one_RDWR),
	ENT(AUDIO_GETOOFFS_one_RDONLY),
	ENT(AUDIO_GETOOFFS_one_WRONLY),
	ENT(AUDIO_GETOOFFS_one_RDWR),
	ENT(AUDIO_GETOOFFS_wrap_RDONLY),
	ENT(AUDIO_GETOOFFS_wrap_WRONLY),
	ENT(AUDIO_GETOOFFS_wrap_RDWR),
	ENT(AUDIO_GETOOFFS_flush_RDONLY),
	ENT(AUDIO_GETOOFFS_flush_WRONLY),
	ENT(AUDIO_GETOOFFS_flush_RDWR),
	ENT(AUDIO_GETOOFFS_set_RDONLY),
	ENT(AUDIO_GETOOFFS_set_WRONLY),
	ENT(AUDIO_GETOOFFS_set_RDWR),
	ENT(audioctl_open_1_RDONLY_RDONLY),
	ENT(audioctl_open_1_RDONLY_RWONLY),
	ENT(audioctl_open_1_RDONLY_RDWR),
	ENT(audioctl_open_1_WRONLY_RDONLY),
	ENT(audioctl_open_1_WRONLY_RWONLY),
	ENT(audioctl_open_1_WRONLY_RDWR),
	ENT(audioctl_open_1_RDWR_RDONLY),
	ENT(audioctl_open_1_RDWR_RWONLY),
	ENT(audioctl_open_1_RDWR_RDWR),
	ENT(audioctl_open_2_RDONLY_RDONLY),
	ENT(audioctl_open_2_RDONLY_RWONLY),
	ENT(audioctl_open_2_RDONLY_RDWR),
	ENT(audioctl_open_2_WRONLY_RDONLY),
	ENT(audioctl_open_2_WRONLY_RWONLY),
	ENT(audioctl_open_2_WRONLY_RDWR),
	ENT(audioctl_open_2_RDWR_RDONLY),
	ENT(audioctl_open_2_RDWR_RWONLY),
	ENT(audioctl_open_2_RDWR_RDWR),
	ENT(audioctl_open_simul),
/**/	ENT(audioctl_open_multiuser0_audio1),	// XXX TODO sysctl
/**/	ENT(audioctl_open_multiuser1_audio1),	// XXX TODO sysctl
/**/	ENT(audioctl_open_multiuser0_audio2),	// XXX TODO sysctl
/**/	ENT(audioctl_open_multiuser1_audio2),	// XXX TODO sysctl
/**/	ENT(audioctl_open_multiuser0_audioctl),	// XXX TODO sysctl
/**/	ENT(audioctl_open_multiuser1_audioctl),	// XXX TODO sysctl
	ENT(audioctl_rw_RDONLY),
	ENT(audioctl_rw_WRONLY),
	ENT(audioctl_rw_RDWR),
	ENT(audioctl_poll),
	ENT(audioctl_kqueue),
	{.name = NULL},
};
