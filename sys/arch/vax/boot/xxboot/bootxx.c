/* $NetBSD: bootxx.c,v 1.37.32.1 2018/05/02 07:20:05 pgoyette Exp $ */

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
 *	@(#)boot.c	7.15 (Berkeley) 5/4/91
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/exec_aout.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/cd9660.h>
#include <lib/libsa/ustarfs.h>

#include <lib/libkern/libkern.h>

#include <machine/pte.h>
#include <machine/sid.h>
#include <machine/mtpr.h>
#include <machine/reg.h>
#include <machine/rpb.h>
#include "../vax/gencons.h"

#include "../mba/mbareg.h"
#include "../mba/hpreg.h"

#define NRSP 1 /* Kludge */
#define NCMD 1 /* Kludge */

#include <dev/mscp/mscp.h>
#include <dev/mscp/mscpreg.h>

#include "../boot/data.h"

#define	RF_PROTECTED_SECTORS	64	/* XXX refer to <.../rf_optnames.h> */

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
int	vax_load_failure;
struct udadevice {u_short udaip;u_short udasa;};
volatile struct udadevice *csr;
static int moved;

extern int from;
#define	FROM750	1
#define	FROMMV	2
#define	FROMVMB	4

/*
 * The boot block are used by 11/750, 8200, MicroVAX II/III, VS2000,
 * VS3100/??, VS4000 and VAX6000/???, and only when booting from disk.
 */
void
Xmain(void)
{
	union {
		struct exec aout;
		Elf32_Ehdr elf;
	} hdr;
	int io;
	u_long entry;

	vax_cputype = (mfpr(PR_SID) >> 24) & 0xFF;
	moved = 0;
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
	} else {
		memset(rpb, 0, sizeof(struct rpb));
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
		__asm("halt");

	read(io, (void *)&hdr.aout, sizeof(hdr.aout));
	if (N_GETMAGIC(hdr.aout) == OMAGIC && N_GETMID(hdr.aout) == MID_VAX) {
		vax_load_failure++;
		entry = hdr.aout.a_entry;
		if (entry < sizeof(hdr.aout))
			entry = sizeof(hdr.aout);
		read(io, (void *) entry, hdr.aout.a_text + hdr.aout.a_data);
		memset((void *) (entry + hdr.aout.a_text + hdr.aout.a_data),
		       0, hdr.aout.a_bss);
	} else if (memcmp(hdr.elf.e_ident, ELFMAG, SELFMAG) == 0) {
		Elf32_Phdr ph;
		size_t off = sizeof(hdr.elf);
		vax_load_failure += 2;
		read(io, (char *)&hdr.elf + sizeof(hdr.aout),
		     sizeof(hdr.elf) - sizeof(hdr.aout));
		if (hdr.elf.e_machine != EM_VAX || hdr.elf.e_type != ET_EXEC
		    || hdr.elf.e_phnum != 1)
			goto die;
		vax_load_failure++;
		entry = hdr.elf.e_entry;
		if (hdr.elf.e_phoff != sizeof(hdr.elf))
			goto die;
		vax_load_failure++;
		read(io, &ph, sizeof(ph));
		off += sizeof(ph);
		if (ph.p_type != PT_LOAD)
			goto die;
		vax_load_failure++;
		while (off < ph.p_offset) {
			u_int32_t tmp;
			read(io, &tmp, sizeof(tmp));
			off += sizeof(tmp);
		}
		read(io, (void *) hdr.elf.e_entry, ph.p_filesz);
		memset((void *) (hdr.elf.e_entry + ph.p_filesz), 0,
		       ph.p_memsz - ph.p_filesz);
	} else {
		goto die;
	}
	hoppabort(entry);
die:
	__asm("halt");
}

/*
 * Write an extremely limited version of a (us)tar filesystem, suitable
 * for loading secondary-stage boot loader.
 * - Can only load file "boot".
 * - Must be the first file on tape.
 */
struct fs_ops file_system[] = {
#ifdef NEED_UFS
	{ ffsv1_open, 0, ffsv1_read, 0, 0, ffsv1_stat },
	{ ffsv2_open, 0, ffsv2_read, 0, 0, ffsv2_stat },
#endif
#ifdef NEED_CD9660
	{ cd9660_open, 0, cd9660_read, 0, 0, cd9660_stat },
#endif
#ifdef NEED_USTARFS
	{ ustarfs_open, 0, ustarfs_read, 0, 0, ustarfs_stat },
#endif
};

int nfsys = __arraycount(file_system);

#if 0
int tar_open(char *path, struct open_file *f);
ssize_t tar_read(struct open_file *f, void *buf, size_t size, size_t *resid);

int
tar_open(char *path, struct open_file *f)
{
	char *buf = alloc(512);

	memset(buf, 0, 512);
	romstrategy(0, 0, 8192, 512, buf, 0);
	if (memcmp(buf, "boot", 5) || memcmp(&buf[257], "ustar", 5))
		return EINVAL; /* Not a ustarfs with "boot" first */
	return 0;
}

ssize_t
tar_read(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	romstrategy(0, 0, (8192+512), size, buf, 0);
	*resid = size;
	return 0; /* XXX */
}
#endif


int
devopen(struct open_file *f, const char *fname, char **file)
{
	*file = (char *)fname;

	if (from == FROM750)
		return 0;
	/*
	 * Reinit the VMB boot device.
	 */
	if (bqo->unit_init && (moved++ == 0)) {
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

extern struct disklabel romlabel;

int
romstrategy(void *sc, int func, daddr_t dblk, size_t size, void *buf, size_t *rsize)
{
	int	block = dblk;
	int     nsize = size;
	char	*cbuf;

	cbuf = (char *)buf;

	if (romlabel.d_magic == DISKMAGIC && romlabel.d_magic2 == DISKMAGIC) {
		if (romlabel.d_npartitions > 1) {
			block += romlabel.d_partitions[0].p_offset;
			if (romlabel.d_partitions[0].p_fstype == FS_RAID) {
				block += RF_PROTECTED_SECTORS;
			}
		}
	}

	if (from == FROMMV) {
		romread_uvax(block, size, cbuf, rpb);
	} else /* if (from == FROM750) */ {
		while (size > 0) {
			if (rpb->devtyp == BDEV_HP)
				hpread(block);
			else
				read750(block, (int *)bootregs);
			memcpy(cbuf, 0, 512);
			size -= 512;
			cbuf += 512;
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
	__asm("clrl %%r1;"
	    "movl %3,%%r0;"
	    "ediv %4,%%r0,%0,%1;"
	    "movl %1,%%r0;"
	    "ediv %5,%%r0,%2,%1"
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
alloc(size_t size)
{
	void *ut = top;
	top += size;
	return ut;
}

void
dealloc(void *ptr, size_t size)
{
}

int
romclose(struct open_file *f)
{
	return 0;
}

#ifdef USE_PRINTF
void
putchar(int ch)
{
	/*
	 * On KA88 we may get C-S/C-Q from the console.
	 * Must obey it.
	 */
	while (mfpr(PR_RXCS) & GC_DON) {
		if ((mfpr(PR_RXDB) & 0x7f) == 19) {
			while (1) {
				while ((mfpr(PR_RXCS) & GC_DON) == 0)
					;
				if ((mfpr(PR_RXDB) & 0x7f) == 17)
					break;
			}
		}
	}

	while ((mfpr(PR_TXCS) & GC_RDY) == 0)
		;
	mtpr(0, PR_TXCS);
	mtpr(ch & 0377, PR_TXDB);
	if (ch == 10)
		putchar(13);
}
#endif
