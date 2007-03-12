/*	$NetBSD: rd.c,v 1.7.10.1 2007/03/12 05:47:58 rmind Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah Hdr: rd.c 1.20 92/12/21
 *
 *	@(#)rd.c	8.1 (Berkeley) 7/15/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah Hdr: rd.c 1.20 92/12/21
 *
 *	@(#)rd.c	8.1 (Berkeley) 7/15/93
 */

/*
 * CS80/SS80 disk driver
 */
#include <sys/param.h>
#include <sys/disklabel.h>

#include <machine/stdarg.h>

#include <lib/libsa/stand.h>

#include <hp300/dev/rdreg.h>

#include <hp300/stand/common/conf.h>
#include <hp300/stand/common/hpibvar.h>
#include <hp300/stand/common/samachdep.h>

struct	rd_iocmd rd_ioc;
struct	rd_rscmd rd_rsc;
struct	rd_stat rd_stat;
struct	rd_ssmcmd rd_ssmc;

struct	disklabel rdlabel;

struct	rdminilabel {
	u_short	npart;
	u_long	offset[MAXPARTITIONS];
};

struct	rd_softc {
	int	sc_ctlr;
	int	sc_unit;
	int	sc_part;
	char	sc_retry;
	char	sc_alive;
	short	sc_type;
	struct	rdminilabel sc_pinfo;
};

#define	RDRETRY		5

struct	rdidentinfo {
	short	ri_hwid;
	short	ri_maxunum;
	int	ri_nblocks;
};

static int rdinit(int, int);
static int rdident(int, int);
static void rdreset(int, int);
static int rdgetinfo(struct rd_softc *);
static int rderror(int, int, int);

struct rd_softc rd_softc[NHPIB][NRD];

struct rdidentinfo rdidentinfo[] = {
	{ RD7946AID,	0,	 108416 },
	{ RD9134DID,	1,	  29088 },
	{ RD9134LID,	1,	   1232 },
	{ RD7912PID,	0,	 128128 },
	{ RD7914PID,	0,	 258048 },
	{ RD7958AID,	0,	 255276 },
	{ RD7957AID,	0,	 159544 },
	{ RD7933HID,	0,	 789958 },
	{ RD9134LID,	1,	  77840 },
	{ RD7936HID,	0,	 600978 },
	{ RD7937HID,	0,	1116102 },
	{ RD7914CTID,	0,	 258048 },
	{ RD7946AID,	0,	 108416 },
	{ RD9134LID,	1,	   1232 },
	{ RD7957BID,	0,	 159894 },
	{ RD7958BID,	0,	 297108 },
	{ RD7959BID,	0,	 594216 },
	{ RD2200AID,	0,	 654948 },
	{ RD2203AID,	0,	1309896 }
};
int numrdidentinfo = sizeof(rdidentinfo) / sizeof(rdidentinfo[0]);

int
rdinit(int ctlr, int unit)
{
	struct rd_softc *rs = &rd_softc[ctlr][unit];

	rs->sc_type = rdident(ctlr, unit);
	if (rs->sc_type < 0)
		return 0;
	rs->sc_alive = 1;
	return 1;
}

static void
rdreset(int ctlr, int unit)
{
	uint8_t stat;

	rd_ssmc.c_unit = C_SUNIT(0);
	rd_ssmc.c_cmd = C_SSM;
	rd_ssmc.c_refm = REF_MASK;
	rd_ssmc.c_fefm = FEF_MASK;
	rd_ssmc.c_aefm = AEF_MASK;
	rd_ssmc.c_iefm = IEF_MASK;
	hpibsend(ctlr, unit, C_CMD, (uint8_t *)&rd_ssmc, sizeof(rd_ssmc));
	hpibswait(ctlr, unit);
	hpibrecv(ctlr, unit, C_QSTAT, &stat, 1);
}

