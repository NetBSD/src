/*	$NetBSD: ga.c,v 1.5 2009/11/07 07:27:43 cegger Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
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

/* Graphic Adaptor  (350, 360) */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ga.c,v 1.5 2009/11/07 07:27:43 cegger Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#ifdef _STANDALONE
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include "local.h"
#endif
#ifdef _KERNEL
#include <uvm/uvm_extern.h>
#include <machine/vmparam.h>
#endif
#include <machine/gareg.h>
#include <machine/gavar.h>

bool ga_map(struct ga *);
void ga_clut_init(struct ga *);
void ga_vblank_start(const struct ga *);
void ga_bt463_reg(const struct ga *, int);
void ga_bt463_data(const struct ga *, int);
void ga_bt463_reg_data(const struct ga *, int, int);
#ifdef _STANDALONE
void ga_dda_busy(const struct ga *);
void ga_ovl_init(const struct ga *);
void ga_id_init(const struct ga *);
void ga_block_clear(const struct ga *);
void ga_plane_mask_test(const struct ga *);
#endif

#define	ga_reg_write(ga, ofs, val)					\
	(*(volatile uint32_t *)((ga)->reg_addr + (ofs)) = (val))
#define	ga_reg_read(ga, ofs)						\
	(*(volatile uint32_t *)((ga)->reg_addr + (ofs)))

bool
ga_init(struct ga *ga)
{
	int i;

	/* Map GA register and buffers */
	if (ga->reg_addr == 0 && ga_map(ga) != 0)
		return false;

	/* This is 350 GA-ROM initialization sequence. */
	if (ga->flags == 0x0000) {
		ga_bt463_reg_data(ga, 0x201, 0x40);
		ga_bt463_reg_data(ga, 0x202, 0x40);
		ga_bt463_reg_data(ga, 0x203,
		    ((ga_reg_read(ga, 0xe00) & 2) << 6) | 0x40);
	} else if (ga->flags == 0x0001) {
		ga_bt463_reg_data(ga, 0x201, 0x40);
		ga_bt463_reg_data(ga, 0x202, 0);
		ga_bt463_reg_data(ga, 0x203,
		    ((ga_reg_read(ga, 0xe00) & 2) << 6) | 0x40);
		ga_bt463_reg_data(ga, 0x204, 0xff);	/* Display ON/OFF ? */
		ga_bt463_reg_data(ga, 0x206, 0);
		ga_bt463_reg_data(ga, 0x20a, 0);
	}

	/* Window type table */
	ga_bt463_reg(ga, 0x300);
	for (i = 0; i < 16; i++) {
		ga_bt463_data(ga, 0x00);
		ga_bt463_data(ga, 0xe1);
		ga_bt463_data(ga, 0x01);
	}

	ga_vblank_start(ga);

	/* ??? */
	ga_bt463_reg(ga, 0x302);
	for (i = 0; i < 2; i++) {
		ga_bt463_data(ga, 0x00);
		ga_bt463_data(ga, 0xe3);
		ga_bt463_data(ga, 0x21);
	}

	/* Read mask P0-P7 */
	if (ga->flags != 0x0001) {
		/* TR2A display blinks if this was done.. */
		ga_bt463_reg(ga, 0x205);
		for (i = 0; i < 4; i++)
			ga_bt463_data(ga, 0xff);
	}

	/* Blink mask P0-P7 */
	ga_bt463_reg(ga, 0x209);
	for (i = 0; i < 4; i++)
		ga_bt463_data(ga, 0x00);

	ga_clut_init(ga);

	/* ??? */
	ga_bt463_reg(ga, 0x200);
	for (i = 0; i < 0xff; i++) {
		ga_reg_write(ga, 0xc8c, 0);
		ga_reg_write(ga, 0xc8c, 0);
		ga_reg_write(ga, 0xc8c, 0);
	}

	if (ga_reg_read(ga, 0xe00) & 2)
		ga_reg_write(ga, 0xe08, 0x790);	/* 71Hz */
	else
		ga_reg_write(ga, 0xe08, 0x670); /* 60Hz */
#ifdef _STANDALONE
	ga_block_clear(ga);
	ga_ovl_init(ga);
	ga_id_init(ga);
#endif
	/* Cursor RAM clear */
	ga_reg_write(ga, 0xc90, 0);
	ga_reg_write(ga, 0xc94, 0);
	ga_reg_write(ga, 0xca0, 0);
	ga_reg_write(ga, 0xca4, 0);
	for (i = 0; i < 512; i++) {
		ga_reg_write(ga, 0xc98, 0);
		ga_reg_write(ga, 0xca8, 0);
	}

	return true;
}

