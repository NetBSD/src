/*	$NetBSD: installboot.c,v 1.34.2.1 2015/09/22 12:05:38 skrll Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <paths.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <limits.h>
#include <nlist.h>
#include <kvm.h>

#define	DKTYPENAMES
#define	FSTYPENAMES
#include <sys/disklabel.h>
#include <machine/ahdilabel.h>

#include "installboot.h"

static void	usage(void);
#ifdef CHECK_OS_BOOTVERSION
static void	oscheck(void);
#endif
static u_int	abcksum(void *);
static void	setNVpref(void);
static void	setIDEpar(u_int8_t *, size_t);
static void	mkahdiboot(struct ahdi_root *, char *,
						char *, u_int32_t);
static void	mkbootblock(struct bootblock *, char *,
				char *, struct disklabel *, u_int);
static void	install_fd(char *, struct disklabel *);
static void	install_sd(char *, struct disklabel *);
static void	install_wd(char *, struct disklabel *);

static struct bootblock	bootarea;
static struct ahdi_root ahdiboot;
static const char	mdecpath[] = PATH_MDEC;
static const char	stdpath[] = PATH_STD;
static const char	milanpath[] = PATH_MILAN;
static bool		nowrite;
static bool		verbose;
static int		trackpercyl;
static int		secpertrack;
static bool		milan;

static void
usage(void)
{
	fprintf(stderr,
		"usage: installboot [options] device\n"
#ifndef NO_USAGE
		"where options are:\n"
		"\t-N  do not actually write anything on the disk\n"
		"\t-m  use Milan boot blocks\n"
		"\t-t  number of tracks per cylinder (IDE disk)\n"
		"\t-u  number of sectors per track (IDE disk)\n"
		"\t-v  verbose mode\n"
#endif
		);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct disklabel dl;
	char		 *dn;
	char		 *devchr;
	int		 fd, c;

#ifdef CHECK_OS_BOOTVERSION
	/* check OS bootversion */
	oscheck();
