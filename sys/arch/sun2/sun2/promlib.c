/*	$NetBSD: promlib.c,v 1.4 2001/06/14 13:21:39 fredette Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Matthew Fredette.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>

#include <machine/stdarg.h>
#define _SUN2_PROMLIB_PRIVATE
#include <machine/promlib.h>

#include <sun2/sun2/machdep.h>
#include <sun2/sun2/control.h>
#include <sun68k/sun68k/vector.h>
#include <machine/pte.h>

/*
 * The state we save when we get ready to disappear into the PROM.
 */
struct kernel_state {
	int saved_spl;
	int saved_ctx;
	u_int saved_ptes[4];
};

static void **sunmon_vbr;
static struct kernel_state sunmon_kernel_state;
static struct bootparam sunmon_bootparam;
static u_int sunmon_ptes[4];

static void tracedump __P((int));

/*
 * The PROM keeps its data is in the first four physical pages, and
 * assumes that they're mapped to the first four virtual pages.
 * Normally we keep the first four virtual pages unmapped, so before
 * we can dereference pointers in romVectorPtr or call the PROM, we 
 * have to restore its mappings.  
 */

/*
 * This swaps out one set of PTEs for the first 
 * four virtual pages, and swaps another set in.
 */
static inline void _prom_swap_ptes __P((u_int *, u_int *));
static inline void
_prom_swap_ptes(swapout, swapin)
	u_int *swapout, *swapin;
{
	int pte_number;
	vm_offset_t va;

	for(pte_number = 0, va = 0; pte_number < 4; pte_number++, va += NBPG) {
		swapout[pte_number] = get_pte(va);
		set_pte(va, swapin[pte_number]);
	}
}

/*
 * Prepare for running the PROM monitor.
 */
static inline void _mode_monitor __P((struct kernel_state *, int));
static inline void
_mode_monitor(state, full)
	struct kernel_state *state;
	int full;
{
	/*
	 * Save the current context, and the PTEs for pages
	 * zero through three, and reset them to what the PROM 
	 * expects.
	 */
	state->saved_ctx = get_context();
	set_context(0);
	_prom_swap_ptes(state->saved_ptes, sunmon_ptes);

	/*
	 * If we're going to enter the PROM fully, raise the interrupt
	 * level, disable our level 5 clock, restore the PROM vector
	 * table, and enable the PROM NMI clock.
	 */
	if (full) {
		state->saved_spl = splhigh();
		set_clk_mode(0, 0);
		setvbr(sunmon_vbr);
		set_clk_mode(1, 1);
	}
}

/*
 * Prepare for running the kernel.
 */
static inline void _mode_kernel __P((struct kernel_state *, int));
static inline void
_mode_kernel(state, full)
	struct kernel_state *state;
	int full;
{
	/*
	 * If we were in the PROM fully, disable the PROM NMI clock,
	 * restore our own vector table, and enable our level 5 clock.
	 */
	if (full) {
		set_clk_mode(1, 0);
		setvbr(vector_table);
		set_clk_mode(0, 1);
		splx(state->saved_spl);
	}

	/*
	 * Restore our PTEs for pages zero through three, 
	 * and restore the current context.
	 */
	_prom_swap_ptes(sunmon_ptes, state->saved_ptes);
	set_context(state->saved_ctx);
}

/* We define many prom_ functions using this macro. */
#define PROMLIB_FUNC(type, new, proto, old, args, ret)			\
type new proto								\
{									\
	struct kernel_state state;					\
	int rc;								\
	_mode_monitor(&state, 0);					\
	rc = (*(romVectorPtr->old)) args;				\
	_mode_kernel(&state, 0);					\
	ret ;								\
}
PROMLIB_FUNC(int, prom_memsize, (void), memorySize, + 0, return(rc))
PROMLIB_FUNC(int, prom_stdin, (void), inSource, + 0, return(rc))
PROMLIB_FUNC(int, prom_stdout, (void), outSink, + 0, return(rc))
PROMLIB_FUNC(int, prom_getchar, (void), getChar, (), return(rc))
PROMLIB_FUNC(int, prom_peekchar, (void), mayGet, (), return(rc))
PROMLIB_FUNC(void, prom_putchar, (int c), fbWriteChar, (c), return)
PROMLIB_FUNC(void, prom_putstr, (char *buf, int len), fbWriteStr, (buf, len), return)

