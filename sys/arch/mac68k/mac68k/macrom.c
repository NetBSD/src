/*	$NetBSD: macrom.c,v 1.73 2014/10/18 08:33:25 snj Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Mac ROM Glue
 *
 * This allows MacBSD to access (in a limited fashion) routines included
 * in the Mac ROMs, like ADBReInit.
 *
 * As a (fascinating) side effect, this glue allows ROM code (or any other
 * MacOS code) to call MacBSD kernel routines, like NewPtr.
 *
 * Uncleaned-up weirdness,
 *	This doesn't work on a lot of machines.  Perhaps the IIsi stuff
 * can be generalized somewhat for others.  It looks like most machines
 * are similar to the IIsi ("Universal ROMs"?).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: macrom.c,v 1.73 2014/10/18 08:33:25 snj Exp $");

#include "opt_adb.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/cpu.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/frame.h>
#include <machine/viareg.h>

#include <mac68k/mac68k/macrom.h>
#include <mac68k/dev/adbvar.h>

	/* trap modifiers (put it macrom.h) */
#define TRAP_TOOLBOX(a)	((a) & 0x800)
#define TRAP_PASSA0(a)	((a) & 0x100)
#define TRAP_NUM(a)	(TRAP_TOOLBOX(a) ? (a) & 0x3ff : (a) & 0xff)
#define TRAP_SYS(a)	((a) & 0x400)
#define TRAP_CLEAR(a)	((a) & 0x200)


	/* Mac Rom Glue global variables */
/*
 * ADB Storage.  Is 512 bytes enough?  Too much?
 */
u_char mrg_adbstore[512];
u_char mrg_adbstore2[512];
u_char mrg_adbstore3[512];
u_char mrg_ExpandMem[512];			/* 0x1ea Bytes minimum */
u_char mrg_adbstore4[32];			/* 0x16 bytes was the largest I found yet */
u_char mrg_adbstore5[80];			/* 0x46 bytes minimum */

/*
 * InitEgret in the AV ROMs requires a low memory global at 0x2010 to be
 * pointed at this jump table, which can be found at 0x40803280. It's
 * included here so we can do mrg_fixupROMBase on it.
 */

u_int32_t mrg_AVInitEgretJT[] = {
	0x408055D0, 0x4083985A, 0x40839AB6, 0x4080F180,
	0x4080C0B6, 0x4080C30A, 0x4080C380, 0x4080C482,
	0x4080C496, 0x4080C82E, 0x4080C9FE, 0x4080CA16,
	0x4081D1D6, 0x4085CDDE, 0x4080DF28, 0x4080DFC6,
	0x4080E292, 0x4080E2C0, 0x4080E348, 0x4080E600,
	0x4080E632, 0x4080E6B8, 0x4080E6E4, 0x4080E750,
	0x4080E776, 0x4080E7B4, 0x408B90E0, 0x40852490,
	0x40852280, 0x40852410, 0x4080E8F0, 0x4080E940,
	0x4080E960, 0x4080E9B0, 0x4080E9E0, 0x4080EA50,
	0x4080EA70, 0x4080EB14, 0x4080EBC0, 0x4081D1D6,
	0x40810AB0, 0x40810BDA, 0x40810CCA, 0x40810FF2,
	0x4080FF8C, 0x40810292, 0x40812CE2, 0x40813AAE,
	0x40813AE0, 0x408113DE, 0x40811EB0, 0x40811FA0,
	0x40811DD0, 0x4083B720, 0x408412E0, 0x40841300,
	0x40841380, 0x4083A390, 0x408411F0
};

void *	mrg_romadbintr = (void *)0;	/* ROM ADB interrupt */
void *	mrg_rompmintr = 0;		/* ROM PM (?) interrupt */
const char *mrg_romident = NULL;		/* ident string for ROMs */
void *	mrg_ADBAlternateInit = 0;
void *	mrg_InitEgret = 0;
void *	mrg_ADBIntrPtr = (void *)0x0;	/* ADB interrupt taken from MacOS vector table*/
void *	ROMResourceMap = 0;
extern romvec_t *mrg_MacOSROMVectors;
#if defined(MRG_TEST) || defined(MRG_DEBUG)
void *	ResHndls[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};
#else
void *	ResHndls[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};
#endif

void *dtmgr_softintr_cookie;

/*
 * Last straw functions; we didn't set them up, so freak out!
 * When someone sees these called, we can finally go back and
 * bother to implement them.
 */

int
mrg_Delay(void)
{
#define TICK_DURATION 16625
	u_int32_t ticks;

	__asm volatile ("movl	%%a0,%0"	/* get arguments */
		: "=g" (ticks)
		:
		: "a0");

#if defined(MRG_DEBUG)
	printf("mrg: mrg_Delay(%d) = %d ms\n", ticks, ticks * 60);
#endif
	delay(ticks * TICK_DURATION);

	/*
	 * The number of ticks since startup should be
	 * returned here. Until someone finds a need for
	 * this, we just return the requested number
	 * of ticks
	 */
	return (ticks);
}

/*
 * Handle the Deferred Task manager here
 *
 */
static void *	mrg_DTList = NULL;

void
mrg_DTInstall(void)
{
	void *ptr, *prev;
	void **cptr, **cprev;

	__asm volatile ("movl %%a0,%0" : "=g" (ptr));

	prev = (void *)&mrg_DTList;
	while (*(void **)prev != NULL) 
		prev = *(void **)prev;
	cptr = (void **)ptr;
	cprev = (void **)prev;
	*cptr = NULL;
	*cprev = ptr;
	softint_schedule(dtmgr_softintr_cookie);

	__asm volatile ("clrl %%d0" : : : "d0");
}

