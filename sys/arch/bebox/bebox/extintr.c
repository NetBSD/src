/*	$NetBSD: extintr.c,v 1.1 1997/10/14 06:47:35 sakamoto Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/cpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/pmap.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/intr.h>

extern vm_offset_t bebox_mb_reg;

#define	BEBOX_SET_MASK		0x80000000
#define	BEBOX_INTR_MASK		0x0fffffdc

#define	BEBOX_INTR(x)		(0x80000000 >> x)
#define	BEBOX_INTR_SERIAL3	BEBOX_INTR(6)
#define	BEBOX_INTR_SERIAL4	BEBOX_INTR(7)
#define	BEBOX_INTR_MIDI1	BEBOX_INTR(8)
#define	BEBOX_INTR_MIDI2	BEBOX_INTR(9)
#define	BEBOX_INTR_SCSI		BEBOX_INTR(10)
#define	BEBOX_INTR_PCI1		BEBOX_INTR(11)
#define	BEBOX_INTR_PCI2		BEBOX_INTR(12)
#define	BEBOX_INTR_PCI3		BEBOX_INTR(13)
#define	BEBOX_INTR_SOUND	BEBOX_INTR(14)
#define	BEBOX_INTR_8259		BEBOX_INTR(26)
#define	BEBOX_INTR_IRDA		BEBOX_INTR(27)
#define	BEBOX_INTR_A2D		BEBOX_INTR(28)
#define	BEBOX_INTR_GEEKPORT	BEBOX_INTR(29)

static struct {
	int intr;
	int irq;
} irq_map[] = {
	{ BEBOX_INTR_SERIAL3, 	16 },
	{ BEBOX_INTR_SERIAL4, 	17 },
	{ BEBOX_INTR_MIDI1, 	18 },
	{ BEBOX_INTR_MIDI2, 	19 },
	{ BEBOX_INTR_SCSI, 	20 },
	{ BEBOX_INTR_PCI1, 	21 },
	{ BEBOX_INTR_PCI2, 	22 },
	{ BEBOX_INTR_PCI3, 	23 },
	{ BEBOX_INTR_SOUND, 	24 },
	{ BEBOX_INTR_8259, 	25 },
	{ BEBOX_INTR_IRDA, 	26 },
	{ BEBOX_INTR_A2D, 	27 },
	{ BEBOX_INTR_GEEKPORT, 	28 },
};
static int intr_map_len = sizeof (irq_map) / sizeof (int);

static int ext_imask[NIPL];
static int ext_default_mask;

/*
 * external interrupt handler
 */
void
ext_intr()
{
	int i, irq = 0;
	volatile unsigned long cpu0_int_mask;
	volatile unsigned long int_state;
	extern long intrcnt[];

	cpu0_int_mask = BEBOX_INTR_MASK &
		*(unsigned int *)(bebox_mb_reg + 0x0f0);
	int_state = *(unsigned int *)(bebox_mb_reg + 0x2f0);

	if (int_state & BEBOX_INTR_8259) {
		irq = isa_global_intr(0);
	} else if (int_state &= cpu0_int_mask) {
		for (i = 0; i < intr_map_len; i++)
			if (int_state & irq_map[i].intr) {
				irq = isa_global_intr(irq_map[i].irq);
				break;
			}
	}

	intrcnt[irq]++;

	intr_return();
}

void
ext_intr_calculatemasks(irq, set)
	int irq;
	int set;
{
	int i, j;

	if (irq >= 16)
		for (i = 0; i < intr_map_len; i++)
			if (irq_map[i].irq == irq) {
				*(volatile unsigned int *)(bebox_mb_reg + 0x0f0)
					= (set ? BEBOX_SET_MASK : 0) |
						irq_map[i].intr;
				ext_default_mask |= irq_map[i].intr;
				break;
			}

	for (i = 0; i < NIPL; i++)
		for (j = 0; j < intr_map_len; j++)
			if (imask[i] & (1 << irq_map[j].irq))
				ext_imask[i] |= irq_map[j].intr;
}

void
ext_imask_set(pl)
	int pl;
{
	*(volatile unsigned int *)(bebox_mb_reg + 0x0f0)
		= BEBOX_SET_MASK|ext_default_mask|BEBOX_INTR_8259|0x3;
	*(volatile unsigned int *)(bebox_mb_reg + 0x0f0)
		= ext_imask[pl];

	isa_imask_set(pl);
}
