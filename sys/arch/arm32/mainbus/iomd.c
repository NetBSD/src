/* $NetBSD: iomd.c,v 1.1 1996/08/21 19:53:21 mark Exp $ */

/*
 * Copyright (c) 1996 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iomd.c
 *
 * Probing and configuration for the IOMD
 *
 * Created      : 10/10/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <machine/io.h>
#include <machine/katelib.h>
#include <machine/cpu.h>
#include <machine/iomd.h>

#include "iomd.h"
#if NIOMD != 1
#error Need at 1 IOMD device configured
#endif

/* Declare prototypes */

/*
 * int iomdmatch(struct device *parent, void *match, void *aux)
 *
 * Just return ok for this.
 */ 
 
int
iomdmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct device *dev = match;

	if (dev->dv_unit == 0)
		return(1);
	return(0);
}


/*
 * void iomdattach(struct device *parent, struct device *dev, void *aux)
 *
 * Identify the IOMD
 */
  
void
iomdattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	int id;
	int refresh;
	int dma_time;
	int combo_time;
	int loop;

	id = ReadByte(IOMD_ID0) | (ReadByte(IOMD_ID1) << 8);

	printf(": ");

	switch (id) {
#ifdef CPU_ARM7500
		case ARM7500_IOC_ID:
			printf("ARM7500 IOMD ");
			refresh = ReadByte(IOMD_REFCR) & 0x0f;
			break;
#else
		case RPC600_IOMD_ID:
			printf("RPC IOMD ");
			refresh = ReadByte(IOMD_VREFCR) & 0x09;
			break;
#endif	/* CPU_ARM7500 */
		default:
			printf("Unknown IOMD=%04x ", id);
			refresh = 0;
			break;
	}
	printf("\n");
	printf("%s: ", self->dv_xname);
	printf("DRAM refresh=");
	switch(refresh){
		case 0x0:
			printf("off");
			break;
		case 0x1:
			printf("16us");
			break;
		case 0x2:
			printf("32us");
			break;
		case 0x4:
			printf("64us");
			break;
		case 0x8:
			printf("128us");
			break;
		default:
			printf("unknown [%02x]", refresh);
			break;
	}

	dma_time = ReadByte(IOMD_DMATCR);
	printf(", dma cycle types=");
	for (loop = 0; loop < 4; ++loop,dma_time = dma_time >> 2)
		printf("%c", 'A' + (dma_time & 3));

	combo_time = ReadByte(IOMD_IOTCR);
	printf(", combo cycle type=%c", 'A' + ((combo_time >> 2) & 3));
	printf("\n");
}

struct cfattach iomd_ca = {
	sizeof(struct device), iomdmatch, iomdattach
};

struct cfdriver iomd_cd = {
	NULL, "iomd", DV_DULL, 1
};

/* End of iomd.c */
