/* $NetBSD: gpioctl.c,v 1.16.2.1 2012/04/17 00:09:46 yamt Exp $ */

/*
 * Copyright (c) 2008, 2010, 2011 Marc Balmer <mbalmer@NetBSD.org>
 * Copyright (c) 2004 Alexander Yurchenko <grange@openbsd.org>
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

/*
 * Program to control GPIO devices.
 */

#include <sys/types.h>
#include <sys/gpio.h>
#include <sys/ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

static char *dev;
static int devfd = -1;
static int quiet = 0;

static void getinfo(void);
static void gpioread(int, char *);
static void gpiowrite(int, char *, int);
static void gpioset(int pin, char *name, int flags, char *alias);
static void gpiounset(int pin, char *name);
static void devattach(char *, int, uint32_t, uint32_t);
__dead static void usage(void);

extern long long strtonum(const char *numstr, long long minval,
    long long maxval, const char **errstrp);

static const struct bitstr {
	unsigned int mask;
	const char *string;
} pinflags[] = {
	{ GPIO_PIN_INPUT, "in" },
	{ GPIO_PIN_OUTPUT, "out" },
	{ GPIO_PIN_INOUT, "inout" },
	{ GPIO_PIN_OPENDRAIN, "od" },
	{ GPIO_PIN_PUSHPULL, "pp" },
	{ GPIO_PIN_TRISTATE, "tri" },
	{ GPIO_PIN_PULLUP, "pu" },
	{ GPIO_PIN_PULLDOWN, "pd" },
	{ GPIO_PIN_INVIN, "iin" },
	{ GPIO_PIN_INVOUT, "iout" },
	{ GPIO_PIN_PULSATE, "pulsate" },
	{ 0, NULL },
};

int
main(int argc, char *argv[])
{
	const struct bitstr *bs;
	int pin, ch, n, fl = 0, value = 0;
	const char *errstr;
	char *ep;
	int ga_offset = -1;
	uint32_t ga_mask = 0;
	uint32_t ga_flags = 0;
	long lval;
	char *nam = NULL;
	char *flags;
	char devn[32];

	while ((ch = getopt(argc, argv, "q")) != -1)
		switch (ch) {
		case 'q':
			quiet = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	dev = argv[0];

	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) {
		(void)snprintf(devn, sizeof(devn), "%s%s", _PATH_DEV, dev);
		dev = devn;
	}

	if ((devfd = open(dev, O_RDWR)) == -1)
		err(EXIT_FAILURE, "%s", dev);

	if (argc == 1) {
		getinfo();
		return EXIT_SUCCESS;
	}

	if (!strcmp(argv[1], "attach")) {
		char *driver, *offset, *mask;

		if (argc != 5 && argc != 6)
			usage();

		driver = argv[2];
		offset = argv[3];
		mask = argv[4];
		flags = argc == 6 ? argv[5] : NULL;

		ga_offset = strtonum(offset, 0, INT_MAX, &errstr);
		if (errstr)
			errx(EXIT_FAILURE, "offset is %s: %s", errstr, offset);
		lval = strtol(mask, &ep, 0);
		if (*mask == '\0' || *ep != '\0')
			errx(EXIT_FAILURE, "invalid mask (not a number)");
		if ((errno == ERANGE && (lval == LONG_MAX
		    || lval == LONG_MIN)) || (unsigned long)lval > UINT_MAX)
			errx(EXIT_FAILURE, "mask out of range");
		ga_mask = lval;
		if (flags != NULL) {
			lval = strtol(flags, &ep, 0);
			if (*flags == '\0' || *ep != '\0')
				errx(EXIT_FAILURE,
				    "invalid flag locator (not a number)");
			if ((errno == ERANGE && (lval == LONG_MAX
			    || lval == LONG_MIN))
			    || (unsigned long)lval > UINT_MAX)
				errx(EXIT_FAILURE, "flag locator out of range");

			ga_flags = lval;
		}
		devattach(driver, ga_offset, ga_mask, ga_flags);
		return EXIT_SUCCESS;
	} else {
		char *nm = NULL;

		/* expecting a pin number or name */
		pin = strtonum(argv[1], 0, INT_MAX, &errstr);
		if (errstr)
			nm = argv[1];	/* try named pin */
		if (argc > 2) {
			if (!strcmp(argv[2], "set")) {
				for (n = 3; n < argc; n++) {
					for (bs = pinflags; bs->string != NULL;
					     bs++) {
						if (!strcmp(argv[n],
						    bs->string)) {
							fl |= bs->mask;
							break;
						}
					}
					if (bs->string == NULL)
						nam = argv[n];
				}
				gpioset(pin, nm, fl, nam);
			} else if (!strcmp(argv[2], "unset"))
				gpiounset(pin, nm);
			else {
				value = strtonum(argv[2], INT_MIN, INT_MAX,
				   &errstr);
				if (errstr) {
					if (!strcmp(argv[2], "on"))
						value = GPIO_PIN_HIGH;
					else if (!strcmp(argv[2], "off"))
						value = GPIO_PIN_LOW;
					else if (!strcmp(argv[2], "toggle"))
						value = 2;
					else
						errx(EXIT_FAILURE,
						    "%s: invalid value",
						    argv[2]);
				}
				gpiowrite(pin, nm, value);
			}
		} else
			gpioread(pin, nm);
	}

	return EXIT_SUCCESS;
}