/*
 * printf is difficult, because it's a varargs function.
 * This is very ugly.  Please fix me!
 */
void
#ifdef __STDC__
prom_printf(const char *fmt, ...)
#else
prom_printf(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	struct kernel_state state;
	int rc;
	va_list ap;
	const char *p1;
	char c1;
	struct printf_args {
		int arg[15];
	} varargs;
	int i;

	/*
	 * Since the PROM obviously doesn't take a va_list, we conjure
	 * up a structure of ints to hold the arguments, and pass it
	 * the structure (*not* a pointer to the structure!) to get
	 * the same effect.  This means there is a limit on the number
	 * of arguments you can use with prom_printf.  Ugly indeed.
	 */
	va_start(ap, fmt);
	i = 0;
	for(p1 = fmt; (c1 = *(p1++)) != '\0'; ) {
		if (c1 == '%') {
			if (i == (sizeof(varargs.arg) / sizeof(varargs.arg[0]))) {
				prom_printf("too many args to prom_printf, format %s", fmt);
				prom_abort();
			}
			varargs.arg[i++] = va_arg(ap, int);
		}
	}
	va_end(ap);

	/* now call the monitor's printf: */
	_mode_monitor(&state, 0);
	rc = (*
	    /* the ghastly type we cast the PROM printf vector to: */
	    ( (int (*) __P((const char *, struct printf_args)))
	    /* the PROM printf vector: */
		(romVectorPtr->printf))
		)(fmt, varargs);
	_mode_kernel(&state, 0);
}

/* Return the boot path. */
char *
prom_getbootpath()
{
	/*
	 * The first bootparam argument is the device string.
	 */
	return (sunmon_bootparam.argPtr[0]);
}

/* Return the boot args. */
char *
prom_getbootargs()
{
	/*
	 * The second bootparam argument is any options.
	 */
	return (sunmon_bootparam.argPtr[1]);
}

/* Return the boot file. */
char *
prom_getbootfile()
{
	return (sunmon_bootparam.fileName);
}

/* This maps a PROM `sd' unit number into a SCSI target. */
int
prom_sd_target(unit)
	int unit;
{
	switch(unit) {
	case 2:	return (4);
	}
	return (unit);
}

/*
 * This aborts to the PROM, but should allow the user
 * to "c" continue back into the kernel.
 */
void
prom_abort()
{
	u_int16_t old_g0_g4_vectors[4], *vec, *store;

	_mode_monitor(&sunmon_kernel_state, 1);

	/*
	 * Set up our g0 and g4 handlers, by writing into
	 * the PROM's vector table directly.  Note that
	 * the braw instruction displacement is PC-relative.
	 */
#define	BRAW	0x6000
	vec = (u_int16_t *) sunmon_vbr;
	store = old_g0_g4_vectors;
	*(store++) = *vec;
	*(vec++) = BRAW;
	*(store++) = *vec;
	*(vec++) = ((u_long) g0_entry) - ((u_long) vec);
	*(store++) = *vec;
	*(vec++) = BRAW;
	*(store++) = *vec;
	*(vec++) = ((u_long) g4_entry) - ((u_long) vec);
#undef	BRAW

	delay(100000);

	/*
	 * Drop into the PROM in a way that allows a continue.
	 * Already setup "trap #14" in prom_init().
	 */

	asm(" trap #14 ; _sunmon_continued: nop");

	/* We have continued from a PROM abort! */

	/* Put back the old g0 and g4 handlers. */
	vec = (u_int16_t *) sunmon_vbr;
	store = old_g0_g4_vectors;
	*(vec++) = *(store++);
	*(vec++) = *(store++);
	*(vec++) = *(store++);
	*(vec++) = *(store++);

	_mode_kernel(&sunmon_kernel_state, 1);
}

