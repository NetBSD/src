/*	$NetBSD: ka610.c,v 1.1 2001/05/01 13:17:55 ragge Exp $	*/
/*
 * Copyright (c) 2001 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of 
 *     Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/nexus.h>
#include <machine/cpu.h>
#include <machine/clock.h>
#include <machine/sid.h>

static void ka610_conf(void);
static void ka610_memerr(void);
static int ka610_mchk(caddr_t);
static void ka610_halt(void);
static void ka610_reboot(int);
 
/* 
 * Declaration of KA610-specific calls.
 */
struct cpu_dep ka610_calls = {
	NULL,
	ka610_mchk,
	ka610_memerr, 
	ka610_conf,
	generic_clkread,
	generic_clkwrite,
	1,	 /* ~VUPS */
	2,	/* SCB pages */
	ka610_halt,
	ka610_reboot,
	NULL,
	NULL,
	CPU_RAISEIPL,
};


void
ka610_conf()
{
	printf("cpu0: KA610, HW rev %d, ucode rev %d\n",
	    vax_cpudata & 0xff, (vax_cpudata >> 8) & 0xff);
}

void
ka610_memerr()
{
	printf("Memory err!\n");
}

int
ka610_mchk(caddr_t addr)
{
	panic("Machine check");
	return 0;
}

static void
ka610_halt()
{
	asm("halt");
}

static void
ka610_reboot(int arg)
{
	asm("halt");
}

