/*
 * Copyright (c) 1993 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
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
 *
 *	$Id: promdev.c,v 1.2 1994/07/01 10:46:59 pk Exp $
 */

#include <sys/param.h>
#include <sys/disklabel.h>
#include <machine/bsd_openprom.h>
#include "stand.h"

int promopen __P((struct open_file *, ...));
int promclose __P((struct open_file *));
int promioctl __P((struct open_file *, int, void *));
int promstrategy __P((void *, int, daddr_t, u_int, char *, u_int *));

struct devsw devsw[] = {
	{ "prom", promstrategy, promopen, promclose, promioctl },
};

int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));


struct promvec	*promvec;
struct prom_softc {
	int	fd;	/* Prom file descriptor */
	int	poff;	/* Partition offset */
	int	psize;	/* Partition size */
} prom_softc[1];

static struct disklabel sdlabel;

int
devopen(f, fname, file)
	struct open_file *f;
	char *fname;
	char **file;
{
	int	v2 = promvec->pv_romvec_vers == 2;
	register struct prom_softc	*pp = &prom_softc[0];
	int	error, i, pn = 0;
	char	*dev, *cp;
static	char	iobuf[MAXBSIZE];

	if (v2)
		dev = *promvec->pv_v2bootargs.v2_bootpath;
	else
		dev = (*promvec->pv_v0bootargs)->ba_bootdev;

	/*
	 * Extract partition # from boot device string.
	 */
	for (cp = dev; *cp; cp++) /* void */;
	while (*cp != '/' && cp > dev) {
		if (*cp == ':')
			pn = *(cp+1) - 'a';
		--cp;
	}

	if (v2)
		pp->fd = (*promvec->pv_v2devops.v2_open)(dev);
	else
		pp->fd = (*promvec->pv_v0devops.v0_open)(dev);

	if (pp->fd == 0) {
		printf("Can't open device `%s'\n", dev);
		return ENXIO;
	}
	error = promstrategy(pp, F_READ, LABELSECTOR, DEV_BSIZE, iobuf, &i);
	if (error)
		return error;
	if (i != DEV_BSIZE)
		return EINVAL;

	if (sun_disklabel(iobuf, &sdlabel) == 0) {
		printf("WARNING: no label\n");
		/* some default label */
		return EINVAL;
	} else {
		pp->poff = sdlabel.d_partitions[pn].p_offset;
		pp->psize = sdlabel.d_partitions[pn].p_size;
	}

	f->f_dev = devsw;
	f->f_devdata = (void *)pp;
	*file = fname;
	return 0;
}

#include <sparc/scsi/sun_disklabel.h>

/*
 * Take a sector (cp) containing a SunOS disk label and set lp to a BSD
 * disk label.
 */
int
sun_disklabel(cp, lp)
	register caddr_t cp;
	register struct disklabel *lp;
{
	register u_short *sp;
	register struct sun_disklabel *sl;
	register int i, v;

	sp = (u_short *)(cp + sizeof(struct sun_disklabel));
	--sp;
	v = 0;
	while (sp >= (u_short *)cp)
		v ^= *sp--;
	if (v)
		return (0);
	sl = (struct sun_disklabel *)cp;
	lp->d_magic = 0;	/* denote as pseudo */
	lp->d_ncylinders = sl->sl_ncylinders;
	lp->d_acylinders = sl->sl_acylinders;
	v = (lp->d_ntracks = sl->sl_ntracks) *
	    (lp->d_nsectors = sl->sl_nsectors);
	lp->d_secpercyl = v;
	lp->d_npartitions = 8;
	for (i = 0; i < 8; i++) {
		lp->d_partitions[i].p_offset =
		    sl->sl_part[i].sdkp_cyloffset * v;
		lp->d_partitions[i].p_size = sl->sl_part[i].sdkp_nsectors;
#ifdef DEBUG
printf("P%d: off %x, size %x\n", i,
		lp->d_partitions[i].p_offset, lp->d_partitions[i].p_size);
#endif
	}
	return (1);
}

int
promstrategy(devdata, func, dblk, size, buf, rsize)
	void *devdata;
	int func;
	daddr_t dblk;
	u_int size;
	char *buf;
	u_int *rsize;
{
	register struct prom_softc *pp = (struct prom_softc *)devdata;
	register u_int	(*pf)();
	int	v2 = promvec->pv_romvec_vers == 2;
	daddr_t	blk = dblk + pp->poff;
	int	error = 0;

#ifdef DEBUG
	printf("promstrategy: size=%d blk=%d dblk=%d\n", size, blk, dblk);
#endif
	twiddle();

	if (func == F_READ)
		pf = v2 ? (u_int (*)())promvec->pv_v2devops.v2_read:
			  (u_int (*)())promvec->pv_v0devops.v0_rbdev;
	else
		pf = v2 ? (u_int (*)())promvec->pv_v2devops.v2_write:
			  (u_int (*)())promvec->pv_v0devops.v0_wbdev;

	if (v2)
		(*promvec->pv_v2devops.v2_seek)(pp->fd, 0, blk<<DEV_BSHIFT);
	else
		(*promvec->pv_v0devops.v0_seek)(pp->fd, blk<<DEV_BSHIFT, 0);

	if (v2) {
		*rsize = (*pf)(pp->fd, buf, size);
	} else
		*rsize = (*pf)(pp->fd, size>>DEV_BSHIFT, blk, buf);

	return error;
}

int
promopen(f)
	struct open_file *f;
{
#ifdef DEBUG
	printf("promopen:\n");
#endif

	f->f_devdata = (void *)prom_softc;
	return 0;
}

int
promclose(f)
	struct open_file *f;
{
	return EIO;
}

int
promioctl(f, cmd, data)
	struct open_file *f;
	int cmd;
	void *data;
{
	return EIO;
}

getchar()
{
	register int c;
 
	c = (*promvec->pv_getchar)();
	if (c == '\r')
		c = '\n';
	return (c);
}
 
putchar(c)
	register int c;
{
	if (c == '\n')
		(*promvec->pv_putchar)('\r');
	(*promvec->pv_putchar)(c);
}

