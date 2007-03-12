/*	$NetBSD: mem.c,v 1.12.26.1 2007/03/12 05:47:59 rmind Exp $	*/

/*	$OpenBSD: mem.c,v 1.5 2001/05/05 20:56:36 art Exp $	*/

/*
 * Copyright (c) 1998,1999 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF MIND,
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1991,1992,1994, The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Subject to your agreements with CMU,
 * permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: mem.c 1.9 94/12/16$
 */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mem.c,v 1.12.26.1 2007/03/12 05:47:59 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>

#include <hp700/hp700/machdep.h>
#include <hp700/dev/cpudevs.h>
#include <hp700/dev/viper.h>

/* registers on the PCXL2 MIOC */
struct l2_mioc {
	uint32_t	pad[0x20];	/* 0x000 */
	uint32_t	mioc_control;	/* 0x080 MIOC control bits */
	uint32_t	mioc_status;	/* 0x084 MIOC status bits */
	uint32_t	pad1[6];	/* 0x088 */
	uint32_t	sltcv;		/* 0x0a0 L2 cache control */
#define SLTCV_AVWL	0x00002000	/* extra cycle for addr valid write low */
#define SLTCV_UP4COUT	0x00001000	/* update cache on CPU castouts */
#define SLTCV_EDCEN	0x08000000	/* enable error correction */
#define SLTCV_EDTAG	0x10000000	/* enable diagtag */
#define SLTCV_CHKTP	0x20000000	/* enable parity checking */
#define SLTCV_LOWPWR	0x40000000	/* low power mode */
#define SLTCV_ENABLE	0x80000000	/* enable L2 cache */
#define SLTCV_BITS	"\020\15avwl\16up4cout\24edcen\25edtag\26chktp\27lowpwr\30l2ena"
	uint32_t	tagmask;	/* 0x0a4 L2 cache tag mask */
	uint32_t	diagtag;	/* 0x0a8 L2 invalidates tag */
	uint32_t	sltestat;	/* 0x0ac L2 last logged tag read */
	uint32_t	slteadd;	/* 0x0b0 L2 pa of -- " -- */
	uint32_t	pad2[3];	/* 0x0b4 */
	uint32_t	mtcv;		/* 0x0c0 MIOC timings */
	uint32_t	ref;		/* 0x0cc MIOC refresh timings */
	uint32_t	pad3[4];	/* 0x0d0 */
	uint32_t	mderradd;	/* 0x0e0 addr of most evil mem error */
	uint32_t	pad4;		/* 0x0e4 */
	uint32_t	dmaerr;		/* 0x0e8 addr of most evil dma error */
	uint32_t	dioerr;		/* 0x0ec addr of most evil dio error */
	uint32_t	gsc_timeout;	/* 0x0f0 1-compl of GSC timeout delay */
	uint32_t	hidmamem;	/* 0x0f4 amount of phys mem installed */
	uint32_t	pad5[2];	/* 0x0f8 */
	uint32_t	memcomp[16];	/* 0x100 memory address comparators */
	uint32_t	memmask[16];	/* 0x140 masks for -- " -- */
	uint32_t	memtest;	/* 0x180 test address decoding */
	uint32_t	pad6[0xf];	/* 0x184 */
	uint32_t	outchk;		/* 0x1c0 address decoding output */
	uint32_t	pad7[0x168];	/* 0x200 */
	uint32_t	gsc15x_config;	/* 0x7a0 writev enable */
};

struct mem_softc {
	struct device sc_dev;

	volatile struct vi_trs *sc_vp;
	volatile struct l2_mioc *sc_l2;
};

int	memmatch(struct device *, struct cfdata *, void *);
void	memattach(struct device *, struct device *, void *);

CFATTACH_DECL(mem, sizeof(struct mem_softc),
    memmatch, memattach, NULL, NULL);

extern struct cfdriver mem_cd;

dev_type_read(mmrw);
dev_type_ioctl(mmioctl);
dev_type_mmap(mmmmap);

const struct cdevsw mem_cdevsw = {
	nullopen, nullclose, mmrw, mmrw, mmioctl,
	nostop, notty, nopoll, mmmmap,
};

static void *zeropage;

/* A lock for the vmmap. */
static struct lock vmmap_lock;

int
memmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_MEMORY ||
	    ca->ca_type.iodc_sv_model != HPPA_MEMORY_PDEP)
		return 0;
	return 1;
}