void
mrg_execute_deferred(void)
{
	void *ptr;
	int s;

	while (mrg_DTList != NULL) {
		s = splhigh();
		ptr = *(void **)mrg_DTList;
		mrg_DTList = *(void **)ptr;
		splx(s);

		__asm volatile (
		"	moveml %%a0-%%a6/%%d1-%%d7,%%sp@-	\n"
		"	movl %0,%%a0		\n"
		"	movl %%a0@(8),%%a2	\n"
		"	movl %%a0@(12),%%a1	\n"
		"	jsr %%a2@		\n"
		"	moveml %%sp@+,%%a0-%%a6/%%d1-%%d7" : : "g" (ptr));
	}
}

void
mrg_VBLQueue(void)
{
#define qLink 0
#define qType 4
#define vblAddr 6
#define vblCount 10
#define vblPhase 12
	char *vbltask;
	char *last_vbltask;
	
	last_vbltask = (char *)&VBLQueue_head;
	vbltask = VBLQueue_head;
	while (0 != vbltask) {
		if (0 != *((u_int16_t *)(vbltask + vblPhase)))
			*((u_int16_t *)(vbltask + vblPhase)) -= 1;
		else if (0 != *((u_int16_t *)(vbltask + vblCount)))
			*((u_int16_t *)(vbltask + vblCount)) -= 1;
		else {
#if defined(MRG_DEBUG)
			printf("mrg: mrg_VBLQueue: calling VBL task at 0x%x with VBLTask block at %p\n",
			    *((u_int32_t *)(vbltask + vblAddr)), vbltask);
#endif
			__asm volatile(
			"	movml	#0xfffe,%%sp@-	\n"
			"	movl	%0,%%a0		\n"
			"	movl	%1,%%a1		\n"
			"	jbsr	%%a1@		\n"
			"	movml	%%sp@+,#0x7fff"
				: : "g" (vbltask),
				    "g" (*((char *)(vbltask + vblAddr)))
				: "a0","a1");
#if defined(MRG_DEBUG)
			printf("mrg: mrg_VBLQueue: back from VBL task\n");
#endif
			if (0 == *((u_int16_t *)(vbltask + vblCount))) {
#if defined(MRG_DEBUG)
				printf("mrg: mrg_VBLQueue: removing VBLTask block at %p\n",
				    vbltask);
#endif
				*((u_int32_t *)(last_vbltask + qLink)) =
				    *((u_int32_t *)(vbltask + qLink));
				/*
				 * can't free memory from VBLTask block as
				 * we don't know where it came from
				 */
				if (vbltask == VBLQueue_tail)
					VBLQueue_tail = last_vbltask;
			}
		}
		last_vbltask = vbltask;
		vbltask = (void *) *((u_int32_t *)(vbltask + qLink));
	}
}

void
mrg_init_stub_1(void)
{
	__asm volatile ("movml #0xffff,%sp@-");
	printf("mrg: hit mrg_init_stub_1\n");
  	__asm volatile ("movml %sp@+, #0xffff");
}

void
mrg_init_stub_2(void)
{
	panic("mrg: hit mrg_init_stub_2");
}

short
Count_Resources(u_int32_t rsrc_type)
{
	rsrc_t *rsrc = (rsrc_t *)ROMResourceMap;
	short count = 0;

#if defined(MRG_DEBUG)
	printf("Count_Resources looking for 0x%08lx at 0x%08lx\n",
	    (long)rsrc_type, (long)rsrc);
#endif
	/*
	 * Return a Count of all the ROM Resouces of the requested type.
	 */
	if (ROMResourceMap == 0)
		panic("Oops! Need ROM Resource Map ListHead address!");

	while (rsrc != 0) {
#if defined(MRG_DEBUG)
		if (rsrc_type == 0)
			printf("0x%08lx: %04x %04x %04x %04x %08x %08x %08x %04x\n",
			    (long)rsrc, rsrc->unknown[0], rsrc->unknown[1],
			    rsrc->unknown[2], rsrc->unknown[3], rsrc->next,
			    rsrc->body, rsrc->name, rsrc->index);
#endif
		if (rsrc_type == 0 || (rsrc_type == rsrc->name))
			count++;
		rsrc = rsrc->next == 0 ? 0 : (rsrc_t *)(rsrc->next + (char *)ROMBase);
	}

#if defined(MRG_DEBUG)
	printf("Count_Resources found %d\n", count);
#endif
	return count;
}

void **
Get_Ind_Resource(u_int32_t rsrc_type, u_int16_t rsrc_ind)
{
	rsrc_t *rsrc = (rsrc_t *)ROMResourceMap;
	short i = 0;

	/*
	 * This routine return the "Handle" to a ROM Resource.  Since few
	 * ROM Resources are called for in NetBSD we keep a small table
	 * for the Handles we return. (Can't reuse the Handle without
	 * defeating the purpose for a Handle in the first place!)  If
	 * we get more requests than we have space for, we panic.
	 */
	if (ROMResourceMap == 0)
		panic("Oops! Need ROM Resource Map ListHead address!");

	while (rsrc != 0) {
		if (rsrc_type == rsrc->name) {
			rsrc_ind--;
			if (rsrc_ind == 0) {
				for (i = 0;
				    i < sizeof(ResHndls) / sizeof(void *); i++)
					if ((ResHndls[i] == 0) ||
					    (ResHndls[i] == (void *)(rsrc->next + (char *)ROMBase))) {
						ResHndls[i] = (void *)(rsrc->body + (char *)ROMBase);
						return (void **)&ResHndls[i];
					}
				panic("ResHndls table too small!");
			}
		}
		rsrc = rsrc->next == 0 ? 0 : (rsrc_t *)(rsrc->next + (char *)ROMBase);
	}
	return (void **)0;
}

void
mrg_FixDiv(void)
{
	panic("Oops! Need ROM address of _FixDiv for this system!");
}

void
mrg_FixMul(void)
{
	panic("Oops! Need ROM address of _FixMul for this system!");
}

void
mrg_1sec_timer_tick(void)
{	
	/* The timer tick from the Egret chip triggers this routine via
	 * Lvl1DT[0] (addr 0x192) once every second.
	 */
}
  
