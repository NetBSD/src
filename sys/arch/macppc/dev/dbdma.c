/*	$NetBSD: dbdma.c,v 1.7.38.1 2007/06/07 20:30:43 garbled Exp $	*/

/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dbdma.c,v 1.7.38.1 2007/06/07 20:30:43 garbled Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/pio.h>
#include <macppc/dev/dbdma.h>

dbdma_command_t	*dbdma_alloc_commands = NULL;

void
dbdma_start(dmap, commands)
	dbdma_regmap_t *dmap;
	dbdma_command_t *commands;
{
	unsigned long addr = vtophys((vaddr_t)commands);

	if (addr & 0xf)
		panic("dbdma_start command structure not 16-byte aligned");

	dmap->d_intselect = 0xff;  /* Endian magic - clear out interrupts */
	DBDMA_ST4_ENDIAN(&dmap->d_control, 
			 DBDMA_CLEAR_CNTRL( (DBDMA_CNTRL_ACTIVE	|
					     DBDMA_CNTRL_DEAD	|
					     DBDMA_CNTRL_WAKE	|
					     DBDMA_CNTRL_FLUSH	|
					     DBDMA_CNTRL_PAUSE	|
					     DBDMA_CNTRL_RUN      )));      
     
	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_ACTIVE)
		;

	dmap->d_cmdptrhi = 0;	/* 64-bit not yet */
	DBDMA_ST4_ENDIAN(&dmap->d_cmdptrlo, addr);

	DBDMA_ST4_ENDIAN(&dmap->d_control, DBDMA_SET_CNTRL(DBDMA_CNTRL_RUN));
}

void
dbdma_stop(dmap)
	dbdma_regmap_t *dmap;
{
	out32rb(&dmap->d_control, DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_RUN) |
			  DBDMA_SET_CNTRL(DBDMA_CNTRL_FLUSH));

	while (in32rb(&dmap->d_status) &
		(DBDMA_CNTRL_ACTIVE|DBDMA_CNTRL_FLUSH));
}

void
dbdma_flush(dmap)
	dbdma_regmap_t *dmap;
{
	out32rb(&dmap->d_control, DBDMA_SET_CNTRL(DBDMA_CNTRL_FLUSH));

	while (in32rb(&dmap->d_status) & (DBDMA_CNTRL_FLUSH));
}

void
dbdma_reset(dmap)
	dbdma_regmap_t *dmap;
{
	out32rb(&dmap->d_control, 
			 DBDMA_CLEAR_CNTRL( (DBDMA_CNTRL_ACTIVE	|
					     DBDMA_CNTRL_DEAD	|
					     DBDMA_CNTRL_WAKE	|
					     DBDMA_CNTRL_FLUSH	|
					     DBDMA_CNTRL_PAUSE	|
					     DBDMA_CNTRL_RUN      )));      

	while (in32rb(&dmap->d_status) & DBDMA_CNTRL_RUN);
}

void
dbdma_continue(dmap)
	dbdma_regmap_t *dmap;
{
	out32rb(&dmap->d_control,
		DBDMA_SET_CNTRL(DBDMA_CNTRL_RUN | DBDMA_CNTRL_WAKE) |
		DBDMA_CLEAR_CNTRL(DBDMA_CNTRL_PAUSE | DBDMA_CNTRL_DEAD));
}

void
dbdma_pause(dmap)
	dbdma_regmap_t *dmap;
{
	DBDMA_ST4_ENDIAN(&dmap->d_control,DBDMA_SET_CNTRL(DBDMA_CNTRL_PAUSE));

	while (DBDMA_LD4_ENDIAN(&dmap->d_status) & DBDMA_CNTRL_ACTIVE)
		;
}

dbdma_command_t	*
dbdma_alloc(size)
	int size;
{
	u_int buf;

	buf = (u_int)malloc(size + 0x0f, M_DEVBUF, M_WAITOK);
	buf = (buf + 0x0f) & ~0x0f;

	return (dbdma_command_t *)buf;
}
