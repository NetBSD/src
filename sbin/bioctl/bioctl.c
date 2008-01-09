/* $NetBSD: bioctl.c,v 1.1.6.2 2008/01/09 01:38:02 matt Exp $ */
/* $OpenBSD: bioctl.c,v 1.52 2007/03/20 15:26:06 jmc Exp $ */

/*
 * Copyright (c) 2007, 2008 Juan Romero Pardines
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
__RCSID("$NetBSD: bioctl.c,v 1.1.6.2 2008/01/09 01:38:02 matt Exp $");
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <dev/biovar.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <util.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <util.h>
#include "strtonum.h"

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func)(int, int, char **);
};

struct biotmp {
	struct bioc_inq *bi;
	struct bioc_vol *bv;
	char volname[64];
	int fd;
	int volid;
	int diskid;
	bool format;
	bool show_disknovol;
};

struct locator {
	int channel;
	int target;
	int lun;
};

static void 	usage(void);
static void	bio_alarm(int, int, char **);
static void	bio_show_common(int, int, char **);
static int	bio_show_volumes(struct biotmp *);
static void	bio_show_disks(struct biotmp *);
static void	bio_setblink(int, int, char **);
static void 	bio_blink(int, char *, int, int);
static void	bio_setstate_hotspare(int, int, char **);
static void	bio_setstate_passthru(int, int, char **);
static void	bio_setstate_common(int, char *, struct bioc_setstate *,
				    struct locator *);
static void	bio_setstate_consistency(int, int, char **);
static void	bio_volops_create(int, int, char **);
#ifdef notyet
static void	bio_volops_modify(int, int, char **);
#endif
static void	bio_volops_remove(int, int, char **);

static const char *str2locator(const char *, struct locator *);

static struct bio_locate bl;
static struct command commands[] = {
	{ 
	  "show",
	  "[disks] | [volumes]",
	  bio_show_common },
	{ 
	  "alarm",
	  "[enable] | [disable] | [silence] | [test]",
	  bio_alarm },
	{ 
	  "blink",
	  "start | stop [channel:target[.lun]]",
	  bio_setblink },
	{
	  "hotspare",
	  "add | remove channel:target.lun",
	  bio_setstate_hotspare },
	{
	  "passthru",
	  "add DISKID | remove channel:target.lun",
	  bio_setstate_passthru },
	{
	  "check",
	  "start | stop VOLID",
	  bio_setstate_consistency },
	{
	  "create",
	  "volume VOLID DISKIDs [SIZE] STRIPE RAID_LEVEL channel:target.lun",
	  bio_volops_create },
#ifdef notyet
	{
	  "modify",
	  "volume VOLID STRIPE RAID_LEVEL channel:target.lun",
	  bio_volops_modify },
#endif
	{
	  "remove",
	  "volume VOLID channel:target.lun",
	  bio_volops_remove },

	{ NULL, NULL, NULL }
};

int
main(int argc, char **argv)
{
	char 		*dvname;
	const char 	*cmdname;
	int 		fd = 0, i;

	/* Must have at least: device command */
	if (argc < 3)
		usage();

	/* Skip program name, get and skip device name and command */
	setprogname(*argv);
	dvname = argv[1];
	cmdname = argv[2];
	argv += 3;
	argc -= 3;

	/* Look up and call the command */
	for (i = 0; commands[i].cmd_name != NULL; i++)
		if (strcmp(cmdname, commands[i].cmd_name) == 0)
			break;
	if (commands[i].cmd_name == NULL)
		errx(EXIT_FAILURE, "unknown command: %s", cmdname);

	/* Locate the device by issuing the BIOCLOCATE ioctl */
	fd = open("/dev/bio", O_RDWR);
	if (fd == -1)
		err(EXIT_FAILURE, "Can't open /dev/bio");

	bl.bl_name = dvname;
	if (ioctl(fd, BIOCLOCATE, &bl) == -1)
		errx(EXIT_FAILURE, "Can't locate %s device via /dev/bio",
		    bl.bl_name);

	/* and execute the command */
	(*commands[i].cmd_func)(fd, argc, argv);

	(void)close(fd);
	exit(EXIT_SUCCESS);
}