static int
rdident(int ctlr, int unit)
{
	struct rd_describe desc;
	uint8_t stat, cmd[3];
	char name[7];
	int id, i;

	id = hpibid(ctlr, unit);
	if ((id & 0x200) == 0)
		return -1;
	for (i = 0; i < numrdidentinfo; i++)
		if (id == rdidentinfo[i].ri_hwid)
			break;
	if (i == numrdidentinfo)
		return -1;
	id = i;
	rdreset(ctlr, unit);
	cmd[0] = C_SUNIT(0);
	cmd[1] = C_SVOL(0);
	cmd[2] = C_DESC;
	hpibsend(ctlr, unit, C_CMD, cmd, sizeof(cmd));
	hpibrecv(ctlr, unit, C_EXEC, (uint8_t *)&desc, 37);
	hpibrecv(ctlr, unit, C_QSTAT, &stat, sizeof(stat));
	memset(name, 0, sizeof(name));
	if (!stat) {
		int n = desc.d_name;
		for (i = 5; i >= 0; i--) {
			name[i] = (n & 0xf) + '0';
			n >>= 4;
		}
	}
	/*
	 * Take care of a couple of anomolies:
	 * 1. 7945A and 7946A both return same HW id
	 * 2. 9122S and 9134D both return same HW id
	 * 3. 9122D and 9134L both return same HW id
	 */
	switch (rdidentinfo[id].ri_hwid) {
	case RD7946AID:
		if (memcmp(name, "079450", 6) == 0)
			id = RD7945A;
		else
			id = RD7946A;
		break;

	case RD9134LID:
		if (memcmp(name, "091340", 6) == 0)
			id = RD9134L;
		else
			id = RD9122D;
		break;

	case RD9134DID:
		if (memcmp(name, "091220", 6) == 0)
			id = RD9122S;
		else
			id = RD9134D;
		break;
	}
	return id;
}

char io_buf[MAXBSIZE];

static int
rdgetinfo(struct rd_softc *rs)
{
	struct rdminilabel *pi = &rs->sc_pinfo;
	struct disklabel *lp = &rdlabel;
	char *msg;
	int err, savepart;
	size_t i;

	memset((void *)lp, 0, sizeof *lp);
	lp->d_secsize = DEV_BSIZE;

	/* Disklabel is always from RAW_PART. */
	savepart = rs->sc_part;
	rs->sc_part = RAW_PART;
	err = rdstrategy(rs, F_READ, LABELSECTOR,
	    lp->d_secsize ? lp->d_secsize : DEV_BSIZE, io_buf, &i);
	rs->sc_part = savepart;

	if (err) {
		printf("rdgetinfo: rdstrategy error %d\n", err);
		return 0;
	}
	
	msg = getdisklabel(io_buf, lp);
	if (msg) {
		printf("rd(%d,%d,%d): WARNING: %s\n",
		       rs->sc_ctlr, rs->sc_unit, rs->sc_part, msg);
		pi->npart = 3;
		pi->offset[0] = pi->offset[1] = -1;
		pi->offset[2] = 0;
	} else {
		pi->npart = lp->d_npartitions;
		for (i = 0; i < pi->npart; i++)
			pi->offset[i] = lp->d_partitions[i].p_size == 0 ?
				-1 : lp->d_partitions[i].p_offset;
	}
	return 1;
}

int
rdopen(struct open_file *f, ...)
{
	va_list ap;
	int ctlr, unit, part;
	struct rd_softc *rs;

	va_start(ap, f);
	ctlr = va_arg(ap, int);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);

	if (ctlr >= NHPIB || hpibalive(ctlr) == 0)
		return EADAPT;
	if (unit >= NRD)
		return ECTLR;
	rs = &rd_softc[ctlr][unit];
	rs->sc_part = part;
	rs->sc_unit = unit;
	rs->sc_ctlr = ctlr;
	if (rs->sc_alive == 0) {
		if (rdinit(ctlr, unit) == 0)
			return ENXIO;
		if (rdgetinfo(rs) == 0)
			return ERDLAB;
	}
	if (part != RAW_PART &&     /* always allow RAW_PART to be opened */
	    (part >= rs->sc_pinfo.npart || rs->sc_pinfo.offset[part] == -1))
		return EPART;
	f->f_devdata = (void *)rs;
	return 0;
}