void
prom_halt()
{
	_mode_monitor(&sunmon_kernel_state, 1);
	(*romVectorPtr->exitToMon)();
	for(;;);
	/*NOTREACHED*/
}

/*
 * Caller must pass a string that is in our data segment.
 */
void
prom_boot(bs)
	char *bs;
{
	_mode_monitor(&sunmon_kernel_state, 1);
	(*romVectorPtr->reBoot)(bs);
	(*romVectorPtr->exitToMon)();
	for(;;);
	/*NOTREACHED*/
}


/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing "g4".
 */
struct funcall_frame {
	struct funcall_frame *fr_savfp;
	int fr_savpc;
	int fr_arg[1];
};
/*VARARGS0*/
static void
tracedump(x1)
	int x1;
{
	struct funcall_frame *fp = (struct funcall_frame *)(&x1 - 2);
	u_int stackpage = ((u_int)fp) & ~PGOFSET;

	prom_printf("Begin traceback...fp = 0x%x\n", fp);
	do {
		if (fp == fp->fr_savfp) {
			prom_printf("FP loop at 0x%x", fp);
			break;
		}
		prom_printf("Called from 0x%x, fp=0x%x, args=0x%x 0x%x 0x%x 0x%x\n",
				   fp->fr_savpc, fp->fr_savfp,
				   fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	} while ( (((u_int)fp) & ~PGOFSET) == stackpage);
	prom_printf("End traceback...\n");
}

/* Handlers for the old-school "g0" and "g4" */
void g0_handler __P((void));
void
g0_handler()
{
	_mode_kernel(&sunmon_kernel_state, 1);
	panic("zero");
}
void g4_handler __P((int));
void
g4_handler(addr)
	int addr;
{
	_mode_kernel(&sunmon_kernel_state, 1);
	tracedump(addr);
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
prom_init()
{
	struct bootparam *old_bp;
	struct bootparam *new_bp;
	int bp_shift;
	int i;
	char *p;
	int fl;

	/*
	 * Any second the pointers in the PROM vector are going to
	 * break (since they point into pages zero through three, 
	 * which we like to keep unmapped), so we grab a complete 
	 * copy of the bootparams, taking care to adjust the pointers 
	 * in the copy to also point to the copy.
	 */
	old_bp = *romVectorPtr->bootParam;
	new_bp = &sunmon_bootparam;
	*new_bp = *old_bp;
	bp_shift = ((char *) new_bp) - ((char *) old_bp);
	for(i = 0; i < 8 && new_bp->argPtr[i] != NULL; i++) {
		new_bp->argPtr[i] += bp_shift;
	}
	new_bp->fileName += bp_shift;

	/* Save the PROM's mappings for pages zero through three. */
	_prom_swap_ptes(sunmon_ptes, sunmon_ptes);

	/* Save the PROM monitor Vector Base Register (VBR). */
	sunmon_vbr = getvbr();

	/* Arrange for "trap #14" to cause a PROM abort. */
	sunmon_vbr[32+14] = romVectorPtr->abortEntry;

	/* Try to find some options. */
	p = prom_getbootargs();
	if (p != NULL) {

		/* Skip any whitespace */
		for(; *p != '-'; )
			if (*(p++) == '\0') {
				p = NULL;
				break;
			}
	}

	/* If we have options. */
	if (p != NULL) {
#ifdef	DEBUG
		prom_printf("boot options: %s\n", p);
#endif
		for(; *(++p); ) {
			BOOT_FLAG(*p, fl);
			if (fl)
				boothowto |= fl;
			else
				prom_printf("unknown option `%c'\n", *p);
		}
	}
}
