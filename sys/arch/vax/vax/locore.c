/*	$NetBSD: locore.c,v 1.1 1994/11/25 19:09:56 ragge Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
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

 /* All bugs are subject to removal without further notice */
		

#include "machine/cpu.h"
#include "machine/sid.h"
#include "machine/param.h"
#include "machine/loconf.h"
#include "sys/types.h"
#include "machine/param.h"
#include "machine/vmparam.h"
#include "vm/vm.h"

#define ROUND_PAGE(x)   (((uint)(x)+PAGE_SIZE-1)& ~(PAGE_SIZE-1))

u_int	proc0paddr;

start()
{
	extern u_int *end, v_cmap, p_cmap;
	register curtop,i,maxmem;
	extern struct cpu_dep cpu_calls[];

	mtpr(0x1f,PR_IPL); /* No interrupts before istack is ok, please */

	asm("movl r10,_bootdev");   /* XXX stack should have been setup */
	asm("movl r11,_boothowto"); /* in boot, easier to transfer args */

	/* FIRST we must set up kernel stack, directly after end */
	/* This is the only thing we have to setup here, rest in pmap */
	PAGE_SIZE = NBPG*2; /* Set logical page size */
	proc0paddr=ROUND_PAGE(&end);
	mtpr(proc0paddr+UPAGES*NBPG,PR_KSP);

/*
 * Set logical page size and put Sysmap on its place.
 */
	Sysmap=(struct pte *)ROUND_PAGE(mfpr(PR_KSP));


	/* Be sure we are in system space. XXX should be done in boot */
	asm("	pushl   $0x001f0000
		pushl   $to_kmem
		rei
to_kmem:
	");

	/* Be sure some important internal registers have safe values */
	mtpr(0,PR_P0LR);
	mtpr(0,PR_P0BR);
	mtpr(0,PR_P1LR);
	mtpr(0x80000000,PR_P1BR);

	mtpr(0,PR_MAPEN); /* No memory mapping yet */
	mtpr(0,PR_SCBB); /* SCB at physical addr 0 */
	mtpr(0,PR_ESP); /* Must be zero, used in page fault routine */
	
	cninit();
printf("proc0paddr %x, end %x\n",proc0paddr,&end);

	/* Count up memory etc... early machine dependent routines */
	if((i=MACHID(mfpr(PR_SID)))>VAX_MAX) i=0;
	maxmem=(cpu_calls[i].cpu_loinit)();

	pmap_bootstrap(0,maxmem);

/*	mtpr(0x80000000,PR_KSP); /* Change ksp to new value :) */
	cpu_type=mfpr(PR_SID);
	main();
	/* XXX det {r dumt att fastna vid att init startar p} 2 */
	asm("
		pushl   $0x3c00000
		pushl   $2
		rei
	");

	/* Notreached */
}