void
mrg_lvl1dtpanic(void)		/* Lvl1DT stopper */
{
	printf("Agh!  I was called from Lvl1DT!!!\n");
#ifdef DDB
	Debugger();
#endif
}

void
mrg_lvl2dtpanic(void)		/* Lvl2DT stopper */
{
	panic("Agh!  I was called from Lvl2DT!!!");
}

void
mrg_jadbprocpanic(void)	/* JADBProc stopper */
{
	panic("Agh!  Called JADBProc!");
}

void
mrg_jswapmmupanic(void)	/* jSwapMMU stopper */
{
	panic("Agh!  Called jSwapMMU!");
}

void
mrg_jkybdtaskpanic(void)	/* JKybdTask stopper */
{
	panic("Agh!  Called JKybdTask!");
}

#ifdef MRG_ADB
/*
 * mrg_adbintr() and mrg_pmintr() are defined in adb_direct.c if we
 * not using the MRG method of accessing the ADB/PRAM/RTC.
 */

long
mrg_adbintr(void)	/* Call ROM ADB Interrupt */
{
	if (mrg_romadbintr != NULL) {
#if defined(MRG_TRACE)
		tron();
#endif

		/* Gotta load a1 with VIA address. */
		/* ADB int expects it from Mac intr routine. */
		__asm volatile (
		"	movml	#0xffff,%%sp@-	\n"
		"	movl	%0,%%a0		\n"
		"	movl	" ___STRING(_C_LABEL(VIA)) ",%%a1 \n"
		"	jbsr	%%a0@		\n"
		"	movml	%%sp@+,#0xffff"
			:
			: "g" (mrg_romadbintr)
			: "a0","a1");

#if defined(MRG_TRACE)
		troff();
#endif

	}
	return (1);
}

long
mrg_pmintr(void)	/* Call ROM PM Interrupt */
{
	if (mrg_rompmintr != NULL) {
#if defined(MRG_TRACE)
		tron();
#endif

		/* Gotta load a1 with VIA address. */
		/* ADB int expects it from Mac intr routine. */
		__asm volatile (
		"	movml	#0xffff,%%sp@-	\n"
		"	movl	%0,%%a0		\n"
		"	movl	" ___STRING(_C_LABEL(VIA)) ",%%a1 \n"
		"	jbsr	%%a0@		\n"
		"	movml	%%sp@+,#0xffff"
			:
			: "g" (mrg_rompmintr)
			: "a0","a1");

#if defined(MRG_TRACE)
		troff();
#endif
	}
	return (1);
}
#endif	/* MRG_ADB */


void
mrg_notrap(void)
{
	printf("Aigh!\n");
	panic("mrg_notrap: We're doomed!");
}

int
myowntrap(void)
{
	printf("Oooo!  My Own Trap Routine!\n");
	return (50);
}

int
mrg_NewPtr(void)
{
	int result = noErr;
	u_int numbytes;
/*	u_int32_t trapword; */
	char *ptr;

	__asm volatile ("movl	%%d0,%0" : "=g" (numbytes) : : "d0");

#if defined(MRG_SHOWTRAPS)
	printf("mrg: NewPtr(%d bytes, ? clear, ? sys)", numbytes);
#endif

		/* plus 4 for size */
	ptr = malloc(numbytes + 4 , M_DEVBUF, M_NOWAIT); /* ?? */
		/* We ignore "Sys;" where else would it come from? */
		/* plus, (I think), malloc clears block for us */

	if (ptr == NULL) {
		result = memFullErr;
#if defined(MRG_SHOWTRAPS)
		printf(" failed.\n");
#endif
	}else{
#if defined(MRG_SHOWTRAPS)
		printf(" succeded = %p.\n", ptr);
#endif
		*(u_int32_t *)ptr = numbytes;
		ptr += 4;
		memset(ptr, 0, numbytes); /* NewPtr, 0, Clear ! */
	}

	__asm volatile("movl	%0,%%a0" :  : "g" (ptr) : "a0");
	return (result);
}

int
mrg_DisposPtr(void)
{
	int result = noErr;
	char *ptr;

	__asm volatile("movl	%%a0,%0" : "=g" (ptr) : : "a0");

#if defined(MRG_SHOWTRAPS)
	printf("mrg: DisposPtr(%p)\n", ptr);
#endif

	if (ptr == 0)
		result = memWZErr;
	else
		free(ptr - 4, M_DEVBUF);

	return (result);
}

int
mrg_GetPtrSize(void)
{
	char *ptr;

	__asm volatile("movl	%%a0,%0" : "=g" (ptr) : : "a0");

#if defined(MRG_SHOWTRAPS)
	printf("mrg: GetPtrSize(%p)\n", ptr);
#endif

	if (ptr == 0)
		return (memWZErr);
	else
		return (*(int *)(ptr - 4));
}

int
mrg_SetPtrSize(void)
{
	void *ptr;
	int newbytes;

	__asm volatile(
	"	movl	%%a0,%0	\n"
	"	movl	%%d0,%1"
		: "=g" (ptr), "=g" (newbytes) : : "d0","a0");

#if defined(MRG_SHOWTRAPS)
	printf("mrg: SetPtrSize(%p, %d) failed\n", ptr, newbytes);
#endif

	return (memFullErr);	/* How would I handle this, anyway? */
}

int
mrg_PostEvent(void)
{
	return 0;
}

void
mrg_StripAddress(void)
{
}

