/* $NetBSD: bootxx.c,v 1.9 2000/06/04 19:58:17 ragge Exp $ */
/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)boot.c	7.15 (Berkeley) 5/4/91
 */

#include "sys/param.h"
#include "sys/reboot.h"
#include "sys/disklabel.h"

#include "lib/libsa/stand.h"
#include "lib/libsa/ufs.h"
#include "lib/libsa/cd9660.h"

#include "lib/libkern/libkern.h"

#include "../include/pte.h"
#include "../include/sid.h"
#include "../include/mtpr.h"
#include "../include/reg.h"
#include "../include/rpb.h"

#include "../mba/mbareg.h"
#include "../mba/hpreg.h"

#define NRSP 1 /* Kludge */
#define NCMD 1 /* Kludge */

#include "dev/mscp/mscp.h"
#include "dev/mscp/mscpreg.h"

#include "../boot/data.h"

void	Xmain(void);
void	hoppabort(int);
void	romread_uvax(int lbn, int size, void *buf, struct rpb *rpb);
void	hpread(int block);
int	read750(int block, int *regs);
int	unit_init(int, struct rpb *, int);

struct open_file file;

unsigned *bootregs;
struct	rpb *rpb;
struct	bqo *bqo;
int	vax_cputype;
struct udadevice {u_short udaip;u_short udasa;};
volatile struct udadevice *csr;

extern int from;
#define	FROM750	1
#define	FROMMV	2
#define	FROMVMB	4

/*
 * The boot block are used by 11/750, 8200, MicroVAX II/III, VS2000,
 * VS3100/??, VS4000 and VAX6000/???, and only when booting from disk.
 */
void
Xmain()
{
	int io;

	vax_cputype = (mfpr(PR_SID) >> 24) & 0xFF;

	/*
	 */ 
	rpb = (void *)0xf0000; /* Safe address right now */
	bqo = (void *)0xf1000;
        if (from == FROMMV) {
		/*
		 * now relocate rpb/bqo (which are used by ROM-routines)
		 */
		bcopy ((void *)bootregs[11], rpb, sizeof(struct rpb));
		bcopy ((void*)rpb->iovec, bqo, rpb->iovecsz);
		if (rpb->devtyp == BDEV_SDN)
			rpb->devtyp = BDEV_SD;	/* XXX until driver fixed */
	} else {
		bzero(rpb, sizeof(struct rpb));
		rpb->devtyp = bootregs[0];
		rpb->unit = bootregs[3];
		rpb->rpb_bootr5 = bootregs[5];
		rpb->csrphy = bootregs[2];
		rpb->adpphy = bootregs[1];	/* BI node on 8200 */
		if (rpb->devtyp != BDEV_HP && vax_cputype == VAX_TYP_750)
			rpb->adpphy =
			    (bootregs[1] == 0xffe000 ? 0xf30000 : 0xf32000);
        }
	rpb->rpb_base = rpb;
	rpb->iovec = (int)bqo;

	io = open("/boot.vax", 0);
	if (io < 0)
		io = open("/boot", 0);
	if (io < 0)
		asm("halt");

	read(io, (void *)0x10000, 0x10000);
	bcopy((void *) 0x10000, 0, 0xffff);
	hoppabort(32);
	asm("halt");
}

/*
 * Write an extremely limited version of a (us)tar filesystem, suitable
 * for loading secondary-stage boot loader.
 * - Can only load file "boot".
 * - Must be the first file on tape.
 */
struct fs_ops file_system[] = {
	{ ufs_open, 0, ufs_read, 0, 0, ufs_stat },
	{ cd9660_open, 0, cd9660_read, 0, 0, cd9660_stat },
#if 0
	{ ustarfs_open, 0, ustarfs_read, 0, 0, ustarfs_stat },
#endif
};

int nfsys = (sizeof(file_system) / sizeof(struct fs_ops));

#if 0
int tar_open(char *path, struct open_file *f);
ssize_t tar_read(struct open_file *f, void *buf, size_t size, size_t *resid);

int
tar_open(path, f)
	char *path;
	struct open_file *f;
{
	char *buf = alloc(512);

	bzero(buf, 512);
	romstrategy(0, 0, 8192, 512, buf, 0);
	if (bcmp(buf, "boot", 5) || bcmp(&buf[257], "ustar", 5))
		return EINVAL; /* Not a ustarfs with "boot" first */
	return 0;
}