bool
ga_map(struct ga *ga)
{
#ifdef _STANDALONE
	/* IPL maps register region using 16Mpage */
	ga->reg_addr = GA_REG_ADDR;
#endif
#ifdef _KERNEL
	paddr_t pa, epa;
	vaddr_t va, tva;

	pa = (paddr_t)GA_REG_ADDR;
	epa = pa + GA_REG_SIZE;

	if (!(va = uvm_km_alloc(kernel_map, epa - pa, 0, UVM_KMF_VAONLY))) {
		printf("can't map GA register.\n");
		return false;
	}

	for (tva = va; pa < epa; pa += PAGE_SIZE, tva += PAGE_SIZE)
		pmap_kenter_pa(tva, pa, VM_PROT_READ | VM_PROT_WRITE, 0);

	pmap_update(pmap_kernel());

	ga->reg_addr = (uint32_t)va;
#endif

	return true;
}

void
ga_vblank_start(const struct ga *ga)
{

	while ((ga_reg_read(ga, 0xe00) & 0x1) == 0)	/* V-blank */
		;
	while ((ga_reg_read(ga, 0xe00) & 0x1) == 1)
		;
	/* V-blank start */
}

/* Bt463 utils */
void
ga_bt463_reg(const struct ga *ga, int r)
{

	ga_reg_write(ga, 0xc80, r & 0xff);
	ga_reg_write(ga, 0xc84, (r >> 8) & 0xff);
}

void
ga_bt463_data(const struct ga *ga, int v)
{

	ga_reg_write(ga, 0xc88, v & 0xff);
}

void
ga_bt463_reg_data(const struct ga *ga, int r, int v)
{

	ga_bt463_reg(ga, r);
	ga_bt463_data(ga, v);
}

/* CLUT */
void
ga_clut_init(struct ga *ga)
{
	const uint8_t compo6[6] = { 0, 51, 102, 153, 204, 255 };
	const uint8_t ansi_color[16][3] = {
		{ 0x00, 0x00, 0x00 },
		{ 0xff, 0x00, 0x00 },
		{ 0x00, 0xff, 0x00 },
		{ 0xff, 0xff, 0x00 },
		{ 0x00, 0x00, 0xff },
		{ 0xff, 0x00, 0xff },
		{ 0x00, 0xff, 0xff },
		{ 0xff, 0xff, 0xff },
		{ 0x00, 0x00, 0x00 },
		{ 0x80, 0x00, 0x00 },
		{ 0x00, 0x80, 0x00 },
		{ 0x80, 0x80, 0x00 },
		{ 0x00, 0x00, 0x80 },
		{ 0x80, 0x00, 0x80 },
		{ 0x00, 0x80, 0x80 },
		{ 0x80, 0x80, 0x80 },
	};
	int i, j, r, g, b;

	ga_bt463_reg(ga, 0);
	/* ANSI escape sequence */
	for (i = 0; i < 16; i++) {
		ga_reg_write(ga, 0xc8c, ga->clut[i][0] = ansi_color[i][0]);
		ga_reg_write(ga, 0xc8c, ga->clut[i][1] = ansi_color[i][1]);
		ga_reg_write(ga, 0xc8c, ga->clut[i][2] = ansi_color[i][2]);
	}

	/* 16 - 31, gray scale */
	for ( ; i < 32; i++) {
		j = (i - 16) * 17;
		ga_reg_write(ga, 0xc8c, ga->clut[i][0] = j);
		ga_reg_write(ga, 0xc8c, ga->clut[i][1] = j);
		ga_reg_write(ga, 0xc8c, ga->clut[i][2] = j);
	}

	/* 32 - 247, RGB color */
	for (r = 0; r < 6; r++) {
		for (g = 0; g < 6; g++) {
			for (b = 0; b < 6; b++, i++) {
				ga_reg_write(ga, 0xc8c,
				    ga->clut[i][0] = compo6[r]);
				ga_reg_write(ga, 0xc8c,
				    ga->clut[i][1] = compo6[g]);
				ga_reg_write(ga, 0xc8c,
				    ga->clut[i][2] = compo6[b]);
			}
		}
	}

	/* 248 - 256, white */
	for ( ; i < 256; i++) {
		ga_reg_write(ga, 0xc8c, ga->clut[i][0] = 0xff);
		ga_reg_write(ga, 0xc8c, ga->clut[i][1] = 0xff);
		ga_reg_write(ga, 0xc8c, ga->clut[i][2] = 0xff);
	}

	/* 257 - 528, black */
	for ( ; i < 528; i++) {
		ga_reg_write(ga, 0xc8c, 0);
		ga_reg_write(ga, 0xc8c, 0);
		ga_reg_write(ga, 0xc8c, 0);
	}
}

void
ga_clut_get(struct ga *ga)
{
	int i;

	ga_bt463_reg(ga, 0);
	for (i = 0; i < 256; i++) {
		ga->clut[i][0] = ga_reg_read(ga, 0xc8c);
		ga->clut[i][1] = ga_reg_read(ga, 0xc8c);
		ga->clut[i][2] = ga_reg_read(ga, 0xc8c);
	}
}