static void
getinfo(void)
{
	struct gpio_info info;

	if (ioctl(devfd, GPIOINFO, &info) == -1)
		err(EXIT_FAILURE, "GPIOINFO");

	if (quiet)
		return;

	printf("%s: %d pins\n", dev, info.gpio_npins);
}

static void
gpioread(int pin, char *gp_name)
{
	struct gpio_req req;

	memset(&req, 0, sizeof(req));
	if (gp_name != NULL)
		strlcpy(req.gp_name, gp_name, sizeof(req.gp_name));
	else
		req.gp_pin = pin;

	if (ioctl(devfd, GPIOREAD, &req) == -1)
		err(EXIT_FAILURE, "GPIOREAD");

	if (quiet)
		return;

	if (gp_name)
		printf("pin %s: state %d\n", gp_name, req.gp_value);
	else
		printf("pin %d: state %d\n", pin, req.gp_value);
}

static void
gpiowrite(int pin, char *gp_name, int value)
{
	struct gpio_req req;

	if (value < 0 || value > 2)
		errx(EXIT_FAILURE, "%d: invalid value", value);

	memset(&req, 0, sizeof(req));
	if (gp_name != NULL)
		strlcpy(req.gp_name, gp_name, sizeof(req.gp_name));
	else
		req.gp_pin = pin;

	if (value == GPIO_PIN_HIGH || value == GPIO_PIN_LOW) {
		req.gp_value = value;
		if (ioctl(devfd, GPIOWRITE, &req) == -1)
			err(EXIT_FAILURE, "GPIOWRITE");
	} else {
		if (ioctl(devfd, GPIOTOGGLE, &req) == -1)
			err(EXIT_FAILURE, "GPIOTOGGLE");
	}

	if (quiet)
		return;

	if (gp_name)
		printf("pin %s: state %d -> %d\n", gp_name, req.gp_value,
		    (value < 2 ? value : 1 - req.gp_value));
	else
		printf("pin %d: state %d -> %d\n", pin, req.gp_value,
		    (value < 2 ? value : 1 - req.gp_value));
}

static void
gpioset(int pin, char *name, int fl, char *alias)
{
	struct gpio_set set;
	const struct bitstr *bs;

	memset(&set, 0, sizeof(set));
	if (name != NULL)
		strlcpy(set.gp_name, name, sizeof(set.gp_name));
	else
		set.gp_pin = pin;
	set.gp_flags = fl;

	if (alias != NULL)
		strlcpy(set.gp_name2, alias, sizeof(set.gp_name2));

	if (ioctl(devfd, GPIOSET, &set) == -1)
		err(EXIT_FAILURE, "GPIOSET");

	if (quiet)
		return;

	if (name != NULL)
		printf("pin %s: caps:", name);
	else
		printf("pin %d: caps:", pin);
	for (bs = pinflags; bs->string != NULL; bs++)
		if (set.gp_caps & bs->mask)
			printf(" %s", bs->string);
	printf(", flags:");
	for (bs = pinflags; bs->string != NULL; bs++)
		if (set.gp_flags & bs->mask)
			printf(" %s", bs->string);
	if (fl > 0) {
		printf(" ->");
		for (bs = pinflags; bs->string != NULL; bs++)
			if (fl & bs->mask)
				printf(" %s", bs->string);
	}
	printf("\n");
}

static void
gpiounset(int pin, char *name)
{
	struct gpio_set set;

	memset(&set, 0, sizeof(set));
	if (name != NULL)
		strlcpy(set.gp_name, name, sizeof(set.gp_name));
	else
		set.gp_pin = pin;

	if (ioctl(devfd, GPIOUNSET, &set) == -1)
		err(EXIT_FAILURE, "GPIOUNSET");
}

static void
devattach(char *dvname, int offset, uint32_t mask, uint32_t flags)
{
	struct gpio_attach attach;

	memset(&attach, 0, sizeof(attach));
	strlcpy(attach.ga_dvname, dvname, sizeof(attach.ga_dvname));
	attach.ga_offset = offset;
	attach.ga_mask = mask;
	attach.ga_flags = flags;
	if (ioctl(devfd, GPIOATTACH, &attach) == -1)
		err(EXIT_FAILURE, "GPIOATTACH");
}

static void
usage(void)
{
	const char *progname;

	progname = getprogname();
	fprintf(stderr, "usage: %s [-q] device [pin] [0 | 1 | 2 | "
	    "on | off | toggle]\n", progname);
	fprintf(stderr, "       %s [-q] device pin set [flags] [name]\n",
	    progname);
	fprintf(stderr, "       %s [-q] device pin unset\n", progname);
	fprintf(stderr, "       %s [-q] device attach device offset mask "
	    "[flag]\n",
	    progname);

	exit(EXIT_FAILURE);
}
