/*	$NetBSD: sif.c,v 1.5.60.1 2007/12/26 19:42:36 ad Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sif.c,v 1.5.60.1 2007/12/26 19:42:36 ad Exp $");

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <playstation2/playstation2/sifbios.h>
#include <playstation2/ee/dmacvar.h>
#include <playstation2/ee/sifvar.h>

#ifdef SIF_DEBUG
int __spd_total_alloc;
int	sif_debug = 1;
#define	DPRINTF(fmt, args...)						\
	if (sif_debug)							\
		printf("%s: " fmt, __func__ , ##args) 
#define	DPRINTFN(n, arg)						\
	if (sif_debug > (n))						\
n		printf("%s: " fmt, __func__ , ##args) 
#else
#define	DPRINTF(arg...)		((void)0)
#define DPRINTFN(n, arg...)	((void)0)
#endif

#define ROUND64(x)	((((u_int32_t)(x)) + 63) & ~64)
#define BCD_TO_DECIMAL(x)						\
({									\
	typeof(x) x_ = x;						\
	(((x_) >> 4) * 10 + ((x_) & 0xf));				\
})
#define MAJOR(x)	BCD_TO_DECIMAL(((x) >> 8) & 0xff)
#define MINOR(x)	BCD_TO_DECIMAL((x) & 0xff)

void
sif_init()
{
	u_int32_t vers;

	vers = sifbios_getver();
	printf("PlayStation 2 SIF BIOS version %d.%d\n",
	    MAJOR(vers), MINOR(vers));

	/* Initialize SIF */
	if (sifdma_init() < 0)
		panic("SIFDMA");
	if (sifcmd_init() < 0)
		panic("SIFCMD");
	dmac_intr_establish(D_CH5_SIF0, IPL_TTY, sifcmd_intr, 0);
	if (sifrpc_init() < 0)
		panic("SIFRPC");
	if (iopmem_init() < 0)
		panic("IOP Memory");
}

void
sif_exit()
{

	sifrpc_exit();
	sifcmd_exit();
	dmac_intr_disestablish((void *)D_CH5_SIF0);
	sifdma_exit();
}

int
iopdma_allocate_buffer(struct iopdma_segment *seg, size_t size)
{
	/* 
	 * To avoid cache inconsistecy as the result of DMA(to memory), 
	 * DMA buffer size is setted to multiple of CPU cache line size (64B)
	 * and aligned to cache line.
	 */
	seg->size = ROUND64(size);

	seg->iop_paddr = iopmem_alloc(seg->size);

	if (seg->iop_paddr == 0) {
		printf("%s: can't allocate IOP memory.\n", __func__);
		DPRINTF("request = %d byte, current total = %#x\n",
		    size, __spd_total_alloc);
		return (1);
	}

	seg->ee_vaddr = IOPPHYS_TO_EEKV(seg->iop_paddr);

#ifdef SIF_DEBUG
	__spd_total_alloc += size;
#endif
	DPRINTF("0x%08lx+%#x (total=%#x)\n", seg->iop_paddr, size,
	    __spd_total_alloc);

	KDASSERT((seg->ee_vaddr & 63) == 0);

	return (0);
}

void
iopdma_free_buffer(struct iopdma_segment *seg)
{
	int ret;

	ret = iopmem_free(seg->iop_paddr);
#ifdef SIF_DEBUG
	__spd_total_alloc -= seg->size;
#endif
	DPRINTF("0x%08lx (total=%#x, result=%d)\n", seg->iop_paddr,
	    __spd_total_alloc, ret);
}
