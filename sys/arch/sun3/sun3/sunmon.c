/*	$NetBSD: sunmon.c,v 1.18 2006/10/01 03:53:27 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunmon.c,v 1.18 2006/10/01 03:53:27 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>

#include <machine/mon.h>
#include <machine/mc68851.h>

#include <sun3/sun3/machdep.h>
#include <sun3/sun3/interreg.h>
#include <sun68k/sun68k/vector.h>

static void **sunmon_vbr;
static void *sunmon_vcmd;	/* XXX: always 0? */

static void tracedump(int);
static void v_handler(int, char *);

/*
 * Prepare for running the PROM monitor
 */
static void
_mode_monitor(void)
{
	/* Disable our level-5 clock. */
	set_clk_mode(0, IREG_CLOCK_ENAB_5, 0);
	/* Restore the PROM vector table */
	setvbr(sunmon_vbr);
	/* Enable the PROM NMI clock. */
	set_clk_mode(IREG_CLOCK_ENAB_7, 0, 1);
	/* XXX - Disable watchdog action? */
}

/*
 * Prepare for running the kernel
 */
static void
_mode_kernel(void)
{
	/* Disable the PROM NMI clock. */
	set_clk_mode(0, IREG_CLOCK_ENAB_7, 0);
	/* Restore our own vector table */
	setvbr(vector_table);
	/* Enable our level-5 clock. */
	set_clk_mode(IREG_CLOCK_ENAB_5, 0, 1);
}

/*
 * This function takes care of restoring enough of the
 * hardware state to allow the PROM to run normally.
 * The PROM needs: NMI enabled, it's own vector table.
 * In case of a temporary "drop into PROM", this will
 * also put our hardware state back into place after
 * the PROM "c" (continue) command is given.
 */
void 
sunmon_abort(void)
{
	int s = splhigh();
#ifdef	_SUN3X_
	struct mmu_rootptr crp;
#endif

	_mode_monitor();
	delay(100000);

#ifdef	_SUN3X_
	getcrp(&crp);
	loadcrp(&mon_crp);
#endif

	/*
	 * Drop into the PROM in a way that allows a continue.
	 * Already setup "trap #14" in sunmon_init().
	 */

	__asm(" trap #14 ; _sunmon_continued: nop");

	/* We have continued from a PROM abort! */
#ifdef	_SUN3X_
	loadcrp(&crp);
#endif
	_mode_kernel();
	splx(s);
}

void 
sunmon_halt(void)
{
	(void) splhigh();
	_mode_monitor();
	*romVectorPtr->vector_cmd = sunmon_vcmd;
#ifdef	_SUN3X_
	loadcrp(&mon_crp);
	/*
	 * The PROM monitor "exit_to_mon" function appears to have problems...
	 * SunOS uses the "abort" function when you halt (bug work-around?)
	 * so we might as well do the same.
	 */
	__asm(" trap #14"); /* mon_exit_to_mon() provokes PROM monitor bug */
#endif
	mon_exit_to_mon();
	/*NOTREACHED*/
}

/*
 * Caller must pass a string that is in our data segment.
 */
void 
sunmon_reboot(const char *bs)
{

	(void) splhigh();
	_mode_monitor();
	*romVectorPtr->vector_cmd = sunmon_vcmd;
#ifdef	_SUN3X_
	loadcrp(&mon_crp);
#endif
	mon_reboot(bs);
	mon_exit_to_mon();
	/*NOTREACHED*/
}


/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing "g4" (for sun-2
 * compatibility) or "w trace".  This causes the monitor to call
 * the v_handler() routine which will call tracedump() for these cases.
 */
struct funcall_frame {
	struct funcall_frame *fr_savfp;
	int fr_savpc;
	int fr_arg[1];
};
/*VARARGS0*/
static void 
tracedump(int x1)
{
	struct funcall_frame *fp = (struct funcall_frame *)(&x1 - 2);
	u_int stackpage = ((u_int)fp) & ~PGOFSET;

	mon_printf("Begin traceback...fp = 0x%x\n", fp);
	do {
		if (fp == fp->fr_savfp) {
			mon_printf("FP loop at 0x%x", fp);
			break;
		}
		mon_printf("Called from 0x%x, fp=0x%x, args=0x%x 0x%x 0x%x 0x%x\n",
				   fp->fr_savpc, fp->fr_savfp,
				   fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	} while ( (((u_int)fp) & ~PGOFSET) == stackpage);
	mon_printf("End traceback...\n");
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.
 * [lifted from freed cmu mach3 sun3 port]
 */
static void 
v_handler(int addr, char *str)
{

	switch (*str) {
	case '\0':
		/*
		 * No (non-hex) letter was specified on
		 * command line, use only the number given
		 */
		switch (addr) {
		case 0:			/* old g0 */
		case 0xd:		/* 'd'ump short hand */
			_mode_kernel();
			panic("zero");
			/*NOTREACHED*/

		case 4:			/* old g4 */
			goto do_trace;

		default:
			goto err;
		}
		break;

	case 'p':			/* 'p'rint string command */
	case 'P':
		mon_printf("%s\n", (char *)addr);
		break;

	case '%':			/* p'%'int anything a la printf */
		mon_printf(str, addr);
		mon_printf("\n");
		break;

	do_trace:
	case 't':			/* 't'race kernel stack */
	case 'T':
		tracedump(addr);
		break;

	case 'u':			/* d'u'mp hack ('d' look like hex) */
	case 'U':
		goto err;
		break;

	default:
	err:
		mon_printf("Don't understand 0x%x '%s'\n", addr, str);
	}
}

/*
 * Set the PROM vector handler (for g0, g4, etc.)
 * and set boothowto from the PROM arg strings.
 *
 * Note, args are always:
 * argv[0] = boot_device	(i.e. "sd(0,0,0)")
 * argv[1] = options	(i.e. "-ds" or NULL)
 * argv[2] = NULL
 */
void 
sunmon_init(void)
{
	struct sunromvec *rvec;
	struct bootparam *bp;
	char **argp;
	char *p;

	rvec = romVectorPtr;
	bp = *rvec->bootParam;

	/* Save the PROM monitor Vector Base Register (VBR). */
	sunmon_vbr = getvbr();

	/* Arrange for "trap #14" to cause a PROM abort. */
	sunmon_vbr[32+14] = romVectorPtr->abortEntry;

	/* Save and replace the "v command" handler. */
	sunmon_vcmd = *rvec->vector_cmd;
	if (rvec->romvecVersion >= 2)
		*rvec->vector_cmd = v_handler;

	/* Set boothowto flags from PROM args. */
	argp = bp->argPtr;

	/* Skip argp[0] (the device string) */
	argp++;

	/* Have options? */
	if (*argp == NULL)
		return;
	p = *argp;
	if (*p == '-') {
		/* yes, parse options */
#ifdef	DEBUG
		mon_printf("boot option: %s\n", p);
#endif
		for (++p; *p; p++)
			BOOT_FLAG(*p, boothowto);
		argp++;
	}

#ifdef	DEBUG
	/* Have init name? */
	if (*argp == NULL)
		return;
	p = *argp;
	mon_printf("boot initpath: %s\n", p);
#endif
}