int
mrg_SetTrapAddress(void)
{
	extern void *mrg_OStraps[];
	void *ptr;
	int trap_num;

	__asm volatile(
	"	movl %%a0,%0	\n"
	"	movl %%d0,%1"
		: "=g" (ptr), "=g" (trap_num) : : "d0","a0");

#if defined(MRG_DEBUG)
	printf("mrg: trap 0x%x set to 0x%lx\n", trap_num, (long)ptr);
#endif
	mrg_OStraps[trap_num] = ptr;
/*
 * If the Trap for Egret was changed, we'd better remember it!
 */
	if (trap_num == 0x92) {
#if defined(MRG_DEBUG)
		printf("mrg: reconfigured Egret address from 0x%lx to 0x%lx\n",
		    (long)jEgret, (long)ptr);
#endif
		jEgret = (void (*))ptr;
	}
	return 0;
}

/*
 * trap jump address tables (different per machine?)
 * Can I just use the tables stored in the ROMs?
 * *Is* there a table stored in the ROMs?
 * We only initialize the A-Traps for the routines we have
 *  provided ourselves.  The routines we will be trying to
 *  use out of the MacROMs will be initialized at run-time.
 * I did this to make the code easier to follow and to keep
 *  from taking an unexpected side trip into the MacROMs on
 *  those systems we don't have fully decoded.
 */
void *mrg_OStraps[256] = {
#ifdef __GNUC__
		/* God, I love gcc.  see GCC2 manual, section 2.17, */
		/* "labeled elements in initializers." */
	[0x1e]	(void *)mrg_NewPtr,
		(void *)mrg_DisposPtr,
		(void *)mrg_SetPtrSize,
		(void *)mrg_GetPtrSize,
	[0x2f]	(void *)__UNCONST(mrg_PostEvent),	/* XXXGCC ? */
	[0x3b]	(void *)mrg_Delay,	
	[0x47]	(void *)mrg_SetTrapAddress,
	[0x55]	(void *)__UNCONST(mrg_StripAddress),	/* XXXGCC ? */
	[0x82]	(void *)mrg_DTInstall,
#else
#error "Using a GNU C extension."
#endif
};

void *mrg_ToolBoxtraps[1024] = {
	[0x19c] (void *)mrg_CountResources,
	[0x19d] (void *)mrg_GetIndResource,
	[0x1a0] (void *)mrg_GetResource,
	[0x1af] (void *)mrg_ResError,
};

/*
 * Handle a supervisor mode A-line trap.
 */
void
mrg_aline_super(struct frame *frame)
{
	void *trapaddr;
	u_short trapword;
	int isOStrap;
	int trapnum;
	int a0passback;
	u_int32_t a0bucket, d0bucket;
	int danprint=0; /* This shouldn't be necessary, but seems to be.  */

#if defined(MRG_DEBUG)
	printf("mrg: a super");
#endif

	trapword = *(u_short *)frame->f_pc;

	if (trapword == 0xa71e)
		danprint = 1;

#if defined(MRG_DEBUG)
	printf(" wd 0x%lx", (long)trapword);
#endif
	isOStrap = ! TRAP_TOOLBOX(trapword);
	trapnum = TRAP_NUM(trapword);

	if (danprint) {
		/*
		 * Without these print statements, ADBReInit fails on IIsi
		 * It is unclear why--perhaps a compiler bug?  delay()s do not
		 * work, nor does some assembly similar to the  printf calls.
		 * A printf(""); is sufficient, but gcc -Wall is noisy about
		 * it, so I figured backspace is harmless enough...
		 */
		printf(" "); printf("\010");
	}

#if defined(MRG_DEBUG)
	printf(" %s # 0x%x", isOStrap? "OS" :
		"ToolBox", trapnum);
#endif

	/*
	 * Only OS Traps come to us; alinetrap() takes care of ToolBox
	 * traps, which are a horrible Frankenstein-esque abomination.
	 */

	trapaddr = mrg_OStraps[trapnum];
#if defined(MRG_DEBUG)
	printf(" addr 0x%lx\n", (long)trapaddr);
 	printf("    got:    d0 = 0x%8x,  a0 = 0x%8x, called from: 0x%8x\n",
		frame->f_regs[0], frame->f_regs[8], frame->f_pc	);
#endif
	if (trapaddr == NULL) {
		printf("unknown %s trap 0x%x, no trap address available\n",
			isOStrap ? "OS" : "ToolBox", trapword);
		panic("mrg_aline_super()");
	}
	a0passback = TRAP_PASSA0(trapword);

#if defined(MRG_TRACE)
	tron();
#endif

/* 	put a0 in a0 */
/* 	put a1 in a1 */
/* 	put d0 in d0 */
/* 	put d1 in d1 */
/*	put trapaddr in a2 */
/* save a6 */
/* 	call the damn routine */
/* restore a6 */
/* 	store d0 in d0bucket */
/* 	store a0 in d0bucket */
/* This will change a2,a1,d1,d0,a0 and possibly a6 */

	__asm volatile (
	"	movl	%2@,%%d0	\n"
	"	movl	%2@(4),%%d1	\n"
	"	movl	%2@(32),%%a0	\n"
	"	movl	%2@(36),%%a1	\n"
	"	movl	%3,%%a2		\n"
	"	jbsr	%%a2@		\n"
	"	movl	%%a0,%0		\n"
	"	movl	%%d0,%1"

		: "=g" (a0bucket), "=g" (d0bucket)

		: "a" (&frame->f_regs), "g" (trapaddr)

		: "d0","d1","a0","a1","a2"
);

#if defined(MRG_TRACE)
	troff();
#endif
#if defined(MRG_DEBUG)
	printf("    result: d0 = 0x%8x,  a0 = 0x%8x\n",
		d0bucket, a0bucket );
 	printf(" bk");
#endif

	frame->f_regs[0] = d0bucket;
	if (a0passback)
		frame->f_regs[8] = a0bucket;

	frame->f_pc += 2;	/* skip offending instruction */

#if defined(MRG_DEBUG)
	printf(" exit\n");
#endif
}

extern u_int32_t traceloopstart[];
extern u_int32_t traceloopend;
extern u_int32_t *traceloopptr;

