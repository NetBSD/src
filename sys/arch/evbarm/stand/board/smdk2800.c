/*	$NetBSD: smdk2800.c,v 1.2 2003/09/03 03:18:31 mycroft Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Board initialization routines for the Samsung's SMDK2800.
 */

#include <sys/types.h>
#include <lib/libsa/stand.h>
#include <arch/arm/s3c2xx0/s3c2800reg.h>

#include "board.h"

#define RAM_START	0x08000000
#define RAM_SIZE 	0x02000000

void
board_init(void)
{
	mem_init();
}

void
board_fini(void)
{

	/* Nothing to do here. */
}

void
mem_init(void)
{
	uint32_t start, size, heap;

	start = RAM_START;
	size =  RAM_SIZE;
	/* ROM monitor uses top of SDRAM for page table. */
#define ROMMONITOR_PAGETABLE  (RAM_START+RAM_SIZE-0x8000)

	heap = ROMMONITOR_PAGETABLE - HEAP_SIZE;

	printf(">> RAM 0x%x - 0x%x, heap at 0x%x\n",
	    start, (start + size) - 1, heap);
	setheap((void *)heap, (void *)(heap + HEAP_SIZE - 1));
}

#ifdef SELFCOPY_TO_FLASH
#include "flash.h"

void
protect_flash(int protect_on)
{
	volatile uint8_t *gpdatc = 
		(volatile uint8_t *)(S3C2800_GPIO_BASE+GPIO_PDATC);

	if (protect_on)
		*gpdatc &= (1<<3);
	else
		*gpdatc |= (1<<3);
}

int
get_flash_info(void *addr, struct flash_info *info)
{
	get_flash_chip_info(addr, info);

	info->page_size *= 2;
	info->data_width *= 2;
	info->writebuf_len *= 2;
	
	return 0;
}
#endif

long get_com_freq(void);
long
get_com_freq(void)
{
	long clk;
	uint16_t clkcon = *(volatile uint16_t*)(S3C2800_CLKMAN_BASE+CLKMAN_CLKCON);
	uint32_t pllcon = *(volatile uint32_t*)(S3C2800_CLKMAN_BASE+CLKMAN_PLLCON);

	int mdiv = (pllcon & PLLCON_MDIV_MASK) >> PLLCON_MDIV_SHIFT;
	int pdiv = (pllcon & PLLCON_PDIV_MASK) >> PLLCON_PDIV_SHIFT;
	int sdiv = (pllcon & PLLCON_SDIV_MASK) >> PLLCON_SDIV_SHIFT;

#if XTAL_CLK < 1000   /* in MHz */
	clk = (XTAL_CLK * 1000000 * (8 + mdiv)) / ((pdiv + 2) << sdiv);
#else /* in Hz */
	clk = (XTAL_CLK * (8 + mdiv)) / ((pdiv + 2) << sdiv);
#endif

	/*printf( "M=%d P=%d S=%d\n", mdiv, pdiv, sdiv);*/

	if (clkcon & CLKCON_HCLK)
		clk /= 2;
	if (clkcon & CLKCON_PCLK)
		clk /= 2;

	return clk;
}