#endif

	/* parse options */
	while ((c = getopt(argc, argv, "Nmt:u:v")) != -1) {
		switch (c) {
		  case 'N':
			nowrite = true;
			break;
		  case 'm':
			milan = true;
			break;
		  case 't':
			trackpercyl = atoi(optarg);
			break;
		  case 'u':
			secpertrack = atoi(optarg);
			break;
		  case 'v':
			verbose = true;
			break;
		  default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;
	if (argc != 1)
		usage();

	/* get disk label */
	size_t dnlen = sizeof(_PATH_DEV) + strlen(argv[0]) + 8;
	dn = alloca(dnlen);
	if (!strchr(argv[0], '/')) {
		snprintf(dn, dnlen, "%sr%s%c", _PATH_DEV, argv[0],
		    RAW_PART + 'a');
		fd = open(dn, O_RDONLY);
		if (fd < 0 && errno == ENOENT) {
			snprintf(dn, dnlen, "%sr%s", _PATH_DEV, argv[0]);
			fd = open(dn, O_RDONLY);
		}
	} else {
		snprintf(dn, dnlen, "%s", argv[0]);
		fd = open(dn, O_RDONLY);
	}
	if (fd < 0)
		err(EXIT_FAILURE, "%s", dn);
	if (ioctl(fd, DIOCGDINFO, &dl))
		err(EXIT_FAILURE, "%s: DIOCGDINFO", dn);
	if (close(fd))
		err(EXIT_FAILURE, "%s", dn);

	/* Eg: in /dev/fd0c, set devchr to point to the 'f' */
	devchr = strrchr(dn, '/') + 1;
	if (*devchr == 'r')
		++devchr;

	switch (*devchr) {
		case 'f': /* fd */
			install_fd(dn, &dl);
			break;
		case 'w': /* wd */
			install_wd(dn, &dl);
			setNVpref();
			break;
		case 's': /* sd */
			install_sd(dn, &dl);
			setNVpref();
			break;
		default:
			errx(EXIT_FAILURE,
			     "%s: '%c': Device type not supported.",
			     dn, *devchr);
	}

	return(EXIT_SUCCESS);
}

#ifdef CHECK_OS_BOOTVERSION
static void
oscheck(void)
{
	struct nlist kbv[] = {
		{ .n_name = "_bootversion" },
		{ .n_name = NULL }
	};
	kvm_t		*kd_kern;
	char		errbuf[_POSIX2_LINE_MAX];
	u_short		kvers;
	struct stat	sb;

	if (stat(_PATH_UNIX, &sb) < 0) {
		warnx("Cannot stat %s, no bootversion check done", _PATH_UNIX);
		return;
	}

	kd_kern = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
	if (kd_kern == NULL)
		errx(EXIT_FAILURE, "kvm_openfiles: %s", errbuf);
	if (kvm_nlist(kd_kern, kbv) == -1)
		errx(EXIT_FAILURE, "kvm_nlist: %s", kvm_geterr(kd_kern));
	if (kbv[0].n_value == 0)
		errx(EXIT_FAILURE, "%s not in namelist", kbv[0].n_name);
	if (kvm_read(kd_kern, kbv[0].n_value, &kvers, sizeof(kvers)) == -1)
		errx(EXIT_FAILURE, "kvm_read: %s", kvm_geterr(kd_kern));
	kvm_close(kd_kern);
	if (kvers != BOOTVERSION)
		errx(EXIT_FAILURE, "Kern bootversion: %d, expected: %d",
		    kvers, BOOTVERSION);
}
#endif

static void
install_fd(char *devnm, struct disklabel *label)
{
	const char	 *machpath;
	char		 *xxboot, *bootxx;
	struct partition *rootpart;

	if (label->d_secsize != 512)
		errx(EXIT_FAILURE,
		     "%s: %u: Block size not supported.", devnm,
							  label->d_secsize);
	if (label->d_ntracks != 2)
		errx(EXIT_FAILURE,
		     "%s: Single sided floppy not supported.", devnm);

	if (milan)
		machpath = milanpath;
	else
		machpath = stdpath;
	size_t xxbootlen = strlen(mdecpath) + strlen(machpath) + 8;
	xxboot = alloca(xxbootlen);
	snprintf(xxboot, xxbootlen, "%s%sfdboot", mdecpath, machpath);
	bootxx = alloca(xxbootlen);
	snprintf(bootxx, xxbootlen, "%s%sbootxx", mdecpath, machpath);

	/* first used partition (a, b or c) */		/* XXX */
	for (rootpart = label->d_partitions; ; ++rootpart) {
		if (rootpart->p_size)
			break;
	}
	if (rootpart != label->d_partitions) {		/* XXX */
		*(label->d_partitions) = *rootpart;
		memset(rootpart, 0, sizeof(*rootpart));
	}
	label->d_partitions->p_fstype = FS_BSDFFS;	/* XXX */
	label->d_npartitions = 1;
	label->d_checksum = 0;
	label->d_checksum = dkcksum(label);

	trackpercyl = secpertrack = 0;
	mkbootblock(&bootarea, xxboot, bootxx, label, 0);

	if (!nowrite) {
		int	fd;
		if ((fd = open(devnm, O_WRONLY)) < 0)
			err(EXIT_FAILURE, "%s", devnm);
		if (write(fd, &bootarea, sizeof(bootarea)) != sizeof(bootarea))
			err(EXIT_FAILURE, "%s", devnm);
		if (close(fd))
			err(EXIT_FAILURE, "%s", devnm);
		if (verbose)
			printf("Boot block installed on %s\n", devnm);
	}
}

static void
install_sd(char *devnm, struct disklabel *label)
{
	const char	 *machpath;
	char		 *xxb00t, *xxboot, *bootxx;
	struct disklabel rawlabel;
	u_int32_t	 bbsec;
	u_int		 magic;

	if (label->d_partitions[0].p_size == 0)
		errx(EXIT_FAILURE, "%s: No root-filesystem.", devnm);
	if (label->d_partitions[0].p_fstype != FS_BSDFFS)
		errx(EXIT_FAILURE, "%s: %s: Illegal root-filesystem type.",
		     devnm, fstypenames[label->d_partitions[0].p_fstype]);

	bbsec = readdisklabel(devnm, &rawlabel);
	if (bbsec == NO_BOOT_BLOCK)
		errx(EXIT_FAILURE, "%s: No NetBSD boot block.", devnm);
	if (memcmp(label, &rawlabel, sizeof(*label)))
		errx(EXIT_FAILURE, "%s: Invalid NetBSD boot block.", devnm);

	if (milan)
		machpath = milanpath;
	else
		machpath = stdpath;
	if (bbsec) {
		size_t xxb00tlen = strlen(mdecpath) + strlen(machpath) + 14;
		xxb00t = alloca(xxb00tlen);
		snprintf(xxb00t, xxb00tlen, "%s%ssdb00t.ahdi", mdecpath, machpath);
		xxboot = alloca(xxb00tlen);
		snprintf(xxboot, xxb00tlen, "%s%sxxboot.ahdi", mdecpath, machpath);
		magic = AHDIMAGIC;
	} else {
		size_t xxbootlen = strlen(mdecpath) + strlen(machpath) + 8;
		xxb00t = NULL;
		xxboot = alloca(xxbootlen);
		snprintf(xxboot, xxbootlen, "%s%ssdboot", mdecpath, machpath);
		magic = NBDAMAGIC;
	}
	size_t bootxxlen = strlen(mdecpath) + strlen(machpath) + 8;
	bootxx = alloca(bootxxlen);
	snprintf(bootxx, bootxxlen, "%s%sbootxx", mdecpath, machpath);

	trackpercyl = secpertrack = 0;
	if (xxb00t)
		mkahdiboot(&ahdiboot, xxb00t, devnm, bbsec);
	mkbootblock(&bootarea, xxboot, bootxx, label, magic);

	if (!nowrite) {
		off_t	bbo = (off_t)bbsec * AHDI_BSIZE;
		int	fd;

		if ((fd = open(devnm, O_WRONLY)) < 0)
			err(EXIT_FAILURE, "%s", devnm);
		if (lseek(fd, bbo, SEEK_SET) != bbo)
			err(EXIT_FAILURE, "%s", devnm);
		if (write(fd, &bootarea, sizeof(bootarea)) != sizeof(bootarea))
			err(EXIT_FAILURE, "%s", devnm);
		if (verbose)
			printf("Boot block installed on %s (sector %d)\n",
			    devnm, bbsec);
		if (xxb00t) {
			if (lseek(fd, (off_t)0, SEEK_SET) != 0)
				err(EXIT_FAILURE, "%s", devnm);
			if (write(fd, &ahdiboot, sizeof(ahdiboot)) !=
			    sizeof(ahdiboot))
				err(EXIT_FAILURE, "%s", devnm);
			if (verbose)
				printf("AHDI root  installed on %s (0)\n",
				    devnm);
		}
		if (close(fd))
			err(EXIT_FAILURE, "%s", devnm);
	}
}

static void
install_wd(char *devnm, struct disklabel *label)
{
	const char	 *machpath;
	char		 *xxb00t, *xxboot, *bootxx;
	struct disklabel rawlabel;
	u_int32_t	 bbsec;
	u_int		 magic;

	if (label->d_partitions[0].p_size == 0)
		errx(EXIT_FAILURE, "%s: No root-filesystem.", devnm);
	if (label->d_partitions[0].p_fstype != FS_BSDFFS)
		errx(EXIT_FAILURE, "%s: %s: Illegal root-filesystem type.",
		     devnm, fstypenames[label->d_partitions[0].p_fstype]);

	bbsec = readdisklabel(devnm, &rawlabel);
	if (bbsec == NO_BOOT_BLOCK)
		errx(EXIT_FAILURE, "%s: No NetBSD boot block.", devnm);
	if (memcmp(label, &rawlabel, sizeof(*label)))
		errx(EXIT_FAILURE, "%s: Invalid NetBSD boot block.", devnm);

	if (milan)
		machpath = milanpath;
	else
		machpath = stdpath;
	if (bbsec) {
		size_t xxb00tlen = strlen(mdecpath) + strlen(machpath) + 14;
		xxb00t = alloca(xxb00tlen);
		snprintf(xxb00t, xxb00tlen, "%s%swdb00t.ahdi", mdecpath, machpath);
		xxboot = alloca(xxb00tlen);
		snprintf(xxboot, xxb00tlen, "%s%sxxboot.ahdi", mdecpath, machpath);
		magic = AHDIMAGIC;
	} else {
		size_t xxbootlen = strlen(mdecpath) + strlen(machpath) + 8;
		xxb00t = NULL;
		xxboot = alloca(xxbootlen);
		snprintf(xxboot, xxbootlen, "%s%swdboot", mdecpath, machpath);
		magic = NBDAMAGIC;
	}
	size_t bootxxlen = strlen(mdecpath) + strlen(machpath) + 8;
	bootxx = alloca(bootxxlen);
	snprintf(bootxx, bootxxlen, "%s%sbootxx", mdecpath, machpath);

	if (xxb00t)
		mkahdiboot(&ahdiboot, xxb00t, devnm, bbsec);
	mkbootblock(&bootarea, xxboot, bootxx, label, magic);

	if (!nowrite) {
		int	fd;
		off_t	bbo;

		bbo = (off_t)bbsec * AHDI_BSIZE;
		if ((fd = open(devnm, O_WRONLY)) < 0)
			err(EXIT_FAILURE, "%s", devnm);
		if (lseek(fd, bbo, SEEK_SET) != bbo)
			err(EXIT_FAILURE, "%s", devnm);
		if (write(fd, &bootarea, sizeof(bootarea)) != sizeof(bootarea))
			err(EXIT_FAILURE, "%s", devnm);
		if (verbose)
			printf("Boot block installed on %s (sector %d)\n",
			    devnm, bbsec);
		if (xxb00t) {
			if (lseek(fd, (off_t)0, SEEK_SET) != 0)
				err(EXIT_FAILURE, "%s", devnm);
			if (write(fd, &ahdiboot, sizeof(ahdiboot))
							!= sizeof(ahdiboot))
				err(EXIT_FAILURE, "%s", devnm);
			if (verbose)
				printf("AHDI root installed on %s (sector 0)\n",
				    devnm);
		}
		if (close(fd))
			err(EXIT_FAILURE, "%s", devnm);
	}
}

static void
mkahdiboot(struct ahdi_root *newroot, char *xxb00t, char *devnm,
    u_int32_t bbsec)
{
	struct ahdi_root tmproot;
	struct ahdi_part *pd;
	int		 fd;

	/* read prototype root-sector */
	if ((fd = open(xxb00t, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "%s", xxb00t);
	if (read(fd, &tmproot, sizeof(tmproot)) != sizeof(tmproot))
		err(EXIT_FAILURE, "%s", xxb00t);
	if (close(fd))
		err(EXIT_FAILURE, "%s", xxb00t);

	/* set tracks/cylinder and sectors/track */
	setIDEpar(tmproot.ar_fill, sizeof(tmproot.ar_fill));

	/* read current root-sector */
	if ((fd = open(devnm, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "%s", devnm);
	if (read(fd, newroot, sizeof(*newroot)) != sizeof(*newroot))
		err(EXIT_FAILURE, "%s", devnm);
	if (close(fd))
		err(EXIT_FAILURE, "%s", devnm);

	/* set bootflags */
	for (pd = newroot->ar_parts; pd-newroot->ar_parts < AHDI_MAXRPD; ++pd) {
		if (pd->ap_st == bbsec) {
			pd->ap_flg = 0x21;	/* bootable, pref = NetBSD */
			goto gotit;
		}
	}
	errx(EXIT_FAILURE,
	     "%s: NetBSD boot block not on primary AHDI partition.", devnm);

gotit:	/* copy code from prototype and set new checksum */
	memcpy(newroot->ar_fill, tmproot.ar_fill, sizeof(tmproot.ar_fill));
	newroot->ar_checksum = 0;
	newroot->ar_checksum = 0x1234 - abcksum(newroot);

	if (verbose)
		printf("AHDI      boot loader: %s\n", xxb00t);
}

static void
mkbootblock(struct bootblock *bb, char *xxb, char *bxx,
    struct disklabel *label, u_int magic)
{
	int		 fd;

	memset(bb, 0, sizeof(*bb));

	/* set boot block magic */
	bb->bb_magic = magic;

	/* set disk pack label */
	BBSETLABEL(bb, label);

	/* set second-stage boot loader */
	if ((fd = open(bxx, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "%s", bxx);
	if (read(fd, bb->bb_bootxx, sizeof(bb->bb_bootxx))
					!= sizeof(bb->bb_bootxx))
		err(EXIT_FAILURE, "%s", bxx);
	if (close(fd))
		err(EXIT_FAILURE, "%s", bxx);

	/* set first-stage bootloader */
	if ((fd = open(xxb, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "%s", xxb);
	if (read(fd, bb->bb_xxboot, sizeof(bb->bb_xxboot))
					!= sizeof(bb->bb_xxboot))
		err(EXIT_FAILURE, "%s", xxb);
	if (close(fd))
		err(EXIT_FAILURE, "%s", xxb);

	/* set tracks/cylinder and sectors/track */
	setIDEpar(bb->bb_xxboot, sizeof(bb->bb_xxboot));

	/* set AHDI checksum */
	*((u_int16_t *)bb->bb_xxboot + 255) = 0;
	*((u_int16_t *)bb->bb_xxboot + 255) = 0x1234 - abcksum(bb->bb_xxboot);

	if (verbose) {
		printf("Primary   boot loader: %s\n", xxb);
		printf("Secondary boot loader: %s\n", bxx);
	}
}

static void
setIDEpar (u_int8_t *start, size_t size)
{
	static const u_int8_t	mark[] = { 'N', 'e', 't', 'B', 'S', 'D' };

	if ((u_int)trackpercyl > 255)
		errx(EXIT_FAILURE,
		     "%d: Illegal tracks/cylinder value (1..255)", trackpercyl);
	if ((u_int)secpertrack > 255)
		errx(EXIT_FAILURE,
		     "%d: Illegal sectors/track value (1..255)", secpertrack);

	if (trackpercyl || secpertrack) {
		u_int8_t *p;

		if (!trackpercyl)
			errx(EXIT_FAILURE, "Need tracks/cylinder too.");
		if (!secpertrack)
			errx(EXIT_FAILURE, "Need sectors/track too.");

		start += 2;
		size  -= sizeof(mark) + 2;
		for (p = start + size; p >= start; --p) {
			if (*p != *mark)
				continue;
			if (!memcmp(p, mark, sizeof(mark)))
				break;
		}
		if (p < start)
			errx(EXIT_FAILURE,
			     "Malformatted xxboot prototype.");

		*--p = secpertrack;
		*--p = trackpercyl;

		if (verbose) {
			printf("sectors/track  : %d\n", secpertrack);
			printf("tracks/cylinder: %d\n", trackpercyl);
		}
	}
}

static void
setNVpref(void)
{
	static const u_char	bootpref = BOOTPREF_NETBSD;
	static const char	nvrdev[] = PATH_NVRAM;

	if (!nowrite) {
		int	fd;

		if ((fd = open(nvrdev, O_RDWR)) < 0)
			err(EXIT_FAILURE, "%s", nvrdev);
		if (lseek(fd, (off_t)1, SEEK_SET) != 1)
			err(EXIT_FAILURE, "%s", nvrdev);
		if (write(fd, &bootpref, (size_t)1) != 1)
			err(EXIT_FAILURE, "%s", nvrdev);
		if (close(fd))
			err(EXIT_FAILURE, "%s", nvrdev);
		if (verbose)
			printf("Boot preference set to NetBSD.\n");
	}
}

static u_int
abcksum (void *bs)
{
	u_int16_t sum  = 0,
		  *st  = (u_int16_t *)bs,
		  *end = (u_int16_t *)bs + 256;

	while (st < end)
		sum += *st++;
	return(sum);
}
