/*	$NetBSD: strptimetest.c,v 1.1 2005/03/04 21:42:40 dsl Exp $	*/

/*
 * This file placed in the public domain.
 * David Laight March 2005
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
// #include <errno.h>

#define XSTR(x) #x
#define STR(x) XSTR(x)

typedef struct tm tm_t;

const static struct test_list {
    const char	*buf;
    const char	*fmt;
    int		len;
    tm_t	want;
} std_tests[] = {
    { "mon", "%a", 3, { .tm_wday = 1 } },
    { "tueSDay", "%A", 7, { .tm_wday = 2 } },
    { "september", "%b", 9, { .tm_mon = 8 } },
    { "septembe", "%B", 3, { .tm_mon = 8 } },
    { "Fri Mar  4 20:05:34 2005", "%a %b %e %H:%M:%S %Y", 24,
	    { .tm_wday = 5, .tm_mon=2, .tm_mday = 4, .tm_hour=20,
	      .tm_min=5, .tm_sec=34, .tm_year=105 } },

    /* The pre-1.23 versions get some of these wrong! */
    { "5\t3  4 8pm:05:34 2005", "%w%n%m%t%d%n%k%p:%M:%S %Y", 21,
	    { .tm_wday = 5, .tm_mon=2, .tm_mday = 4, .tm_hour=20,
	      .tm_min=5, .tm_sec=34, .tm_year=105 } },
    { "Fri Mar  4 20:05:34 2005", "%c", 24,
	    { .tm_wday = 5, .tm_mon=2, .tm_mday = 4, .tm_hour=20,
	      .tm_min=5, .tm_sec=34, .tm_year=105 } },
    { "x20y", "x%Cy", 4, { .tm_year = 100 } },
    { "x84y", "x%yy", 4, { .tm_year = 84 } },
    { "x2084y", "x%C%yy", 6, { .tm_year = 184 } },
    { "x8420y", "x%y%Cy", 6, { .tm_year = 184 } },
    { "%20845", "%%%C%y5", 6, { .tm_year = 184 } },
    { "%", "%E%", -1 },

    { "1980", "%Y", 4,  { .tm_year = 80 } },
    { "1980", "%EY", 4,  { .tm_year = 80 } },
    { "sunday", "%A", 6 },
    { "sunday", "%EA", -1 },
    { "SaturDay", "%A", 8, { .tm_wday = 6 } },
    { "SaturDay", "%OA", -1 },

    { "0", "%S", 1 },
    { "59", "%S", 2, { .tm_sec = 59 } },
    { "60", "%S", 2, { .tm_sec = 60 } },
    { "61", "%S", 2, { .tm_sec = 61 } },
    { "62", "%S", -1 },
    { 0 },
};


static void
usage(void)
{
	fprintf(stderr, "usage: strptimetest [[-Ssec] [-Mmin] [-Hhour] [-dmday] [-mmon] [-Yyear] [-uwday] [-jyday] buffer format length]\n");
	exit(1);
}

static void
mismatch(const char *buf, const char *fmt, int len,
	const char *tm_x, int got_tm_x, int want_tm_x)
{
	fprintf(stderr,
	    "strptime(\"%.*s\" \"%s\", \"%s\", ...) failed %s %d != %d\n",
	    len, buf, buf + len, fmt, tm_x, got_tm_x, want_tm_x);
}

static int
test(const char *buf, const char *fmt, int len, const tm_t *want)
{
	tm_t got;
	const char *np;

	memset(&got, 0, sizeof got);
	np = strptime(buf, fmt, &got);
	if (!np) {
		/* strptime failed */
		if (len == -1)
			return 0;
		fprintf(stderr,
		    "strptime(\"%.*s\" \"%s\", \"%s\", ...) failed\n",
		    len, buf, buf + len, fmt);
		return 2;
	}
	/* strptime ok */
	if (len == -1) {
		fprintf(stderr,
		    "strptime(\"%.*s\" \"%s\", \"%s\", ...) ok so failed\n",
		    np - buf, buf, np, fmt);
		return 2;
	}
	if (np - buf != len) {
		fprintf(stderr,
		    "strptime(\"%.*s\" \"%s\", \"%s\", ...) failed len %d\n",
		    np - buf, buf, np, fmt, len);
		return 2;
	}

#define CHECK(tm_x) \
	if (got.tm_x != want->tm_x) { \
		mismatch(buf, fmt, len, STR(tm_x), got.tm_x, want->tm_x); \
		return 2; \
	}


	CHECK(tm_sec);
	CHECK(tm_min);
	CHECK(tm_hour);
	CHECK(tm_mday);
	CHECK(tm_mon);
	CHECK(tm_year);
	CHECK(tm_wday);
	CHECK(tm_yday);
#undef CHECK
	return 0;
}

static int
std_test(void)
{
	const struct test_list *t;
	int rval;

	for (t = std_tests; t->fmt; t++) {
		rval = test(t->buf, t->fmt, t->len, &t->want);
		if (rval != 0)
			return rval;
	}
	return 0;
}

static int
argint(void)
{
	char *ep;
	int v;

	v = strtoul(optarg, &ep, 0);
	if (*ep || ep == optarg)
		usage();
	return v;
}

int
main(int argc, char *argv[])
{
	tm_t tm_want;

	if (argc <= 1)
		return std_test();

	memset(&tm_want, 0, sizeof tm_want);
	for (;;) {
		switch (getopt(argc, argv, "S:M:H:d:m:Y:u:j:")) {
		default:
			usage();
		case -1:
			break;
		case 'S': tm_want.tm_sec = argint(); continue;
		case 'M': tm_want.tm_min = argint(); continue;
		case 'H': tm_want.tm_hour = argint(); continue;
		case 'd': tm_want.tm_mday = argint(); continue;
		case 'm': tm_want.tm_mon = argint(); continue;
		case 'Y': tm_want.tm_year = argint() - 1900; continue;
		case 'u': tm_want.tm_wday = argint(); continue;
		case 'j': tm_want.tm_yday = argint(); continue;
		}
		break;
	}
	argv += optind;
	argc -= optind;
	if (argc != 3)
		usage();
	return test(argv[0], argv[1], atoi(argv[2]), &tm_want);
}