void
dumptrace(void)
{
#if defined(MRG_TRACE)
	u_int32_t *traceindex;

	printf("instruction trace:\n");
	traceindex = traceloopptr + 1;
	while (traceindex != traceloopptr) {
		printf("    %08x\n", *traceindex++);
		if (traceindex == &traceloopend)
			traceindex = &traceloopstart[0];
	}
#else
	printf("mrg: no trace functionality enabled\n");
#endif
}

	/* To find out if we're okay calling ROM vectors */
int
mrg_romready(void)
{
	return (mrg_romident != NULL);
}

extern unsigned long	IOBase;
extern volatile u_char	*sccA;

	/* initialize Mac ROM Glue */
void
mrg_init(void)
{
	const char *findername = "MacBSD FakeFinder";
	int i;
#if defined(MRG_TEST)
	void *ptr;
	short rcnt;
	int sizeptr;
	extern short mrg_ResErr;
	void **handle;
#endif
	
	/*
	 * Clear the VBLQueue.
	 */
	VBLQueue = (u_int16_t) 0;
	VBLQueue_head = (void *) 0;
	VBLQueue_tail = (void *) 0;
					 
#if defined(MRG_TEST)
	if (ROMResourceMap) {
		printf("mrg: testing CountResources\n");
		__asm volatile (
		"	clrl    %%sp@-	\n"
		"	clrl    %%sp@-	\n"
		"	.word   0xa99c	\n"
		"	movw    %%sp@+,%0"
			: "=g" (rcnt));
		printf("mrg: found %d resources in ROM\n", rcnt);
		__asm volatile (
		"	clrl    %%sp@-	\n"
		"	movl    #0x44525652,%%sp@-	\n"
		"	.word   0xa99c	\n"
		"	movw    %%sp@+,%0"
			: "=g" (rcnt));
		printf("mrg: %d are DRVR resources\n", rcnt);
		if (rcnt == 0)
			panic("Oops! No DRVR Resources found in ROM");
	}
#endif
#if defined(MRG_TEST)
	if (ROMResourceMap) {
		printf("mrg: testing GetIndResource\n");
		__asm volatile (
		"	clrl    %%sp@-		\n"
		"	movl    #0x44525652,%%sp@-	\n"
		"	movw    #0x01,%%sp@-	\n"
		"	.word   0xa99d		\n"
		"	movl    %%sp@+,%0"
			: "=g" (handle));
		printf("Handle to first DRVR resource is %p\n", handle);
		printf("DRVR: 0x%08lx -> 0x%08lx -> 0x%08lx\n",
		    (long)Get_Ind_Resource(0x44525652, 1),
		    (long)*Get_Ind_Resource(0x44525652, 1),
		    (long)*((u_int32_t *)*Get_Ind_Resource(0x44525652, 1)));
		__asm volatile (
		"	clrl    %%sp@-		\n"
		"	movl    #0x44525652,%%sp@-	\n"
		"	movw    #0x02,%%sp@-	\n"
		"	.word   0xa99d		\n"
		"	movl    %%sp@+,%0"
			: "=g" (handle));
		printf("Handle to second DRVR resource is %p\n", handle);
		printf("DRVR: 0x%08lx -> 0x%08lx -> 0x%08lx\n",
		    (long)Get_Ind_Resource(0x44525652, 2),
		    (long)*Get_Ind_Resource(0x44525652, 2),
		    (long)*((u_int32_t *)*Get_Ind_Resource(0x44525652, 2)));
	}
#endif
	if (mrg_romready()) {
		printf("mrg: '%s' ROM glue", mrg_romident);

#if defined(MRG_TRACE)
#if defined(MRG_FOLLOW)
		printf(", tracing on (verbose)");
#else /* ! defined (MRG_FOLLOW) */
		printf(", tracing on (silent)");
#endif /* defined(MRG_FOLLOW) */
#else /* !defined(MRG_TRACE) */
		printf(", tracing off");
#endif	/* defined(MRG_TRACE) */

#if defined(MRG_DEBUG)
		printf(", debug on");
#else /* !defined(MRG_DEBUG) */
		printf(", debug off");
#endif /* defined(MRG_DEBUG) */

#if defined(MRG_SHOWTRAPS)
		printf(", verbose traps");
#else /* !defined(MRG_SHOWTRAPS) */
		printf(", silent traps");
#endif /* defined(MRG_SHOWTRAPS) */
	}else{
		printf("mrg: kernel has no ROM vectors for this machine!\n");
		return;
	}

	printf("\n");

#if defined(MRG_DEBUG)
	printf("mrg: start init\n");
#endif
		/* expected globals */
	ExpandMem = &mrg_ExpandMem[0];
	*((u_int16_t *)(mrg_ExpandMem + 0x00) ) = 0x0123;	/* magic (word) */
	*((u_int32_t *)(mrg_ExpandMem + 0x02) ) = 0x000001ea;	/* Length of table (long) */
	*((u_int32_t *)(mrg_ExpandMem + 0x1e0)) = (u_int32_t) &mrg_adbstore4[0];

	*((u_int32_t *)(mrg_adbstore4 + 0x8)) = (u_int32_t) mrg_init_stub_1;
	*((u_int32_t *)(mrg_adbstore4 + 0xc)) = (u_int32_t) mrg_init_stub_2;
	*((u_int32_t *)(mrg_adbstore4 + 0x4)) = (u_int32_t) &mrg_adbstore5[0];

	*((u_int32_t *)(mrg_adbstore5 + 0x08)) = (u_int32_t) 0x00100000;
	*((u_int32_t *)(mrg_adbstore5 + 0x0c)) = (u_int32_t) 0x00100000;
	*((u_int32_t *)(mrg_adbstore5 + 0x16)) = (u_int32_t) 0x00480000;

	ADBBase = &mrg_adbstore[0];
	ADBState = &mrg_adbstore2[0];
	ADBYMM = &mrg_adbstore3[0];
	MinusOne = 0xffffffff;
	Lo3Bytes = 0x00ffffff;
	VIA = (void *)__UNVOLATILE(Via1Base);
	MMU32Bit = 1; /* ?means MMU is in 32 bit mode? */
  	if (TimeDBRA == 0)
		TimeDBRA = 0xa3b;		/* BARF default is Mac II */
  	if (ROMBase == 0)
		panic("ROMBase not set in mrg_init()!");

	strcpy(&FinderName[1], findername);
	FinderName[0] = (u_char) strlen(findername);
#if defined(MRG_DEBUG)
	printf("After setting globals\n");
#endif

		/* Fake jump points */
	for (i = 0; i < 8; i++) /* Set up fake Lvl1DT */
		Lvl1DT[i] = mrg_lvl1dtpanic;
	for (i = 0; i < 8; i++) /* Set up fake Lvl2DT */
		Lvl2DT[i] = mrg_lvl2dtpanic;
	Lvl1DT[0] = (void (*)(void))mrg_1sec_timer_tick;
	Lvl1DT[2] = (void (*)(void))mrg_romadbintr;
	Lvl1DT[4] = (void (*)(void))mrg_rompmintr;
	JADBProc = mrg_jadbprocpanic;	/* Fake JADBProc for the time being */
	jSwapMMU = mrg_jswapmmupanic;	/* Fake jSwapMMU for the time being */
	JKybdTask = mrg_jkybdtaskpanic;	/* Fake JKybdTask for the time being */

	/* probably very dangerous */
	jADBOp = (void (*)(void))mrg_OStraps[0x7c];

	mrg_VIA2 = (void *)((char *)__UNVOLATILE(Via1Base) + 
	    VIA2 * 0x2000);	/* see via.h */
	SCCRd = (void *)__UNVOLATILE(sccA);/* ser.c ; we run before serinit */

	jDTInstall = (void *)mrg_DTInstall;
	dtmgr_softintr_cookie = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *))mrg_execute_deferred, NULL);

	/* AV ROMs want this low memory vector to point to a jump table */
	InitEgretJTVec = (u_int32_t **)(void *)&mrg_AVInitEgretJT;

	switch (mach_cputype()) {
		case MACH_68020:	CPUFlag = 2;	break;
		case MACH_68030:	CPUFlag = 3;	break;
		case MACH_68040:	CPUFlag = 4;	break;
		default:
			printf("mrg: unknown CPU type; cannot set CPUFlag\n");
			break;
	}