int
rdclose(struct open_file *f)
{
	struct rd_softc *rs = f->f_devdata;

	/*
	 * Mark the disk `not alive' so that the disklabel
	 * will be re-loaded at next open.
	 */
	memset(rs, 0, sizeof(struct rd_softc));
	f->f_devdata = NULL;

	return 0;
}

int
rdstrategy(void *devdata, int func, daddr_t dblk, size_t size, void *v_buf,
    size_t *rsize)
{
	uint8_t *buf = v_buf;
	struct rd_softc *rs = devdata;
	int ctlr = rs->sc_ctlr;
	int unit = rs->sc_unit;
	daddr_t blk;
	uint8_t stat;

	if (size == 0)
		return 0;

	/*
	 * Don't do partition translation on the `raw partition'.
	 */
	blk = (dblk + ((rs->sc_part == RAW_PART) ? 0 :
	    rs->sc_pinfo.offset[rs->sc_part]));

	rs->sc_retry = 0;
	rd_ioc.c_unit = C_SUNIT(0);
	rd_ioc.c_volume = C_SVOL(0);
	rd_ioc.c_saddr = C_SADDR;
	rd_ioc.c_hiaddr = 0;
	rd_ioc.c_addr = RDBTOS(blk);
	rd_ioc.c_nop2 = C_NOP;
	rd_ioc.c_slen = C_SLEN;
	rd_ioc.c_len = size;
	rd_ioc.c_cmd = func == F_READ ? C_READ : C_WRITE;
retry:
	hpibsend(ctlr, unit, C_CMD, (uint8_t *)&rd_ioc.c_unit,
	    sizeof(rd_ioc) - 2);
	hpibswait(ctlr, unit);
	hpibgo(ctlr, unit, C_EXEC, buf, size, func);
	hpibswait(ctlr, unit);
	hpibrecv(ctlr, unit, C_QSTAT, &stat, 1);
	if (stat) {
		if (rderror(ctlr, unit, rs->sc_part) == 0)
			return EIO;
		if (++rs->sc_retry > RDRETRY)
			return EIO;
		goto retry;
	}
	*rsize = size;

	return 0;
}

static int
rderror(int ctlr, int unit, int part)
{
	uint8_t stat;

	rd_rsc.c_unit = C_SUNIT(0);
	rd_rsc.c_sram = C_SRAM;
	rd_rsc.c_ram = C_RAM;
	rd_rsc.c_cmd = C_STATUS;
	hpibsend(ctlr, unit, C_CMD, (uint8_t *)&rd_rsc, sizeof(rd_rsc));
	hpibrecv(ctlr, unit, C_EXEC, (uint8_t *)&rd_stat, sizeof(rd_stat));
	hpibrecv(ctlr, unit, C_QSTAT, &stat, 1);
	if (stat) {
		printf("rd(%d,%d,0,%d): request status fail %d\n",
		       ctlr, unit, part, stat);
		return 0;
	}
	printf("rd(%d,%d,0,%d) err: vu 0x%x",
	       ctlr, unit, part, rd_stat.c_vu);
	if ((rd_stat.c_aef & AEF_UD) || (rd_stat.c_ief & (IEF_MD|IEF_RD)))
		printf(", block %ld", rd_stat.c_blk);
	printf(", R0x%x F0x%x A0x%x I0x%x\n",
	       rd_stat.c_ref, rd_stat.c_fef, rd_stat.c_aef, rd_stat.c_ief);
	return 1;
}