static void
usage(void)
{
	int i;

	(void)fprintf(stderr, "usage: %s device command [arg [...]]\n",
	    getprogname());

	(void)fprintf(stderr, "Available commands:\n");
	for (i = 0; commands[i].cmd_name != NULL; i++)
		(void)fprintf(stderr, "  %s %s\n", commands[i].cmd_name,
		    commands[i].arg_names);

	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

static const char *
str2locator(const char *string, struct locator *location)
{
	const char 	*errstr;
	char 		parse[80], *targ, *lun;

	strlcpy(parse, string, sizeof parse);
	targ = strchr(parse, ':');
	if (targ == NULL)
		return "target not specified";

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

/*
 * Shows info about available RAID volumes.
 */
static int
bio_show_volumes(struct biotmp *bt)
{
	struct bioc_vol 	bv;
	const char 		*status;
	char 			size[64], percent[16], seconds[20];
	char 			rtype[16], stripe[16], tmp[32];

	memset(&bv, 0, sizeof(bv));
	bv.bv_cookie = bl.bl_cookie;
	bv.bv_volid = bt->volid;
	bv.bv_percent = -1;
	bv.bv_seconds = -1;

	if (ioctl(bt->fd, BIOCVOL, &bv) == -1)
		err(EXIT_FAILURE, "BIOCVOL");

	percent[0] = '\0';
	seconds[0] = '\0';
	if (bv.bv_percent != -1)
		snprintf(percent, sizeof(percent),
		    " %3.2f%% done", bv.bv_percent / 10.0);
	if (bv.bv_seconds)
		snprintf(seconds, sizeof(seconds),
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
	case BIOC_SVMIGRATING:
		status = BIOC_SVMIGRATING_S;
		break;
	case BIOC_SVSCRUB:
		status = BIOC_SVSCRUB_S;
		break;
	case BIOC_SVCHECKING:
		status = BIOC_SVCHECKING_S;
		break;
	case BIOC_SVINVALID:
	default:
		status = BIOC_SVINVALID_S;
		break;
	}

	snprintf(bt->volname, sizeof(bt->volname), "%u", bv.bv_volid);
	if (bv.bv_vendor)
		snprintf(tmp, sizeof(tmp), "%s %s", bv.bv_dev, bv.bv_vendor);
	else
		snprintf(tmp, sizeof(tmp), "%s", bv.bv_dev);

	switch (bv.bv_level) {
	case BIOC_SVOL_HOTSPARE:
		snprintf(rtype, sizeof(rtype), "Hot spare");
		snprintf(stripe, sizeof(stripe), "N/A");
		break;
	case BIOC_SVOL_PASSTHRU:
		snprintf(rtype, sizeof(rtype), "Pass through");
		snprintf(stripe, sizeof(stripe), "N/A");
		break;
	default:
		snprintf(rtype, sizeof(rtype), "RAID %u", bv.bv_level);
		snprintf(stripe, sizeof(stripe), "%uK", bv.bv_stripe_size);
		break;
	}

	humanize_number(size, 5, (int64_t)bv.bv_size, "", HN_AUTOSCALE,
	    HN_B | HN_NOSPACE | HN_DECIMAL);

	printf("%6s %-12s %4s %20s %12s %6s %s%s\n",
	    bt->volname, status, size, tmp,
	    rtype, stripe, percent, seconds);

	bt->bv = &bv;

	return bv.bv_nodisk;
}

/*
 * Shows info about physical disks.
 */
static void
bio_show_disks(struct biotmp *bt)
{
	struct bioc_disk 	bd;
	const char 		*status;
	char 			size[64], serial[32], scsiname[16];

	memset(&bd, 0, sizeof(bd));
	bd.bd_cookie = bl.bl_cookie;
	bd.bd_diskid = bt->diskid;
	bd.bd_volid = bt->volid;

	if (bt->show_disknovol) {
		if (ioctl(bt->fd, BIOCDISK_NOVOL, &bd) == -1)
			err(EXIT_FAILURE, "BIOCDISK_NOVOL");
		if (!bd.bd_disknovol)
			return;
	} else {
		if (ioctl(bt->fd, BIOCDISK, &bd) == -1)
			err(EXIT_FAILURE, "BIOCDISK");
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
	case BIOC_SDPASSTHRU:
		status = BIOC_SDPASSTHRU_S;
		break;
	case BIOC_SDINVALID:
	default:
		status = BIOC_SDINVALID_S;
		break;
	}

	if (bt->format)
		snprintf(bt->volname, sizeof(bt->volname),
		    "%u:%u", bt->bv->bv_volid, bd.bd_diskid);

	humanize_number(size, 5, bd.bd_size, "", HN_AUTOSCALE,
	    HN_B | HN_NOSPACE | HN_DECIMAL);

	if (bd.bd_procdev[0])
		snprintf(scsiname, sizeof(scsiname), "%u:%u.%u %s",
	    	    bd.bd_channel, bd.bd_target, bd.bd_lun,
		    bd.bd_procdev);
	else
		snprintf(scsiname, sizeof(scsiname), "%u:%u.%u noencl",
		    bd.bd_channel, bd.bd_target, bd.bd_lun);

	if (bd.bd_serial[0])
		strlcpy(serial, bd.bd_serial, sizeof(serial));
	else
		strlcpy(serial, "unknown serial", sizeof(serial));

	if (bt->format)
		printf("%6s %-12s %4s %20s <%s>\n",
		    bt->volname, status, size, scsiname,
				    bd.bd_vendor);
	else
		printf("%5d [%-28s] %-12s %-6s %12s\n",
		    bt->diskid, bd.bd_vendor, status, size, scsiname);

}

/*
 * Shows info about volumes/disks.
 */
static void
bio_show_common(int fd, int argc, char **argv)
{
	struct biotmp 		*biot;
	struct bioc_inq 	bi;
	int 			i, d, ndisks;
	bool 			show_all, show_disks;
	bool 			show_vols, show_caps;

	show_all = show_disks = show_vols = show_caps = false;

	if (argc > 1)
		usage();

	if (argv[0]) {
		if (strcmp(argv[0], "disks") == 0)
			show_disks = true;
		else if (strcmp(argv[0], "volumes") == 0)
			show_vols = true;
		else
			usage();
	} else
		show_all = true;

	memset(&bi, 0, sizeof(bi));
	bi.bi_cookie = bl.bl_cookie;

	if (ioctl(fd, BIOCINQ, &bi) == -1)
		err(EXIT_FAILURE, "BIOCINQ");

	/*
	 * If there are volumes there's no point to continue.
	 */
	if (show_all || show_vols) {
		if (!bi.bi_novol) {
			warnx("no volumes available");
			return;
		}
	}

	biot = calloc(1, sizeof(*biot));
	if (!biot)
		err(EXIT_FAILURE, "biotemp calloc");

	biot->fd = fd;
	biot->bi = &bi;
	/*
	 * Go to the disks section if that was specified.
	 */
	if (show_disks)
		goto disks;

	/* 
	 * Common code to show only info about volumes and disks
	 * associated to them.
	 */
	printf("%6s %-12s %4s %20s %12s %6s\n",
	    "Volume", "Status", "Size", "Device/Label",
	    "RAID Level", "Stripe");
	printf("=============================================="
	       "===================\n");

	for (i = 0; i < bi.bi_novol; i++) {
		biot->format = true;
		biot->volid = i;
		ndisks = bio_show_volumes(biot);
		if (show_vols)
			continue;

		for (d = 0; d < ndisks; d++) {
			biot->diskid = d;
			bio_show_disks(biot);
		}

	}
	goto out;

disks:
	/* 
	 * show info about all disks connected to the raid controller,
	 * even if they aren't associated with a volume or raid set.
	 */
	if (show_disks) {
		printf("%5s %-30s %-12s %-6s %12s\n",
		    "Disk", "Model/Serial", "Status", "Size", "Location");
		printf("==============================================="
		       "======================\n");
		for (d = 0; d < bi.bi_nodisk; d++) {
			biot->show_disknovol = true;
			biot->diskid = d;
			bio_show_disks(biot);
		}
	}
out:
	free(biot);
}

/*
 * To handle the alarm feature.
 */
static void
bio_alarm(int fd, int argc, char **argv)
{
	struct bioc_alarm 	ba;
	bool 			show = false;

	memset(&ba, 0, sizeof(ba));
	ba.ba_cookie = bl.bl_cookie;

	if (argc > 1)
		usage();

	if (argc == 0) {
		/* show alarm status */
		ba.ba_opcode = BIOC_GASTATUS;
		show = true;
	} else if (strcmp(argv[0], "silence") == 0) {
		/* silence alarm */
		ba.ba_opcode = BIOC_SASILENCE;
	} else if (strcmp(argv[0], "enable") == 0) {
		/* enable alarm */
		ba.ba_opcode = BIOC_SAENABLE;
	} else if (strcmp(argv[0], "disable") == 0) {
		/* disable alarm */
		ba.ba_opcode = BIOC_SADISABLE;
	} else if (strcmp(argv[0], "test") == 0) {
		/* test alarm */
		ba.ba_opcode = BIOC_SATEST;
	} else
		usage();

	if (ioctl(fd, BIOCALARM, &ba) == -1)
		err(EXIT_FAILURE, "BIOCALARM");

	if (show)
		printf("alarm is currently %s\n",
		    ba.ba_status ? "enabled" : "disabled");
}

/*
 * To add/remove a hotspare disk.
 */
static void
bio_setstate_hotspare(int fd, int argc, char **argv)
{
	struct bioc_setstate	bs;
	struct locator		location;

	memset(&bs, 0, sizeof(bs));

	if (argc != 2)
		usage();

	if (strcmp(argv[0], "add") == 0)
		bs.bs_status = BIOC_SSHOTSPARE;
	else if (strcmp(argv[0], "remove") == 0)
		bs.bs_status = BIOC_SSDELHOTSPARE;
	else
		usage();

	bio_setstate_common(fd, argv[1], &bs, &location);
}

/*
 * To add/remove a pass through disk.
 */
static void
bio_setstate_passthru(int fd, int argc, char **argv)
{
	struct bioc_setstate	bs;
	struct locator		location;
	char			*endptr;
	bool			rem = false;

	if (argc > 3)
		usage();

	memset(&bs, 0, sizeof(bs));

	if (strcmp(argv[0], "add") == 0) {
		if (argv[1] == NULL || argv[2] == NULL)
			usage();

		bs.bs_status = BIOC_SSPASSTHRU;
	} else if (strcmp(argv[0], "remove") == 0) {
		if (argv[1] == NULL)
			usage();

		bs.bs_status = BIOC_SSDELPASSTHRU;
		rem = true;
	} else
		usage();

	if (rem)
		bio_setstate_common(fd, argv[1], &bs, &location);
	else {
		bs.bs_other_id = (unsigned int)strtoul(argv[1], &endptr, 10);
		if (*endptr != '\0')
			errx(EXIT_FAILURE, "Invalid Volume ID value");

		bio_setstate_common(fd, argv[2], &bs, &location);
	}
}

/*
 * To start/stop a consistency check in a RAID volume.
 */
static void
bio_setstate_consistency(int fd, int argc, char **argv)
{
	struct bioc_setstate	bs;
	char			*endptr;

	if (argc > 2)
		usage();

	if (strcmp(argv[0], "start") == 0)
		bs.bs_status = BIOC_SSCHECKSTART_VOL;
	else if (strcmp(argv[0], "stop") == 0)
		bs.bs_status = BIOC_SSCHECKSTOP_VOL;
	else
		usage();

	memset(&bs, 0, sizeof(bs));
	bs.bs_volid = (unsigned int)strtoul(argv[2], &endptr, 10);
	if (*endptr != '\0')
		errx(EXIT_FAILURE, "Invalid Volume ID value");

	bio_setstate_common(fd, NULL, &bs, NULL);
}

static void
bio_setstate_common(int fd, char *arg, struct bioc_setstate *bs,
		    struct locator *location)
{
	const char		*errstr;

	if (!arg || !location)
		goto send;

	errstr = str2locator(arg, location);
	if (errstr)
		errx(EXIT_FAILURE, "Target %s: %s", arg, errstr);

	bs->bs_channel = location->channel;
	bs->bs_target = location->target;
	bs->bs_lun = location->lun;

send:
	bs->bs_cookie = bl.bl_cookie;

	if (ioctl(fd, BIOCSETSTATE, bs) == -1)
		err(EXIT_FAILURE, "BIOCSETSTATE");
}

/*
 * To create a RAID volume.
 */
static void
bio_volops_create(int fd, int argc, char **argv)
{
	struct bioc_volops	bc;
	struct bioc_inq 	bi;
	struct bioc_disk	bd;
	struct locator		location;
	uint64_t 		total_disksize = 0, first_disksize = 0;
	int64_t 		volsize = 0;
	const char 		*errstr;
	char 			*endptr, *stripe;
	char			*scsiname, *raid_level, size[64];
	int			disk_first = 0, disk_end = 0;
	int			i, nfreedisks = 0;
	int			user_disks = 0;

	if (argc < 6 || argc > 7)
		usage();

	if (strcmp(argv[0], "volume") != 0)
		usage();

	/* 
	 * No size requested, use max size depending on RAID level.
	 */
	if (argc == 6) {
		stripe = argv[3];
		raid_level = argv[4];
		scsiname = argv[5];
	} else {
		stripe = argv[4];
		raid_level = argv[5];
		scsiname = argv[6];
	}

	memset(&bd, 0, sizeof(bd));
	memset(&bc, 0, sizeof(bc));
	memset(&bi, 0, sizeof(bi));

	bc.bc_cookie = bd.bd_cookie = bi.bi_cookie = bl.bl_cookie;
	bc.bc_opcode = BIOC_VCREATE_VOLUME;

	bc.bc_volid = (unsigned int)strtoul(argv[1], &endptr, 10);
	if (*endptr != '\0')
		errx(EXIT_FAILURE, "Invalid Volume ID value");

	if (argc == 7)
		if (dehumanize_number(argv[3], &volsize) == -1)
			errx(EXIT_FAILURE, "Invalid SIZE value");

	bc.bc_stripe = (unsigned int)strtoul(stripe, &endptr, 10);
	if (*endptr != '\0')
		errx(EXIT_FAILURE, "Invalid STRIPE size value");

	bc.bc_level = (unsigned int)strtoul(raid_level, &endptr, 10);
	if (*endptr != '\0')
		errx(EXIT_FAILURE, "Invalid RAID_LEVEL value");

	errstr = str2locator(scsiname, &location);
	if (errstr)
		errx(EXIT_FAILURE, "Target %s: %s", scsiname, errstr);

	/*
	 * Parse the device list that will be used for the volume,
	 * by using a bit field for the disks.
	 */
	if ((isdigit((unsigned char)argv[2][0]) == 0) || argv[2][1] != '-' ||
	    (isdigit((unsigned char)argv[2][2]) == 0))
		errx(EXIT_FAILURE, "Invalid DISKIDs value");

	disk_first = atoi(&argv[2][0]);
	disk_end = atoi(&argv[2][2]);

	for (i = disk_first; i < disk_end + 1; i++) {
		bc.bc_devmask |= (1 << i);
		user_disks++;
	}

	/*
	 * Find out how many disks are free and how much size we
	 * have available for the new volume.
	 */
	if (ioctl(fd, BIOCINQ, &bi) == -1)
		err(EXIT_FAILURE, "BIOCINQ");

	for (i = 0; i < bi.bi_nodisk; i++) {
		bd.bd_diskid = i;
		if (ioctl(fd, BIOCDISK_NOVOL, &bd) == -1)
			err(EXIT_FAILURE, "BIOCDISK_NOVOL");

		if (bd.bd_status == BIOC_SDUNUSED) {
			if (i == 0)
				first_disksize = bd.bd_size;

			total_disksize += bd.bd_size;
			nfreedisks++;
		}
	}

	/*
	 * Basic checks to be sure we don't do something stupid.
	 */
	if (nfreedisks == 0)
		errx(EXIT_FAILURE, "No free disks available");

	switch (bc.bc_level) {
	case 0:	/* RAID 0 requires at least one disk */
		if (argc == 7) {
			if (volsize > total_disksize)
				errx(EXIT_FAILURE, "volume size specified "
				   "is larger than available on free disks");
			bc.bc_size = (uint64_t)volsize;
		} else
			bc.bc_size = total_disksize;

		break;
	case 1:	/* RAID 1 requires two disks and size is total - 1 disk */
		if (nfreedisks < 2 || user_disks < 2)
			errx(EXIT_FAILURE, "2 disks are required at least for "
			    "this RAID level");

		if (argc == 7) {
			if (volsize > (total_disksize - first_disksize))
				errx(EXIT_FAILURE, "volume size specified "
				   "is larger than available on free disks");
			bc.bc_size = (uint64_t)volsize;
		} else
			bc.bc_size = (total_disksize - first_disksize);

		break;
	case 3:	/* RAID 0+1/3/5 requires three disks and size is total - 1 disk */
	case 5:
		if (nfreedisks < 3 || user_disks < 3)
			errx(EXIT_FAILURE, "3 disks are required at least for "
			    "this RAID level");

		if (argc == 7) {
			if (volsize > (total_disksize - first_disksize))
				errx(EXIT_FAILURE, "volume size specified "
				    "is larger than available on free disks");
			bc.bc_size = (uint64_t)volsize;
		} else
			bc.bc_size = (total_disksize - first_disksize);

		break;
	case 6:	/* RAID 6 requires four disks and size is total - 2 disks */
		if (nfreedisks < 4 || user_disks < 4)
			errx(EXIT_FAILURE, "4 disks are required at least for "
			    "this RAID level");

		if (argc == 7) {
			if (volsize > (total_disksize - (first_disksize * 2)))
				err(EXIT_FAILURE, "volume size specified "
				    "is larger than available on free disks");
			bc.bc_size = (uint64_t)volsize;
		} else
			bc.bc_size = (total_disksize - (first_disksize * 2));

		break;
	default:
		errx(EXIT_FAILURE, "Unsupported RAID level");
	}

	bc.bc_channel = location.channel;
	bc.bc_target = location.target;
	bc.bc_lun = location.lun;

	if (ioctl(fd, BIOCVOLOPS, &bc) == -1)
		err(EXIT_FAILURE, "BIOCVOLOPS");

        humanize_number(size, 5, bc.bc_size, "", HN_AUTOSCALE,
	    HN_B | HN_NOSPACE | HN_DECIMAL);

	printf("Created volume %u size: %s stripe: %uK level: %u "
	    "SCSI location: %u:%u.%u\n", bc.bc_volid, size, bc.bc_stripe,
	    bc.bc_level, bc.bc_channel, bc.bc_target, bc.bc_lun);
}

#ifdef notyet
/*
 * To modify a RAID volume.
 */
static void
bio_volops_modify(int fd, int argc, char **argv)
{
	/* XTRAEME: TODO */
}
#endif

/*
 * To remove a RAID volume.
 */
static void
bio_volops_remove(int fd, int argc, char **argv)
{
	struct bioc_volops	bc;
	struct locator		location;
	const char		*errstr;
	char			*endptr;

	if (argc != 3 || strcmp(argv[0], "volume") != 0)
		usage();

	memset(&bc, 0, sizeof(bc));
	bc.bc_cookie = bl.bl_cookie;
	bc.bc_opcode = BIOC_VREMOVE_VOLUME;

	bc.bc_volid = (unsigned int)strtoul(argv[1], &endptr, 10);
	if (*endptr != '\0')
		errx(EXIT_FAILURE, "Invalid Volume ID value");

	errstr = str2locator(argv[2], &location);
	if (errstr)
		errx(EXIT_FAILURE, "Target %s: %s", argv[2], errstr);

	bc.bc_channel = location.channel;
	bc.bc_target = location.target;
	bc.bc_lun = location.lun;

	if (ioctl(fd, BIOCVOLOPS, &bc) == -1)
		err(EXIT_FAILURE, "BIOCVOLOPS");

	printf("Removed volume %u at SCSI location %u:%u.%u\n",
	    bc.bc_volid, bc.bc_channel, bc.bc_target, bc.bc_lun);
}

/*
 * To blink/unblink a disk in enclosures.
 */
static void
bio_setblink(int fd, int argc, char **argv)
{
	struct locator		location;
	struct bioc_inq		bi;
	struct bioc_vol		bv;
	struct bioc_disk	bd;
	struct bioc_blink	bb;
	const char		*errstr;
	int			v, d, rv, blink = 0;

	if (argc != 2)
		usage();

	if (strcmp(argv[0], "start") == 0)
		blink = BIOC_SBBLINK;
	else if (strcmp(argv[0], "stop") == 0)
		blink = BIOC_SBUNBLINK;
	else
		usage();

	errstr = str2locator(argv[1], &location);
	if (errstr)
		errx(EXIT_FAILURE, "Target %s: %s", argv[1], errstr);

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
	if (rv == -1)
		err(EXIT_FAILURE, "BIOCINQ");

	for (v = 0; v < bi.bi_novol; v++) {
		memset(&bv, 0, sizeof(bv));
		bv.bv_cookie = bl.bl_cookie;
		bv.bv_volid = v;
		rv = ioctl(fd, BIOCVOL, &bv);
		if (rv == -1)
			err(EXIT_FAILURE, "BIOCVOL");

		for (d = 0; d < bv.bv_nodisk; d++) {
			memset(&bd, 0, sizeof(bd));
			bd.bd_cookie = bl.bl_cookie;
			bd.bd_volid = v;
			bd.bd_diskid = d;

			rv = ioctl(fd, BIOCDISK, &bd);
			if (rv == -1)
				err(EXIT_FAILURE, "BIOCDISK");

			if (bd.bd_channel == location.channel &&
			    bd.bd_target == location.target &&
			    bd.bd_lun == location.lun) {
				if (bd.bd_procdev[0] != '\0') {
					bio_blink(fd, bd.bd_procdev,
					    location.target, blink);
				} else
					warnx("Disk %s is not in an enclosure",
					   argv[1]);
				return;
			}
		}
	}

	warnx("Disk %s does not exist", argv[1]);
}

static void
bio_blink(int fd, char *enclosure, int target, int blinktype)
{
	struct bio_locate	bio;
	struct bioc_blink	blink;

	bio.bl_name = enclosure;
	if (ioctl(fd, BIOCLOCATE, &bio) == -1)
		errx(EXIT_FAILURE,
		    "Can't locate %s device via /dev/bio", enclosure);

	memset(&blink, 0, sizeof(blink));
	blink.bb_cookie = bio.bl_cookie;
	blink.bb_status = blinktype;
	blink.bb_target = target;

	if (ioctl(fd, BIOCBLINK, &blink) == -1)
		err(EXIT_FAILURE, "BIOCBLINK");
}
