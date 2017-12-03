/*	$NetBSD: ka610.c,v 1.8.18.1 2017/12/03 11:36:48 jdolecek Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka610.c,v 1.8.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/nexus.h>
#include <machine/clock.h>
#include <machine/sid.h>

static void ka610_attach_cpu(device_t);
static void ka610_memerr(void);
static int ka610_mchk(void *);
static void ka610_halt(void);
static void ka610_reboot(int);

static const char * const ka610_devs[] = { "cpu", "uba", NULL };
 
/* 
 * Declaration of KA610-specific calls.
 */
const struct cpu_dep ka610_calls = {
	.cpu_mchk	= ka610_mchk,
	.cpu_memerr	= ka610_memerr, 
	.cpu_gettime	= generic_gettime,
	.cpu_settime	= generic_settime,
	.cpu_vups	= 1,	 /* ~VUPS */
	.cpu_scbsz	= 2,	/* SCB pages */
	.cpu_halt	= ka610_halt,
	.cpu_reboot	= ka610_reboot,
	.cpu_devs	= ka610_devs,
	.cpu_attach_cpu	= ka610_attach_cpu,
	.cpu_flags	= CPU_RAISEIPL,
};


void
ka610_attach_cpu(device_t self)
{
	aprint_normal(": KA610, HW rev %d, ucode rev %d\n",
	    vax_cpudata & 0xff, (vax_cpudata >> 8) & 0xff);
}

void
ka610_memerr(void)
{
	printf("Memory err!\n");
}

int
ka610_mchk(void *addr)
{
	panic("Machine check");
	return 0;
}

void
ka610_halt(void)
{
	__asm("halt");
}

void
ka610_reboot(int arg)
{
	__asm("halt");
}