#if defined(MRG_TEST)
	printf("Allocating a pointer...\n");
	ptr = (void *)NewPtr(1024);
	printf("Result is 0x%lx.\n", (long)ptr);
	sizeptr = GetPtrSize((Ptr)ptr);
	printf("Pointer size is %d\n", sizeptr);
	printf("Freeing the pointer...\n");
	DisposPtr((Ptr)ptr);
	printf("Free'd.\n");

	for (i = 0; i < 500000; i++)
		if ((i % 100000) == 0)printf(".");
	printf("\n");

	mrg_ResErr = 0xdead;	/* set an error we know */
	printf("Getting error code...\n");
	i = ResError();
	printf("Result code (0xdeadbaaf): %x\n", i);
	printf("Getting an ADBS Resource...\n");
	handle = GetResource(0x41244253, 2);
	printf("Handle result from GetResource: 0x%lx\n", (long)handle);
	printf("Getting error code...\n");
	i = ResError();
	printf("Result code (-192?) : %d\n", i);

	for (i = 0; i < 500000; i++)
		if ((i % 100000) == 0)printf(".");
	printf("\n");

#if defined(MRG_TRACE)
	printf("Turning on a trace\n");
	tron();
	printf("We are now tracing\n");
	troff();
	printf("Turning off trace\n");
	dumptrace();
#endif /* MRG_TRACE */

	for (i = 0; i < 500000; i++)
		if ((i % 100000) == 0)
			printf(".");
	printf("\n");
#endif /* MRG_TEST */

#if defined(MRG_DEBUG)
	printf("after setting jump points\n");
	printf("mrg: end init\n");
#endif

	if (1) {
		/*
		 * For the bloody Mac II ROMs, we have to map this space
		 * so that the PRam functions will work.
		 * Gee, Apple, is that a hard-coded hardware address in
		 * your code?  I think so! (_ReadXPRam + 0x0062 on the
		 * II)  We map the VIAs in here.  The C610 apparently
		 * needs it, too, which means that a bunch of 040s do, too.
		 * Once again, I regret the mapping changes I made...  -akb
		 */
#ifdef DIAGNOSTIC
		printf("mrg: I/O map kludge for ROMs that use hardware %s",
			"addresses directly.\n");
#endif
		pmap_map(0x50f00000, 0x50f00000, 0x50f00000 + 0x4000,
			 VM_PROT_READ|VM_PROT_WRITE);
	}
}

#ifdef MRG_ADB
static void	setup_egret(void);

static void
setup_egret(void)
{
	if (0 != mrg_InitEgret) {

	/* This initializes ADBState (mrg_ADBStore2) and
	   enables interrupts */
		__asm volatile (
		"	movml	%%a0-%%a2,%%sp@-	\n"
		"	movl	%1,%%a0	\n"	/* ADBState, mrg_adbstore2 */
		"	movl	%0,%%a1	\n"
		"	jbsr	%%a1@	\n"
		"	movml	%%sp@+,%%a0-%%a2 "
			:
			: "g" (mrg_InitEgret), "g" (ADBState)
			: "a0","a1");
		jEgret = (void (*)) mrg_OStraps[0x92]; /* may have been set in asm() */
	}
	else printf("Help ...  No vector for InitEgret!!\n");
	
#if defined(MRG_DEBUG)
	printf("mrg: ADBIntrVector: 0x%8lx,  mrg_ADBIntrVector: 0x%8lx\n",
			(long) mrg_romadbintr,
			*((long *) 0x19a));
	printf("mrg: EgretOSTrap: 0x%8lx\n",
			(long) mrg_OStraps[0x92]);
#endif
}
#endif

