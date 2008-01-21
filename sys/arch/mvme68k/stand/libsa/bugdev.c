/*	$NetBSD: bugdev.c,v 1.9.2.1 2008/01/21 09:37:49 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/disklabel.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include "libsa.h"

void cputobsdlabel(struct disklabel *lp, struct cpu_disklabel *clp);

int errno;

struct bugsc_softc {
	int	fd;	/* Prom file descriptor */
	int	poff;	/* Partition offset */
	int	psize;	/* Partition size */
	short	ctrl;
	short	dev;
} bugsc_softc[1];

int
devopen(struct open_file *f, const char *fname, char **file)
{
	struct bugsc_softc *pp = &bugsc_softc[0];
	int	error, pn = 0;
	char	*dev, *cp;
	size_t	nrd;
	static	int iobuf[DEV_BSIZE / sizeof(int)];
	struct disklabel sdlabel;

	dev = bugargs.arg_start;

	/*
	 * Extract partition # from boot device string.
	 * The Bug command line format of this is:
	 *
	 *   147-Bug> bo [drive],,[<d>:][kernel_name] [options]
	 *
	 * Where:
	 *       [drive]         The bug LUN number, eg. 0
	 *       [<d>:]          <d> is partition # ('a' to 'h')
	 *       [kernel_name]   Eg. netbsd or /netbsd
	 *       [options]       Eg. -s
	 *
	 * At this time, all we need do is scan for a ':', and assume the
	 * preceding letter is a partition id.
	 */
	for (cp = dev + 1; *cp && cp <= bugargs.arg_end; cp++) {
		if ( *cp == ':' ) {
			pn = *(cp - 1) - 'a';
			break;
		}
	}

	if ( pn < 0 || pn >= MAXPARTITIONS ) {
		printf("Invalid partition number; defaulting to 'a'\n");
		pn = 0;
	}

	pp->fd = bugscopen(f);

	if (pp->fd < 0) {
		printf("Can't open device `%s'\n", dev);
		return ENXIO;
	}
	error = bugscstrategy(pp, F_READ, LABELSECTOR, DEV_BSIZE, iobuf, &nrd);
	if (error)
		return error;
	if (nrd != DEV_BSIZE)
		return EINVAL;

	/*LINTED*/
	cputobsdlabel(&sdlabel, (struct cpu_disklabel *)&(iobuf[0]));
	pp->poff = sdlabel.d_partitions[pn].p_offset;
	pp->psize = sdlabel.d_partitions[pn].p_size;

	f->f_dev = devsw;
	f->f_devdata = (void *)pp;
	/*LINTED*/
	*file = (char *)fname;
	return 0;
}

/* silly block scale factor */
#define BUG_BLOCK_SIZE 256
#define BUG_SCALE (512/BUG_BLOCK_SIZE)
/*ARGSUSED*/
int
bugscstrategy(void *devdata, int func, daddr_t dblk, size_t size, void *buf,
	      size_t *rsize)
{
	struct mvmeprom_dskio dio;
	struct bugsc_softc *pp = (struct bugsc_softc *)devdata;
	daddr_t	blk = dblk + pp->poff;

	twiddle();

	dio.ctrl_lun = pp->ctrl;
	dio.dev_lun = pp->dev;
	dio.status = 0;
	dio.pbuffer = buf;
	dio.blk_num = blk * BUG_SCALE;
	dio.blk_cnt = size / BUG_BLOCK_SIZE; /* assumed size in bytes */
	dio.flag = 0;
	dio.addr_mod = 0;
#ifdef DEBUG
	printf("bugscstrategy: size=%d blk=%d buf=%x\n", size, blk, buf);
	printf("ctrl %d dev %d\n", dio.ctrl_lun, dio.dev_lun);
#endif
	mvmeprom_diskrd(&dio);

	*rsize = dio.blk_cnt * BUG_BLOCK_SIZE;
#ifdef DEBUG
	printf("rsize %d status %x\n", *rsize, dio.status);
#endif

	if (dio.status)
		return EIO;
	return 0;
}