void
ga_clut_set(const struct ga *ga)
{
	int i;

	ga_bt463_reg(ga, 0);
	for (i = 0; i < 256; i++) {
		ga_reg_write(ga, 0xc8c, ga->clut[i][0]);
		ga_reg_write(ga, 0xc8c, ga->clut[i][1]);
		ga_reg_write(ga, 0xc8c, ga->clut[i][2]);
	}
}

/* Not yet analyzed. */
#ifdef _STANDALONE
void
ga_dda_busy(const struct ga *ga)
{

	while ((ga_reg_read(ga, 0xf00) & 0x8000) == 0)
		;
}

void
ga_ovl_init(const struct ga *ga)
{
	uint32_t *p0, *p1;

	ga_reg_write(ga, 0x400, 0xffffffff);
	p0 = (uint32_t *)0xf2000000;
	p1 = (uint32_t *)0xf2200000;
	while (p0 < p1)
		*p0++ = 0;
}

void
ga_id_init(const struct ga *ga)
{
	uint32_t *p0, *p1;

	p0 = (uint32_t *)0xf3000000;
	p1 = (uint32_t *)0xf3040000;
	while (p0 < p1)
		*p0++ = 0;
}

void
ga_block_clear(const struct ga *ga)
{
	uint32_t *p0, *p1;

	ga_reg_write(ga, 0xe80, 0);
	ga_reg_write(ga, 0x400, 0xffffff);

	p0 = (uint32_t *)0xf0c00000;
	p1 = (uint32_t *)0xf0c80000;
	while (p0 < p1)
		*p0++ = 0xffffffff;
}

void
ga_plane_mask_test(const struct ga *ga)
{
	int i;

	ga_reg_write(ga, 0x400, 0xffffff);
	*(volatile uint32_t *)0xf1000000 = 0;

	ga_reg_write(ga, 0x400, 0xaaaaaa);
	*(volatile uint32_t *)0xf1000000 = 0xffffff;

	if ((*(volatile uint32_t *)0xf1000000 & 0xffffff) != 0xaaaaaa)
		goto err;
	ga_reg_write(ga, 0x400, 0xffffff);
	*(volatile uint32_t *)0xf1000000 = 0;


	*(volatile uint32_t *)0xf1080008 = 0;
	ga_reg_write(ga, 0x400, 0x555555);
	*(volatile uint32_t *)0xf1080008 = 0xffffff;
	if ((*(volatile uint32_t *)0xf1080008 & 0xffffff) != 0x555555)
		goto err;
	ga_reg_write(ga, 0x400, 0xffffff);
	*(volatile uint32_t *)0xf1080008 = 0;

	*(volatile uint32_t *)0xf1100000 = 0;
	*(volatile uint32_t *)0xf1100000 = 0xffffff;
	if ((*(volatile uint32_t *)0xf1100000 & 0xffffff) != 0xffffff)
		goto err;

	ga_reg_write(ga, 0x400, 0xaaaaaa);
	*(volatile uint32_t *)0xf1100000 = 0;
	if ((*(volatile uint32_t *)0xf1100000 & 0xffffff) != 0x555555)
		goto err;

	ga_reg_write(ga, 0x400, 0);
	*(volatile uint32_t *)0xf1100000 = 0xffffff;
	if ((*(volatile uint32_t *)0xf1100000 & 0xffffff) != 0x555555)
		goto err;

	ga_reg_write(ga, 0x400, 0xffffff);
	*(volatile uint32_t *)0xf1100000 = 0;

	ga_reg_write(ga, 0xe80, 0xffffff);
	ga_reg_write(ga, 0x400, 0xffffff);
	*(volatile uint32_t *)0xf0c00000 = 0xffffffff;
	for (i = 0; i < 32; i++)
		if ((*(volatile uint32_t *)(0xf1000000 + i * 4) & 0xffffff) !=
		    0xffffff)
			goto err;

	ga_reg_write(ga, 0xe80, 0);
	ga_reg_write(ga, 0x400, 0xaaaaaa);
	*(volatile uint32_t *)0xf0c00000 = 0xffffffff;
	for (i = 0; i < 32; i++)
		if ((*(volatile uint32_t *)(0xf1000000 + i * 4) & 0xffffff) !=
		    0x555555)
			goto err;
	ga_reg_write(ga, 0x400, 0x555555);
	*(volatile uint32_t *)0xf0c00000 = 0xffffffff;
	for (i = 0; i < 32; i++)
		if ((*(volatile uint32_t *)(0xf1000000 + i * 4) & 0xffffff) !=
		    0x0)
			goto err;

	printf("SUCCESS\n");
	return;
 err:
	printf("ERROR\n");
}
#endif /* _STANDALONE */
