/*	$NetBSD: installboot.c,v 1.7 1997/07/09 14:31:13 leo Exp $	*/

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
#include <sys/disklabel.h>
#include <machine/ahdilabel.h>

#include "installboot.h"

static void	usage __P((void));
static void	oscheck __P((void));
static u_int	abcksum __P((void *));
static void	setNVpref __P((void));
static void	setIDEpar __P((u_int8_t *, size_t));
static void	mkahdiboot __P((struct ahdi_root *, char *,
						char *, daddr_t));
static void	mkbootblock __P((struct bootblock *, char *,
				char *, struct disklabel *, u_int));
static void	install_fd __P((char *, struct disklabel *));
static void	install_sd __P((char *, struct disklabel *));
static void	install_wd __P((char *, struct disklabel *));

static struct bootblock	bootarea;
static struct ahdi_root ahdiboot;
static const char	mdecpath[] = PATH_MDEC;
static int		nowrite = 0;
static int		verbose = 0;
static int		trackpercyl = 0;
static int		secpertrack = 0;

static void
usage ()
{
	fprintf(stderr,
		"usage: installboot [options] device\n"
		"where options are:\n"
		"\t-N  do not actually write anything on the disk\n"
		"\t-t  number of tracks per cylinder (IDE disk)\n"
		"\t-u  number of sectors per track (IDE disk)\n"
		"\t-v  verbose mode\n");
	exit(EXIT_FAILURE);
}

