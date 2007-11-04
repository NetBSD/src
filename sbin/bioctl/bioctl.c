/* $NetBSD: bioctl.c,v 1.2 2007/11/04 08:25:05 xtraeme Exp $ */
/* $OpenBSD: bioctl.c,v 1.52 2007/03/20 15:26:06 jmc Exp $       */

/*
 * Copyright (c) 2004, 2005 Marco Peereboom
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: bioctl.c,v 1.2 2007/11/04 08:25:05 xtraeme Exp $");
#endif

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <dev/biovar.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <util.h>
#include "strtonum.h"

struct locator {
	int channel;
	int target;
	int lun;
};

static void usage(void);
static const char *str2locator(const char *, struct locator *);

static void bio_inq(int, char *);
static void bio_alarm(int, char *);
static void bio_setstate(int, char *);
static void bio_setblink(int, char *, char *, int);
static void bio_blink(int, char *, int, int);

static int debug;
static int human;
static int verbose;

static struct bio_locate bl;

int
main(int argc, char *argv[])
{
	uint64_t func = 0;
	char *bioc_dev, *al_arg, *bl_arg;
	int fd, ch, rv, blink;

	bioc_dev = al_arg = bl_arg = NULL;
	fd = ch = rv = blink = 0;

	if (argc < 2)
		usage();

	setprogname(*argv);

	while ((ch = getopt(argc, argv, "b:c:l:u:H:ha:Dv")) != -1) {
		switch (ch) {
		case 'a': /* alarm */
			func |= BIOC_ALARM;
			al_arg = optarg;
			break;
		case 'b': /* blink */
			func |= BIOC_BLINK;
			blink = BIOC_SBBLINK;
			bl_arg = optarg;
			break;
		case 'u': /* unblink */
			func |= BIOC_BLINK;
			blink = BIOC_SBUNBLINK;
			bl_arg = optarg;
			break;
		case 'D': /* debug */
			debug = 1;
			break;
		case 'H': /* set hotspare */
			func |= BIOC_SETSTATE;
			al_arg = optarg;
			break;
		case 'h':
			human = 1;
			break;
		case 'i': /* inquiry */
			func |= BIOC_INQ;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (func == 0)
		func |= BIOC_INQ;

	bioc_dev = argv[0];

	if (bioc_dev) {
		fd = open("/dev/bio", O_RDWR);
		if (fd == -1)
			err(EXIT_FAILURE, "Can't open %s", "/dev/bio");

		bl.bl_name = bioc_dev;
		rv = ioctl(fd, BIOCLOCATE, &bl);
		if (rv == -1)
			errx(EXIT_FAILURE, "Can't locate %s device via %s",
			    bl.bl_name, "/dev/bio");
	}

	if (debug)
		warnx("cookie = %p", bl.bl_cookie);

	if (func & BIOC_INQ) {
		bio_inq(fd, bioc_dev);
	} else if (func == BIOC_ALARM) {
		bio_alarm(fd, al_arg);
	} else if (func == BIOC_BLINK) {
		bio_setblink(fd, bioc_dev, bl_arg, blink);
	} else if (func == BIOC_SETSTATE) {
		bio_setstate(fd, al_arg);
	}

	exit(EXIT_SUCCESS);
}

