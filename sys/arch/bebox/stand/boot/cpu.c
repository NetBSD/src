/*	$NetBSD: cpu.c,v 1.4 2005/06/28 20:01:17 junyoung Exp $	*/

/*
 * This file contains information proprietary to Be Inc.
 */

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
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
 *      This product includes software developed by Gary Thomas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <stand.h>
#include "boot.h"

/*
 * Return the ordinal of the CPU on which the code runs (0/1)
 */

#define	CPU1_HRESET	0x20000000

int
whichCPU(void)
{
	volatile unsigned long *CPU_control = (unsigned long *)0x7FFFF3F0;

	if (*CPU_control & 0x02000000) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * Force CPU #1 into Hard RESET state
 */

void
resetCPU1(void)
{
	volatile unsigned long *CPU_control = (unsigned long *)0x7FFFF4F0;

	*CPU_control = CPU1_HRESET;
}

/*
 * Return state of CPU RESET register
 */
unsigned long
cpuState(void)
{
	volatile unsigned long *CPU_control = (unsigned long *)0x7FFFF4F0;

	return *CPU_control;
}

/*
 * Start CPU #1
 */
void
runCPU1(void *entry)
{
	volatile unsigned long *CPU_control = (unsigned long *)0x7FFFF4F0;
	long *PEF_vector = (long *)0x3000;
	long *PEF_vector2 = (long *)0x3018;

	PEF_vector[0] = 0;
	PEF_vector[1] = 0;
	PEF_vector[2] = 0;
	PEF_vector[3] = 0;
	PEF_vector[0] = 0;
	PEF_vector2[0] = 0;
	*CPU_control = 0x80000000 | CPU1_HRESET;
	/* Give the other CPU a chance to find the zero value */
	delay(1000);
	PEF_vector[0] = (long)entry;
	PEF_vector[1] = (long)entry;
	PEF_vector[2] = (long)entry;
	PEF_vector[3] = (long)entry;
	PEF_vector[0] = (long)entry;
	PEF_vector2[0] = (long)entry;
}

/*
 * CPU #1 runs here
 */
volatile int cpu_ctr = 0;

void
cpu1(void)
{
	while (1)
		cpu_ctr++;
}

volatile int CPU1_alive = 0;

void
start_CPU1(void)
{
	volatile long *key = (volatile long *)0x0080;

	CPU1_alive++;
	*key = 0;
	/* Wait for a kernel to load up a vector of where we should jump */
	while (*key == 0)
		delay(10);

	run(NULL, NULL, NULL, NULL, (void *)*key);
}

void
wait_for(volatile int *ptr)
{
	int i;
	for (i = 0; i < 10; i++) {
		if (*ptr)
			return;
		delay(10);
	}
	printf("CPU #1 didn't start!\n");
}