int
bugscopen(struct open_file *f, ...)
{

#ifdef DEBUG
	printf("bugscopen:\n");
#endif

	f->f_devdata = (void *)bugsc_softc;
	bugsc_softc[0].ctrl = (short)bugargs.ctrl_lun;
	bugsc_softc[0].dev =  (short)bugargs.dev_lun;
#ifdef DEBUG
	printf("using mvmebug ctrl %d dev %d\n",
	    bugsc_softc[0].ctrl, bugsc_softc[0].dev);
#endif
	return 0;
}

/*ARGSUSED*/
int
bugscclose(struct open_file *f)
{

	return EIO;
}

/*ARGSUSED*/
int
bugscioctl(struct open_file *f, u_long cmd, void *data)
{

	return EIO;
}

void
cputobsdlabel(struct disklabel *lp, struct cpu_disklabel *clp)
{
	int i;

	lp->d_magic   = (uint32_t)clp->magic1;
	lp->d_type    = (uint16_t)clp->type;
	lp->d_subtype = (uint16_t)clp->subtype;

	memcpy(lp->d_typename, clp->vid_vd, 16);
	memcpy(lp->d_packname, clp->packname, 16);

	lp->d_secsize        = (uint32_t)clp->cfg_psm;
	lp->d_nsectors       = (uint32_t)clp->cfg_spt;
	lp->d_ncylinders     = (uint32_t)clp->cfg_trk; /* trk is num of cyl! */
	lp->d_ntracks        = (uint32_t)clp->cfg_hds;
	lp->d_secpercyl      = (uint32_t)clp->secpercyl;
	lp->d_secperunit     = (uint32_t)clp->secperunit;
	lp->d_sparespertrack = (uint16_t)clp->sparespertrack;
	lp->d_sparespercyl   = (uint16_t)clp->sparespercyl;
	lp->d_acylinders     = (uint32_t)clp->acylinders;
	lp->d_rpm            = (uint16_t)clp->rpm;
	lp->d_interleave     = (uint16_t)clp->cfg_ilv;
	lp->d_trackskew      = (uint16_t)clp->cfg_sof;
	lp->d_cylskew        = (uint16_t)clp->cylskew;
	lp->d_headswitch     = (uint32_t)clp->headswitch;

	/* this silly table is for winchester drives */
	switch (clp->cfg_ssr) {
	case 0:
		lp->d_trkseek = 0;
		break;
	case 1:
		lp->d_trkseek = 6;
		break;
	case 2:
		lp->d_trkseek = 10;
		break;
	case 3:
		lp->d_trkseek = 15;
		break;
	case 4:
		lp->d_trkseek = 20;
		break;
	default:
		lp->d_trkseek = 0;
		break;
	}
	lp->d_flags = (uint32_t)clp->flags;

	for (i = 0; i < NDDATA; i++)
		lp->d_drivedata[i] = (uint32_t)clp->drivedata[i];

	for (i = 0; i < NSPARE; i++)
		lp->d_spare[i] = (uint32_t)clp->spare[i];

	lp->d_magic2      = (uint32_t)clp->magic2;
	lp->d_checksum    = (uint16_t)clp->checksum;
	lp->d_npartitions = (uint16_t)clp->partitions;
	lp->d_bbsize      = (uint32_t)clp->bbsize;
	lp->d_sbsize      = (uint32_t)clp->sbsize;

	memcpy(&(lp->d_partitions[0]), clp->vid_4,
	    sizeof(struct partition) * 4);

	/* CONSTCOND */
	memcpy(&(lp->d_partitions[4]), clp->cfg_4, sizeof(struct partition)
	    * ((MAXPARTITIONS < 16) ? (MAXPARTITIONS - 4) : 12));
}