ssize_t
tar_read(f, buf, size, resid)
	struct open_file *f;
	void *buf;
	size_t size;
	size_t *resid;
{
	romstrategy(0, 0, (8192+512), size, buf, 0);
	*resid = size;
	return 0; /* XXX */
}
#endif


int
devopen(f, fname, file)
	struct open_file *f;
	const char    *fname;
	char          **file;
{
	*file = (char *)fname;

	if (from == FROM750)
		return 0;
	/*
	 * Reinit the VMB boot device.
	 */
	if (bqo->unit_init) {
		int initfn;

		initfn = rpb->iovec + bqo->unit_init;
		if (rpb->devtyp == BDEV_UDA || rpb->devtyp == BDEV_TK) {
			/*
			 * This reset do not seem to be done in the 
			 * ROM routines, so we have to do it manually.
			 */
			csr = (struct udadevice *)rpb->csrphy;
			csr->udaip = 0;
			while ((csr->udasa & MP_STEP1) == 0)
				;
		}
		/*
		 * AP (R12) have a pointer to the VMB argument list,
		 * wanted by bqo->unit_init.
		 */
		unit_init(initfn, rpb, bootregs[12]);
	}
	return 0;
}

int
romstrategy(sc, func, dblk, size, buf, rsize)
	void    *sc;
	int     func;
	daddr_t dblk;
	size_t	size;
	void    *buf;
	size_t	*rsize;
{
	int	block = dblk;
	int     nsize = size;

	if (from == FROMMV) {
		romread_uvax(block, size, buf, rpb);
	} else /* if (from == FROM750) */ {
		while (size > 0) {
			if (rpb->devtyp == BDEV_HP)
				hpread(block);
			else
				read750(block, bootregs);
			bcopy(0, buf, 512);
			size -= 512;
			(char *)buf += 512;
			block++;
		}
	}

	if (rsize)
		*rsize = nsize;
	return 0;
}

/*
 * The 11/750 boot ROM for Massbus disks doesn't seen to have layout info
 * for all RP disks (not RP07 at least) so therefore a very small and dumb
 * device driver is used. It assumes that there is a label on the disk
 * already that has valid layout info. If there is no label, we can't boot
 * anyway.
 */

#define MBA_WCSR(reg, val) \
	((void)(*(volatile u_int32_t *)((adpadr) + (reg)) = (val)));
#define MBA_RCSR(reg) \
	(*(volatile u_int32_t *)((adpadr) + (reg)))
#define HP_WCSR(reg, val) \
	((void)(*(volatile u_int32_t *)((unitadr) + (reg)) = (val)));
#define HP_RCSR(reg) \
	(*(volatile u_int32_t *)((unitadr) + (reg)))

void
hpread(int bn)
{
	int adpadr = bootregs[1];
	int unitadr = adpadr + MUREG(bootregs[3], 0);
	u_int cn, sn, tn;
	struct disklabel *dp;
	extern char start;

	dp = (struct disklabel *)(LABELOFFSET + &start);
	MBA_WCSR(MAPREG(0), PG_V);

	MBA_WCSR(MBA_VAR, 0);
	MBA_WCSR(MBA_BC, (~512) + 1);
#ifdef __GNUC__
	/*
	 * Avoid four subroutine calls by using hardware division.
	 */
	asm("clrl r1;movl %3,r0;ediv %4,r0,%0,%1;movl %1,r0;ediv %5,r0,%2,%1"
	    : "=g"(cn),"=g"(sn),"=g"(tn)
	    : "g"(bn),"g"(dp->d_secpercyl),"g"(dp->d_nsectors)
	    : "r0","r1","cc");
#else
	cn = bn / dp->d_secpercyl;
	sn = bn % dp->d_secpercyl;
	tn = sn / dp->d_nsectors;
	sn = sn % dp->d_nsectors;
#endif
	HP_WCSR(HP_DC, cn);
	HP_WCSR(HP_DA, (tn << 8) | sn);
	HP_WCSR(HP_CS1, HPCS_READ);

	while (MBA_RCSR(MBA_SR) & MBASR_DTBUSY)
		;
	return;
}

extern char end[];
static char *top = (char*)end;

void *
alloc(size)
        unsigned size;
{
	void *ut = top;
	top += size;
	return ut;
}

void
free(ptr, size)
        void *ptr;
        unsigned size;
{
}

int
romclose(f)
	struct open_file *f;
{
	return 0;
}
