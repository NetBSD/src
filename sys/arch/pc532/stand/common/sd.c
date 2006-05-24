/*	$NetBSD: sd.c,v 1.6.12.1 2006/05/24 15:48:15 tron Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory and the Systems
 * Programming Group of the University of Utah Computer Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: sd.c 1.9 92/12/21$
 *
 *	@(#)sd.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory and the Systems
 * Programming Group of the University of Utah Computer Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: sd.c 1.9 92/12/21$
 *
 *	@(#)sd.c	8.1 (Berkeley) 6/10/93
 */

/*
 * SCSI CCS disk driver
 */


#include <sys/param.h>
#include <sys/disklabel.h>

#include <machine/stdarg.h>

#include <lib/libsa/stand.h>

#include <pc532/stand/common/samachdep.h>
#include <pc532/stand/common/so.h>

struct	sd_softc {
	int	sc_ctlr;
	int	sc_unit;
	int	sc_part;
	char	sc_retry;
	char	sc_alive;
	struct	disklabel sc_label;
} sd_softc[NSCSI][NSD];

#ifdef SD_DEBUG
int debug = SD_DEBUG;
#endif

#define	SDRETRY		2

int	sdinit(int, int);
void	sdreset(int, int);
int	sdgetinfo(struct sd_softc *);

int
sdinit(int ctlr, int unit)
{
	struct sd_softc *ss = &sd_softc[ctlr][unit];

	/*
	 * HP version does test_unit_ready
	 * followed by read_capacity to get blocksize
	 */
	ss->sc_alive = 1;
	return (1);
}

void
sdreset(int ctlr, int unit)
{

}

char io_buf[MAXBSIZE];

int
sdgetinfo(struct sd_softc *ss)
{
	struct disklabel *lp;
	char *msg;
	int i, err;

	lp = &sd_softc[ss->sc_ctlr][ss->sc_unit].sc_label;
	memset((caddr_t)lp, 0, sizeof *lp);
	lp->d_secsize = DEV_BSIZE;
	lp->d_secpercyl = 1;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_partitions[ss->sc_part].p_offset = 0;
	lp->d_partitions[ss->sc_part].p_size = 0x7fffffff;

	if ((err = sdstrategy(ss, F_READ,
		       LABELSECTOR, DEV_BSIZE, io_buf, &i)) < 0) {
	    printf("sdgetinfo: sdstrategy error %d\n", err);
	    return 0;
	}

	msg = getdisklabel(io_buf, lp);
	if (msg) {
		printf("sd(%d,%d,%d): %s\n",
		       ss->sc_ctlr, ss->sc_unit, ss->sc_part, msg);
		return 0;
	}
	return(1);
}

int
sdopen(struct open_file *f, ...)
{
	struct sd_softc *ss;
	struct disklabel *lp;
	int ctlr, unit, part;
	va_list ap;

	va_start(ap, f);
	ctlr = va_arg(ap, int);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);

#ifdef SD_DEBUG
	if (debug)
	printf("sdopen: ctlr=%d unit=%d part=%d\n",
	    ctlr, unit, part);
#endif

	if (ctlr >= NSCSI || !scsialive(ctlr))
		return (EADAPT);
	if (unit >= NSD)
		return (ECTLR);
	ss = &sd_softc[ctlr][unit];	/* XXX alloc()? keep pointers? */
	ss->sc_part = part;
	ss->sc_unit = unit;
	ss->sc_ctlr = ctlr;
	if (!ss->sc_alive) {
		if (!sdinit(ctlr, unit))
			return (ENXIO);
		if (!sdgetinfo(ss))
			return (ERDLAB);
	}
	lp = &sd_softc[ctlr][unit].sc_label;
	if (part >= lp->d_npartitions || lp->d_partitions[part].p_size == 0)
		return (EPART);

	f->f_devdata = (void *)ss;
	return (0);
}

int
sdclose(struct open_file *f)
{
	struct sd_softc *ss = f->f_devdata;

	/*
	 * Mark the disk `not alive' so that the disklabel
	 * will be re-loaded at next open.
	 */
	ss->sc_alive = 0;
	f->f_devdata = NULL;

	return (0);
}

int
sdstrategy(void *ss_vp, int func, daddr_t dblk, u_int size, void *buf,
    u_int *rsize)
{
	struct sd_softc *ss = ss_vp;
	int ctlr = ss->sc_ctlr;
	int unit = ss->sc_unit;
	int part = ss->sc_part;
	struct partition *pp = &ss->sc_label.d_partitions[part];
	u_int nblk = size >> DEV_BSHIFT;
	u_int blk = dblk + pp->p_offset;
	char status;

	if (size == 0)
		return(0);

	ss->sc_retry = 0;

#ifdef SD_DEBUG
	if (debug)
	    printf("sdstrategy(%d,%d): size=%d blk=%d nblk=%d\n",
		ctlr, unit, size, blk, nblk);
#endif

retry:
	if (func == F_READ)
		status = scsi_tt_read(ctlr, unit, buf, size, blk, nblk);
	else
		status = scsi_tt_write(ctlr, unit, buf, size, blk, nblk);
	if (status) {
		printf("sd(%d,%d,%d): block=%x, error=0x%x\n",
		       ctlr, unit, ss->sc_part, blk, status);
		if (++ss->sc_retry > SDRETRY)
			return(EIO);
		goto retry;
	}
	*rsize = size;

	return(0);
}