void
mrg_initadbintr(void)
{
	if (mac68k_machine.do_graybars)
		printf("Got following HwCfgFlags: 0x%4x, 0x%8x, 0x%8x, 0x%8x\n",
				HwCfgFlags, HwCfgFlags2, HwCfgFlags3, ADBReInit_JTBL);

	if ((HwCfgFlags == 0) && (HwCfgFlags2 == 0) && (HwCfgFlags3 == 0)) {
		printf("Caution: No HwCfgFlags from Booter, please "
			"use at least booter version 1.8.\n");

		if (current_mac_model->class == MACH_CLASSIIsi) {
			printf("     ...  Using defaults for IIsi.\n");

			/* Egret and ADBReInit look into these HwCfgFlags */
			HwCfgFlags = 0xfc00;	
			HwCfgFlags2 = 0x0000773F;
			HwCfgFlags3 = 0x000001a6;
		}

		printf("Using HwCfgFlags: 0x%4x, 0x%8x, 0x%8x\n",
			HwCfgFlags, HwCfgFlags2, HwCfgFlags3);
	}

#ifdef MRG_ADB	
	/*
	 * If we think there is an Egret in the machine, attempt to
	 * set it up.  If not, just enable the interrupts (only on
	 * some machines, others are already on from ADBReInit?).
	 */
	if (   ((HwCfgFlags3 & 0x0e) == 0x06 )
	    || ((HwCfgFlags3 & 0x70) == 0x20 )) {
		if (mac68k_machine.do_graybars)
			printf("mrg: setup_egret:\n");

		setup_egret();

		if (mac68k_machine.do_graybars)
			printf("mrg: setup_egret: done.\n");

	} else {

		if (mac68k_machine.do_graybars)
			printf("mrg: Not setting up egret.\n");

		via_reg(VIA1, vIFR) = 0x4;
		via_reg(VIA1, vIER) = 0x84;

		if (mac68k_machine.do_graybars)
			printf("mrg: ADB interrupts enabled.\n");
	}	
#else
	/* Extra Egret setup required only for MRG ADB functions. */
	printf("mrg: skipping egret setup\n");
#endif /* MRG_ADB */
}

/*
 * NOTE:  By eliminating the setvectors routine and moving its function
 *        to here we only have to deal with re-locating MacOS Addresses
 *        once and all in one place.
 */