int
main (argc, argv)
	int	argc;
	char	*argv[];
{
	struct disklabel dl;
	char		 *dn;
	int		 fd, c;

	/* check OS bootversion */
	oscheck();

	/* parse options */
	while ((c = getopt(argc, argv, "Nt:u:v")) != -1) {
		switch (c) {
		  case 'N':
			nowrite = 1;
			break;
		  case 't':
			trackpercyl = atoi(optarg);
			break;
		  case 'u':
			secpertrack = atoi(optarg);
			break;
		  case 'v':
			verbose = 1;
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
	dn = alloca(sizeof(_PATH_DEV) + strlen(argv[0]) + 8);
	if (!strchr(argv[0], '/')) {
		sprintf(dn, "%sr%s%c", _PATH_DEV, argv[0], RAW_PART + 'a');
		fd = open(dn, O_RDONLY);
		if (fd < 0 && errno == ENOENT) {
			sprintf(dn, "%sr%s", _PATH_DEV, argv[0]);
			fd = open(dn, O_RDONLY);
		}
	} else {
		sprintf(dn, "%s", argv[0]);
		fd = open(dn, O_RDONLY);
	}
	if (fd < 0)
		err(EXIT_FAILURE, "%s", dn);
	if (ioctl(fd, DIOCGDINFO, &dl))
		err(EXIT_FAILURE, "%s: DIOCGDINFO", dn);
	if (close(fd))
		err(EXIT_FAILURE, "%s", dn);

	switch (dl.d_type) {
		case DTYPE_FLOPPY:
			install_fd(dn, &dl);
			break;
		case DTYPE_ST506:
		case DTYPE_ESDI:
			install_wd(dn, &dl);
			setNVpref();
			break;
		case DTYPE_SCSI:
			install_sd(dn, &dl);
			setNVpref();
			break;
		default:
			errx(EXIT_FAILURE,
			     "%s: %s: Device type not supported.",
			     dn, dktypenames[dl.d_type]);
	}

	return(EXIT_SUCCESS);
}

static void
oscheck ()
{
	struct nlist	kbv[] = { { "_bootversion" }, { NULL } };
	kvm_t		*kd_kern;
	char		errbuf[_POSIX2_LINE_MAX];
	u_short		kvers;

	kd_kern = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
	if (kd_kern == NULL)
		errx(EXIT_FAILURE, "kvm_openfiles: %s", errbuf);
	if (kvm_nlist(kd_kern, kbv) == -1)
		errx(EXIT_FAILURE, "kvm_nlist: %s", kvm_geterr(kd_kern));
	if (kbv[0].n_value == 0)
		errx(EXIT_FAILURE, "%s not in namelist", kbv[0].n_name);
	if (kvm_read(kd_kern, kbv[0].n_value, (char *)&kvers,
						sizeof(kvers)) == -1)
		errx(EXIT_FAILURE, "kvm_read: %s", kvm_geterr(kd_kern));
	kvm_close(kd_kern);
	if (kvers != BOOTVERSION)
		errx(EXIT_FAILURE, "Kern bootversion: %d, expected: %d\n",
					kvers, BOOTVERSION);
}

static void
install_fd (devnm, label)
	char		 *devnm;
	struct disklabel *label;
{
	char		 *xxboot, *bootxx;
	struct partition *rootpart;

	if (label->d_secsize != 512)
		errx(EXIT_FAILURE,
		     "%s: %u: Block size not supported.", devnm,
							  label->d_secsize);
	if (label->d_ntracks != 2)
		errx(EXIT_FAILURE,
		     "%s: Single sided floppy not supported.", devnm);

	xxboot = alloca(strlen(mdecpath) + 8);
	sprintf(xxboot, "%sfdboot", mdecpath);
	bootxx = alloca(strlen(mdecpath) + 8);
	sprintf(bootxx, "%sbootxx", mdecpath);

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
install_sd (devnm, label)
	char		 *devnm;
	struct disklabel *label;
{
	char		 *xxb00t, *xxboot, *bootxx;
	struct disklabel rawlabel;
	daddr_t		 bbsec;
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

	if (bbsec) {
		xxb00t = alloca(strlen(mdecpath) + 14);
		sprintf(xxb00t, "%ssdb00t.ahdi", mdecpath);
		xxboot = alloca(strlen(mdecpath) + 14);
		sprintf(xxboot, "%sxxboot.ahdi", mdecpath);
		magic = AHDIMAGIC;
	} else {
		xxb00t = NULL;
		xxboot = alloca(strlen(mdecpath) + 8);
		sprintf(xxboot, "%ssdboot", mdecpath);
		magic = NBDAMAGIC;
	}
	bootxx = alloca(strlen(mdecpath) + 8);
	sprintf(bootxx, "%sbootxx", mdecpath);

	trackpercyl = secpertrack = 0;
	if (xxb00t)
		mkahdiboot(&ahdiboot, xxb00t, devnm, bbsec);
	mkbootblock(&bootarea, xxboot, bootxx, label, magic);

	if (!nowrite) {
		off_t	bbo = bbsec * AHDI_BSIZE;
		int	fd;

		if ((fd = open(devnm, O_WRONLY)) < 0)
			err(EXIT_FAILURE, "%s", devnm);
		if (lseek(fd, bbo, SEEK_SET) != bbo)
			err(EXIT_FAILURE, "%s", devnm);
		if (write(fd, &bootarea, sizeof(bootarea)) != sizeof(bootarea))
			err(EXIT_FAILURE, "%s", devnm);
		if (verbose)
			printf("Boot block installed on %s (%u)\n", devnm,
								    bbsec);
		if (xxb00t) {
			if (lseek(fd, (off_t)0, SEEK_SET) != 0)
				err(EXIT_FAILURE, "%s", devnm);
			if (write(fd, &ahdiboot, sizeof(ahdiboot)) != sizeof(ahdiboot))
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
install_wd (devnm, label)
	char		 *devnm;
	struct disklabel *label;
{
	char		 *xxb00t, *xxboot, *bootxx;
	struct disklabel rawlabel;
	daddr_t		 bbsec;
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

	if (bbsec) {
		xxb00t = alloca(strlen(mdecpath) + 14);
		sprintf(xxb00t, "%swdb00t.ahdi", mdecpath);
		xxboot = alloca(strlen(mdecpath) + 14);
		sprintf(xxboot, "%sxxboot.ahdi", mdecpath);
		magic = AHDIMAGIC;
	} else {
		xxb00t = NULL;
		xxboot = alloca(strlen(mdecpath) + 8);
		sprintf(xxboot, "%swdboot", mdecpath);
		magic = NBDAMAGIC;
	}
	bootxx = alloca(strlen(mdecpath) + 8);
	sprintf(bootxx, "%sbootxx", mdecpath);

	if (xxb00t)
		mkahdiboot(&ahdiboot, xxb00t, devnm, bbsec);
	mkbootblock(&bootarea, xxboot, bootxx, label, magic);

	if (!nowrite) {
		int	fd;
		off_t	bbo;

		bbo = bbsec * AHDI_BSIZE;
		if ((fd = open(devnm, O_WRONLY)) < 0)
			err(EXIT_FAILURE, "%s", devnm);
		if (lseek(fd, bbo, SEEK_SET) != bbo)
			err(EXIT_FAILURE, "%s", devnm);
		if (write(fd, &bootarea, sizeof(bootarea)) != sizeof(bootarea))
			err(EXIT_FAILURE, "%s", devnm);
		if (verbose)
			printf("Boot block installed on %s (%u)\n", devnm,
								    bbsec);
		if (xxb00t) {
			if (lseek(fd, (off_t)0, SEEK_SET) != 0)
				err(EXIT_FAILURE, "%s", devnm);
			if (write(fd, &ahdiboot, sizeof(ahdiboot))
							!= sizeof(ahdiboot))
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
mkahdiboot (newroot, xxb00t, devnm, bbsec)
	struct ahdi_root *newroot;
	char		 *xxb00t,
			 *devnm;
	daddr_t		 bbsec;
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
mkbootblock (bb, xxb, bxx, label, magic)
	struct bootblock *bb;
	char		 *xxb,
			 *bxx;
	u_int		 magic;
	struct disklabel *label;
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
setIDEpar (start, size)
	u_int8_t	*start;
	size_t		size;
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
setNVpref ()
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
abcksum (bs)
	void	*bs;
{
	u_int16_t sum  = 0,
		  *st  = (u_int16_t *)bs,
		  *end = (u_int16_t *)bs + 256;

	while (st < end)
		sum += *st++;
	return(sum);
}