static void
usage(void)
{
	(void)fprintf(stderr,
		"usage: %s [-Dhv] [-a alarm-function] "
		"[-b channel:target[.lun]]\n"
		"\t[-H channel:target[.lun]]\n"
		"\t[-u channel:target[.lun]] device\n", getprogname());
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

static const char *
str2locator(const char *string, struct locator *location)
{
	const char *errstr;
	char parse[80], *targ, *lun;

	strlcpy(parse, string, sizeof parse);
	targ = strchr(parse, ':');
	if (targ == NULL)
		return ("target not specified");
	*targ++ = '\0';

	lun = strchr(targ, '.');
	if (lun != NULL) {
		*lun++ = '\0';
		location->lun = strtonum(lun, 0, 256, &errstr);
		if (errstr)
			return errstr;
	} else
		location->lun = 0;

	location->target = strtonum(targ, 0, 256, &errstr);
	if (errstr)
		return errstr;
	location->channel = strtonum(parse, 0, 256, &errstr);
	if (errstr)
		return errstr;
	return NULL;
}

static void
bio_inq(int fd, char *name)
{
	const char *status;
	char size[64], scsiname[16], volname[32];
	char percent[10], seconds[20];
	int rv, i, d, volheader, hotspare, unused;
	char encname[16], serial[32];
	struct bioc_disk bd;
	struct bioc_inq bi;
	struct bioc_vol bv;

	memset(&bi, 0, sizeof(bi));

	if (debug)
		printf("bio_inq\n");

	bi.bi_cookie = bl.bl_cookie;

	rv = ioctl(fd, BIOCINQ, &bi);
	if (rv == -1) {
		warn("BIOCINQ");
		return;
	}

	if (debug)
		printf("bio_inq { %p, %s, %d, %d }\n",
		    bi.bi_cookie,
		    bi.bi_dev,
		    bi.bi_novol,
		    bi.bi_nodisk);

	volheader = 0;
	for (i = 0; i < bi.bi_novol; i++) {
		memset(&bv, 0, sizeof(bv));
		bv.bv_cookie = bl.bl_cookie;
		bv.bv_volid = i;
		bv.bv_percent = -1;
		bv.bv_seconds = 0;

		rv = ioctl(fd, BIOCVOL, &bv);
		if (rv == -1) {
			warn("BIOCVOL");
			return;
		}

		if (name && strcmp(name, bv.bv_dev) != 0)
			continue;

		if (!volheader) {
			volheader = 1;
			printf("%-7s %-10s %14s %-8s\n",
			    "Volume", "Status", "Size", "Device");
		}

		percent[0] = '\0';
		seconds[0] = '\0';
		if (bv.bv_percent != -1)
			snprintf(percent, sizeof percent,
			    " %d%% done", bv.bv_percent);
		if (bv.bv_seconds)
			snprintf(seconds, sizeof seconds,
			    " %u seconds", bv.bv_seconds);
		switch (bv.bv_status) {
		case BIOC_SVONLINE:
			status = BIOC_SVONLINE_S;
			break;
		case BIOC_SVOFFLINE:
			status = BIOC_SVOFFLINE_S;
			break;
		case BIOC_SVDEGRADED:
			status = BIOC_SVDEGRADED_S;
			break;
		case BIOC_SVBUILDING:
			status = BIOC_SVBUILDING_S;
			break;
		case BIOC_SVREBUILD:
			status = BIOC_SVREBUILD_S;
			break;
		case BIOC_SVSCRUB:
			status = BIOC_SVSCRUB_S;
			break;
		case BIOC_SVINVALID:
		default:
			status = BIOC_SVINVALID_S;
		}

		snprintf(volname, sizeof volname, "%s %u",
		    bi.bi_dev, bv.bv_volid);

		if (bv.bv_level == -1 && bv.bv_nodisk == 1) {
			hotspare = 1;
			unused = 0;
		} else if (bv.bv_level == -2 && bv.bv_nodisk == 1) {
			unused = 1;
			hotspare = 0;
		} else {
			unused = 0;
			hotspare = 0;

			if (human)
				humanize_number(size, 5,
				    (int64_t)bv.bv_size, "", HN_AUTOSCALE,
				    HN_B | HN_NOSPACE | HN_DECIMAL);
			else
				snprintf(size, sizeof size, "%14llu",
				    (long long unsigned int)bv.bv_size);
			printf("%7s %-10s %14s %-7s RAID%u%s%s\n",
			    volname, status, size, bv.bv_dev,
			    bv.bv_level, percent, seconds);
		}

		for (d = 0; d < bv.bv_nodisk; d++) {
			memset(&bd, 0, sizeof(bd));
			bd.bd_cookie = bl.bl_cookie;
			bd.bd_diskid = d;
			bd.bd_volid = i;

			rv = ioctl(fd, BIOCDISK, &bd);
			if (rv == -1) {
				warn("BIOCDISK");
				return;
			}

			switch (bd.bd_status) {
			case BIOC_SDONLINE:
				status = BIOC_SDONLINE_S;
				break;
			case BIOC_SDOFFLINE:
				status = BIOC_SDOFFLINE_S;
				break;
			case BIOC_SDFAILED:
				status = BIOC_SDFAILED_S;
				break;
			case BIOC_SDREBUILD:
				status = BIOC_SDREBUILD_S;
				break;
			case BIOC_SDHOTSPARE:
				status = BIOC_SDHOTSPARE_S;
				break;
			case BIOC_SDUNUSED:
				status = BIOC_SDUNUSED_S;
				break;
			case BIOC_SDSCRUB:
				status = BIOC_SDSCRUB_S;
				break;
			case BIOC_SDINVALID:
			default:
				status = BIOC_SDINVALID_S;
			}

			if (hotspare || unused)
				;	/* use volname from parent volume */
			else
				snprintf(volname, sizeof volname, "    %3u",
				    bd.bd_diskid);

			if (human)
				humanize_number(size, 5,
				    bd.bd_size, "", HN_AUTOSCALE,
				    HN_B | HN_NOSPACE | HN_DECIMAL);
			else
				snprintf(size, sizeof size, "%14llu",
				    (long long unsigned int)bd.bd_size);
			snprintf(scsiname, sizeof scsiname,
			    "%u:%u.%u",
			    bd.bd_channel, bd.bd_target, bd.bd_lun);
			if (bd.bd_procdev[0])
				strlcpy(encname, bd.bd_procdev, sizeof encname);
			else
				strlcpy(encname, "noencl", sizeof encname);
			if (bd.bd_serial[0])
				strlcpy(serial, bd.bd_serial, sizeof serial);
			else
				strlcpy(serial, "unknown serial", sizeof serial);

			printf("%7s %-10s %14s %-7s %-6s <%s>\n",
			    volname, status, size, scsiname, encname,
			    bd.bd_vendor);
			if (verbose)
				printf("%7s %-10s %14s %-7s %-6s '%s'\n",
				    "", "", "", "", "", serial);
		}
	}
}

static void
bio_alarm(int fd, char *arg)
{
	int rv;
	struct bioc_alarm ba;

	ba.ba_cookie = bl.bl_cookie;

	switch (arg[0]) {
	case 'q': /* silence alarm */
		/* FALLTHROUGH */
	case 's':
		ba.ba_opcode = BIOC_SASILENCE;
		break;

	case 'e': /* enable alarm */
		ba.ba_opcode = BIOC_SAENABLE;
		break;

	case 'd': /* disable alarm */
		ba.ba_opcode = BIOC_SADISABLE;
		break;

	case 't': /* test alarm */
		ba.ba_opcode = BIOC_SATEST;
		break;

	case 'g': /* get alarm state */
		ba.ba_opcode = BIOC_GASTATUS;
		break;

	default:
		warnx("invalid alarm function: %s", arg);
		return;
	}

	rv = ioctl(fd, BIOCALARM, &ba);
	if (rv == -1) {
		warn("BIOCALARM");
		return;
	}

	if (arg[0] == 'g') {
		printf("alarm is currently %s\n",
		    ba.ba_status ? "enabled" : "disabled");

	}
}

static void
bio_setstate(int fd, char *arg)
{
	struct bioc_setstate	bs;
	struct locator		location;
	const char		*errstr;
	int			rv;

	errstr = str2locator(arg, &location);
	if (errstr)
		errx(1, "Target %s: %s", arg, errstr);

	bs.bs_cookie = bl.bl_cookie;
	bs.bs_status = BIOC_SSHOTSPARE;
	bs.bs_channel = location.channel;
	bs.bs_target = location.target;
	bs.bs_lun = location.lun;

	rv = ioctl(fd, BIOCSETSTATE, &bs);
	if (rv == -1) {
		warn("BIOCSETSTATE");
		return;
	}
}

static void
bio_setblink(int fd, char *name, char *arg, int blink)
{
	struct locator		location;
	struct bioc_inq		bi;
	struct bioc_vol		bv;
	struct bioc_disk	bd;
	struct bioc_blink	bb;
	const char		*errstr;
	int			v, d, rv;

	errstr = str2locator(arg, &location);
	if (errstr)
		errx(1, "Target %s: %s", arg, errstr);

	/* try setting blink on the device directly */
	memset(&bb, 0, sizeof(bb));
	bb.bb_cookie = bl.bl_cookie;
	bb.bb_status = blink;
	bb.bb_target = location.target;
	bb.bb_channel = location.channel;
	rv = ioctl(fd, BIOCBLINK, &bb);
	if (rv == 0)
		return;

	/* if the blink didnt work, try to find something that will */

	memset(&bi, 0, sizeof(bi));
	bi.bi_cookie = bl.bl_cookie;
	rv = ioctl(fd, BIOCINQ, &bi);
	if (rv == -1) {
		warn("BIOCINQ");
		return;
	}

	for (v = 0; v < bi.bi_novol; v++) {
		memset(&bv, 0, sizeof(bv));
		bv.bv_cookie = bl.bl_cookie;
		bv.bv_volid = v;
		rv = ioctl(fd, BIOCVOL, &bv);
		if (rv == -1) {
			warn("BIOCVOL");
			return;
		}

		if (name && strcmp(name, bv.bv_dev) != 0)
			continue;

		for (d = 0; d < bv.bv_nodisk; d++) {
			memset(&bd, 0, sizeof(bd));
			bd.bd_cookie = bl.bl_cookie;
			bd.bd_volid = v;
			bd.bd_diskid = d;

			rv = ioctl(fd, BIOCDISK, &bd);
			if (rv == -1) {
				warn("BIOCDISK");
				return;
			}

			if (bd.bd_channel == location.channel &&
			    bd.bd_target == location.target &&
			    bd.bd_lun == location.lun) {
				if (bd.bd_procdev[0] != '\0') {
					bio_blink(fd, bd.bd_procdev,
					    location.target, blink);
				} else
					warnx("Disk %s is not in an enclosure", arg);
				return;
			}
		}
	}

	warnx("Disk %s does not exist", arg);
	return;
}

static void
bio_blink(int fd, char *enclosure, int target, int blinktype)
{
	struct bio_locate	bio;
	struct bioc_blink	blink;
	int			rv;

	bio.bl_name = enclosure;
	rv = ioctl(fd, BIOCLOCATE, &bio);
	if (rv == -1)
		errx(1, "Can't locate %s device via %s", enclosure, "/dev/bio");

	memset(&blink, 0, sizeof(blink));
	blink.bb_cookie = bio.bl_cookie;
	blink.bb_status = blinktype;
	blink.bb_target = target;

	rv = ioctl(fd, BIOCBLINK, &blink);
	if (rv == -1)
		warn("BIOCBLINK");
}