void
mrg_fixupROMBase(void *obase, void *nbase)
{
	u_int32_t oldbase, newbase;
	romvec_t *rom;
	int i;

	oldbase = (u_int32_t) obase;
	newbase = (u_int32_t) nbase;

	/*
	 * Grab the pointer to the Mac ROM Glue Vector table
	 */ 
	rom = mrg_MacOSROMVectors;

	if (rom == NULL)
		return;		/* whoops!  ROM vectors not defined! */

	mrg_romident = rom->romident;

	if (0 != mrg_ADBIntrPtr) {
		mrg_romadbintr = mrg_ADBIntrPtr;
		printf("mrg_fixup: using ADBIntrPtr from booter: 0x%08lx\n",
		    (long)mrg_ADBIntrPtr);
	} else
		mrg_romadbintr = rom->adbintr == 0 ?
		    0 : (char *)rom->adbintr - oldbase + newbase;

	mrg_rompmintr = rom->pmintr == 0 ?
	    0 : (char *)rom->pmintr - oldbase + newbase;
	mrg_ADBAlternateInit = rom->ADBAlternateInit == 0 ?
	    0 : (char *)rom->ADBAlternateInit - oldbase + newbase;

	/*
	 * mrg_adbstore becomes ADBBase
	 */
	*((u_int32_t *)(mrg_adbstore + 0x130)) = rom->adb130intr == 0 ?
	    0 : (u_int32_t) rom->adb130intr - oldbase + newbase;

	mrg_OStraps[0x77] = rom->CountADBs == 0 ?
	    0 : (char *)rom->CountADBs - oldbase + newbase;
	mrg_OStraps[0x78] = rom->GetIndADB == 0 ?
	    0 : (char *)rom->GetIndADB - oldbase + newbase;
	mrg_OStraps[0x79] = rom-> GetADBInfo == 0 ?
	    0 : (char *)rom->GetADBInfo - oldbase + newbase;
	mrg_OStraps[0x7a] = rom->SetADBInfo == 0 ?
	    0 : (char *)rom->SetADBInfo - oldbase + newbase;
	mrg_OStraps[0x7b] = rom->ADBReInit == 0 ?
	    0 : (char *)rom->ADBReInit - oldbase + newbase;
	mrg_OStraps[0x7c] = rom->ADBOp == 0 ?
	    0 : (char *)rom->ADBOp - oldbase + newbase;
	mrg_OStraps[0x85] = rom->PMgrOp == 0 ?
	    0 : (char *)rom->PMgrOp - oldbase + newbase;
	mrg_OStraps[0x51] = rom->ReadXPRam == 0 ?
	    0 : (char *)rom->ReadXPRam - oldbase + newbase;
	mrg_OStraps[0x38] = rom->WriteParam == 0 ?
	    0 : (char *)rom->WriteParam - oldbase + newbase;/* WriteParam*/
	mrg_OStraps[0x3a] = rom->SetDateTime == 0 ?
	    0 : (char *)rom->SetDateTime - oldbase + newbase;/*SetDateTime*/
	mrg_OStraps[0x3f] = rom->InitUtil == 0 ?
	    0 : (char *)rom->InitUtil - oldbase + newbase;  /* InitUtil */
	mrg_OStraps[0x51] = rom->ReadXPRam == 0 ?
	    0 : (char *)rom->ReadXPRam - oldbase + newbase; /* ReadXPRam */
	mrg_OStraps[0x52] = rom->WriteXPRam == 0 ?
	    0 : (char *)rom->WriteXPRam - oldbase + newbase;/* WriteXPRam */

	if (rom->Egret == 0) {
		jEgret = 0;
		mrg_OStraps[0x92] = 0;
	} else {
		jEgret = (void (*))((char *)rom->Egret - oldbase + newbase);
		mrg_OStraps[0x92] = (char *)rom->Egret - oldbase + newbase;
	}
	mrg_InitEgret = rom->InitEgret == 0 ?
	    0 : (char *)rom->InitEgret - oldbase + newbase;

	if (rom->jClkNoMem == 0) {
		printf("WARNING: don't have a value for jClkNoMem, ");
		printf("please contact:  walter@ghpc8.ihf.rwth-aachen.de\n");
		printf("Can't read RTC without it. Using MacOS boot time.\n");
		jClkNoMem = 0;
	} else
		jClkNoMem = (void (*))((char *)rom->jClkNoMem - oldbase + newbase);
	/*
	 * Get the ToolBox Routines we may need.  These are
	 * used in the ADB Initialization of some systems.
	 * If we don't have the ROM addresses for these routines
	 * we'll setup to catch the calls in our own dummy
	 * routines. That way we can politely tell the user
	 * what we'll need to complete initialization on the system.
	 */
	mrg_ToolBoxtraps[0x04d] = rom->FixDiv == 0 ?
	    (void *)mrg_FixDiv : (char *)rom->FixDiv - oldbase + newbase;
	mrg_ToolBoxtraps[0x068] = rom->FixMul == 0 ?
	    (void *)mrg_FixMul : (char *)rom->FixMul - oldbase + newbase;

	/*
	 * Some systems also require this to be setup for use in
	 * ADB Initialization.  Use whatever address was provided
	 * to us in the romvec table for this system. This may
	 * cause a problem on some systems, and may need a better
	 * Trap handler in the future.
	 */
	ADBReInit_JTBL = rom->ADBReInit_JTBL == 0 ?
	   0 : (u_int32_t)rom->ADBReInit_JTBL - oldbase + newbase;

	/*
	 * Setup to trap unexpected access to ADBProc which is used in
	 * ADB Initialization on some systems. If the correct entry
	 * point in the ADBInit code is selected, this address is
	 * re-configured by the ROM during initialization. This feature
	 * is not currently used by NetBSD.
	 */
	JADBProc = mrg_jadbprocpanic;

	/*
	 * Get the address of the first (top) Resource in the ROM.
	 * This will be the head of a linked list of all Resources
	 * in the ROM which will be mapped in mrg_InitResources.
	 */
	ROMResourceMap = rom->ROMResourceMap == 0 ?
	    0 : (void (*))((char *)rom->ROMResourceMap - oldbase + newbase);

	for (i = 0; i < sizeof(mrg_AVInitEgretJT) / sizeof(mrg_AVInitEgretJT[0]); i++)
		mrg_AVInitEgretJT[i] = mrg_AVInitEgretJT[i] == 0 ?
		    0 : mrg_AVInitEgretJT[i] - oldbase + newbase;

#if defined(MRG_DEBUG)
	printf("mrg: ROM adbintr 0x%08lx -> 0x%08lx\n",
	    (long)rom->adbintr, (long)mrg_romadbintr);
	printf("mrg: ROM pmintr 0x%08lx -> 0x%08lx\n",
	    (long)rom->pmintr, (long)mrg_rompmintr);
	printf("mrg: OS trap 0x77 (CountADBs) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->CountADBs, (long)mrg_OStraps[0x77]);
	printf("mrg: OS trap 0x78 (GetIndADB) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->GetIndADB, (long)mrg_OStraps[0x78]);
	printf("mrg: OS trap 0x79 (GetADBInfo) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->GetADBInfo, (long)mrg_OStraps[0x79]);
	printf("mrg: OS trap 0x7a (SetADBInfo) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->SetADBInfo, (long)mrg_OStraps[0x7a]);
	printf("mrg: OS trap 0x7b (ADBReInit) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->ADBReInit, (long)mrg_OStraps[0x7b]);
	printf("mrg: OS trap 0x7c (ADBOp) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->ADBOp, (long)mrg_OStraps[0x7c]);
	printf("mrg: OS trap 0x85 (PMgrOp) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->PMgrOp, (long)mrg_OStraps[0x85]);
	printf("mrg: OS trap 0x92 (Egret) = 0x%08lx -> 0x%08lx\n",
	    (long)rom->Egret, (long)mrg_OStraps[0x92]);
	printf("mrg: ROM ADBAltInit 0x%08lx -> 0x%08lx\n",
	    (long)rom->ADBAlternateInit, (long)mrg_ADBAlternateInit);
	printf("mrg: ROM ADBReInit_JTBL 0x%08lx -> 0x%08lx\n",
	    (long)rom->ADBReInit_JTBL, (long)ADBReInit_JTBL);
	printf("mrg: ROM InitEgret  0x%08lx -> 0x%08lx\n",
	    (long)rom->InitEgret, (long)mrg_InitEgret);
	printf("mrg: ROM Resource list-head 0x%08lx -> 0x%08lx\n",
	    (long)rom->ROMResourceMap, (long)ROMResourceMap);
#endif
}   

#ifdef MRG_ADB
void
ADBAlternateInit(void)
{
	if (0 == mrg_ADBAlternateInit) {
		ADBReInit();
	} else {
 		__asm volatile (
		"	movml	%%a0-%%a6/%%d0-%%d7,%%sp@-	\n"
		"	movl	%0,%%a1		\n"
		"	movl	%1,%%a3		\n"
		"	jbsr	%%a1@		\n"
		"	movml	%%sp@+,%%a0-%%a6/%%d0-%%d7"
			: 
			: "g" (mrg_ADBAlternateInit), "g" (ADBBase)
			: "a1","a3");
	}
}
#endif /* MRG_ADB */