void
memattach(struct device *parent, struct device *self, void *aux)
{
	struct pdc_iodc_minit pdc_minit PDC_ALIGNMENT;
	struct confargs *ca = aux;
	struct mem_softc *sc = (struct mem_softc *)self;
	int s, err, pagezero_cookie;
	char bits[128];

	printf (":");

	pagezero_cookie = hp700_pagezero_map();

	/* XXX check if we are dealing w/ Viper */
	if (ca->ca_hpa == (hppa_hpa_t)VIPER_HPA) {

		sc->sc_vp = (struct vi_trs *)
		    &((struct iomod *)ca->ca_hpa)->priv_trs;

		/* XXX other values seem to blow it up */
		if (sc->sc_vp->vi_status.hw_rev == 0) {
			bitmask_snprintf(VI_CTRL, VIPER_BITS, bits, 
			    sizeof(bits));
			printf (" viper rev %x, ctrl %s",
			    sc->sc_vp->vi_status.hw_rev,
			    bits);

			s = splhigh();
			VI_CTRL |= VI_CTRL_ANYDEN;
			((struct vi_ctrl *)&VI_CTRL)->core_den = 0;
			((struct vi_ctrl *)&VI_CTRL)->sgc0_den = 0;
			((struct vi_ctrl *)&VI_CTRL)->sgc1_den = 0;
			((struct vi_ctrl *)&VI_CTRL)->core_prf = 1;
			sc->sc_vp->vi_control = VI_CTRL;
			splx(s);
#ifdef DEBUG
			bitmask_snprintf(VI_CTRL, VIPER_BITS, bits, 
			    sizeof(bits));
			printf (" >> %s", bits);
#endif
		} else
			sc->sc_vp = NULL;
	} else
		sc->sc_vp = NULL;

	if ((err = pdc_call((iodcio_t)pdc, 0, PDC_IODC, PDC_IODC_NINIT,
			    &pdc_minit, ca->ca_hpa, PAGE0->imm_spa_size)) < 0)
		pdc_minit.max_spa = PAGE0->imm_max_mem;

	hp700_pagezero_unmap(pagezero_cookie);

	printf (" size %d", pdc_minit.max_spa / (1024*1024));
	if (pdc_minit.max_spa % (1024*1024))
		printf (".%d", pdc_minit.max_spa % (1024*1024));
	printf ("MB");

	/* L2 cache controller is a part of the memory controller on PCXL2 */
	if (HPPA_PA_SPEC_MAJOR(hppa_cpu_info->hppa_cpu_info_pa_spec) == 1 &&
	    HPPA_PA_SPEC_MINOR(hppa_cpu_info->hppa_cpu_info_pa_spec) == 1 &&
	    HPPA_PA_SPEC_LETTER(hppa_cpu_info->hppa_cpu_info_pa_spec) == 'e') {
		sc->sc_l2 = (struct l2_mioc *)ca->ca_hpa;
#ifdef DEBUG
		bitmask_snprintf(sc->sc_l2->sltcv, SLTCV_BITS, bits,
				 sizeof(bits));
		printf(", sltcv %s", bits);
#endif
		/* sc->sc_l2->sltcv |= SLTCV_UP4COUT; */
		if (sc->sc_l2->sltcv & SLTCV_ENABLE) {
			uint32_t tagmask = sc->sc_l2->tagmask >> 20;
			printf(", %dMB L2 cache", tagmask + 1);
		}
	}
	printf("\n");
}

void
viper_setintrwnd(uint32_t mask)
{
	struct mem_softc *sc;

	sc = mem_cd.cd_devs[0];

	if (sc->sc_vp)
		sc->sc_vp->vi_intrwd;
}

void
viper_eisa_en(void)
{
	struct mem_softc *sc;
	int pagezero_cookie;

	sc = mem_cd.cd_devs[0];

	pagezero_cookie = hp700_pagezero_map();
	if (sc->sc_vp)
		((struct vi_ctrl *)&VI_CTRL)->eisa_den = 0;
	hp700_pagezero_unmap(pagezero_cookie);
}

int
mmrw(dev_t dev, struct uio *uio, int flags)
{
	struct iovec *iov;
	vaddr_t	v, o;
	vm_prot_t prot;
	u_int c;
	int error = 0;
	int rw;

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case DEV_MEM:				/*  /dev/mem  */

			/* If the address isn't in RAM, bail. */
			v = uio->uio_offset;
			if (btoc(v) > totalphysmem) {
				error = EFAULT;
				/* this will break us out of the loop */
				continue;
			}

			/*
			 * If the address is inside our large 
			 * directly-mapped kernel BTLB entries, 
			 * use kmem instead.
			 */
			if (v < virtual_start) {
				goto use_kmem;
			}

			lockmgr(&vmmap_lock, LK_EXCLUSIVE, NULL);

			/* Temporarily map the memory at vmmap. */
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ :
			    VM_PROT_WRITE;
			pmap_enter(pmap_kernel(), (vaddr_t)vmmap,
			    trunc_page(v), prot, prot|PMAP_WIRED);
			pmap_update(pmap_kernel());
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			error = uiomove((char *)vmmap + o, c, uio);
			pmap_remove(pmap_kernel(), (vaddr_t)vmmap,
			    (vaddr_t)vmmap + PAGE_SIZE);
			pmap_update(pmap_kernel());

			lockmgr(&vmmap_lock, LK_RELEASE, NULL);
			break;

		case DEV_KMEM:				/*  /dev/kmem  */
			v = uio->uio_offset;
use_kmem:
			o = v & PGOFSET;
			c = min(uio->uio_resid, (int)(PAGE_SIZE - o));
			rw = (uio->uio_rw == UIO_READ) ? B_READ : B_WRITE;
			if (!uvm_kernacc((void *)v, c, rw)) {
				error = EFAULT;
				/* this will break us out of the loop */
				continue;
			}
			error = uiomove((void *)v, c, uio);
			break;

		case DEV_NULL:				/*  /dev/null  */
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case DEV_ZERO:			/*  /dev/zero  */
			/* Write to /dev/zero is ignored. */
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			} 
			/* 
			 * On the first call, allocate and zero a page
			 * of memory for use with /dev/zero.
			 */
			if (zeropage == NULL) {
				zeropage = (void *)
				    malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(zeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}
	}
	return (error);
}

paddr_t
mmmmap(dev_t dev, off_t off, int prot)
{

	if (minor(dev) != 0)
		return (-1);

	/*
	 * Allow access only in RAM.
	 */
#if 0
	if (off < ctob(firstusablepage) ||
	    off >= ctob(lastusablepage + 1))
		return (-1);
#endif
	return (btop(off));
}
