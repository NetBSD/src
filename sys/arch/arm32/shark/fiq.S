/*	$NetBSD: fiq.S,v 1.2 1998/05/01 21:18:39 cgd Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * fiq.S
 *
 * Low level fiq handlers
 *
 * Created      : 19/05/97
 */

#include <machine/cpu.h>
#include <arm32/shark/fiq.h>
#include <arm32/shark/sequoia.h>

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text

/* this variable is used to detect when it is necessary to unwedge the
   sequoia SMI/PMI edge detect logic.  *sigh* */
Lfiqs_happened:
	.word	_fiqs_happened
Lsequoia_index_cache:
	.word	_sequoia_index_cache

	.data
	.global	_fiqs_happened
_fiqs_happened:
	.word	0

	.text

	.global	_shark_fiq
	.global	_shark_fiq_end

/*
 *	r8  - VAM_IO_DATA = address of IO space in virtual memory
 *	r9  - C routine to call (0 => no routine)
 *	r10 - argument (0 => pass PMI reason register)
 *	r11 - scratch
 *	r12 - scratch
 *	r13 - stack for C routine call
 */

_shark_fiq:
	/* clear the FIQ immediately */
	/* do the FIQ/SMI clear here to avoid synchronization problems!
	   both the chip enable (caused by the ISA register read) and
	   the sequoia bit clear must be done.  do it in that order so
           the sequoia port accesses will clear the chip enable caused
           by accessing FIQ_CLEAR_IOPORT
        */
	/* set up to read/write the power management status register */
	ldr	r11, Lsequoia_index_cache
	ldr	r12, [r11]

	/* see if the index is properly set.  unfortunately, the common
	   case will probably be that the FIQ handler (e.g. smartcard)
	   talks to the sequoia, so the index will need to be set. */
	cmp	r12, #PMC_PMSR_REG
	movne	r12, #PMC_PMSR_REG
#ifdef STRH
	strneh	r12, [r8, #SEQUOIA_INDEX]
#else
	.word	0x11c8c2b4
#endif
	strne	r12, [r11]

	/* set SMIACT by changing the power management mode to sleep */
	mov	r12, #PMMD_SLEEP
#ifdef STRH
	strh	r12, [r8, #SEQUOIA_DATA]
#else
	.word	0xe1c8c2b6
#endif

	/* get the PMI reason register, if desired */
	cmp	r10, #0
#ifdef LDRH
	ldreqh	r10, [r8, #SEQUOIA_DATA]
#else
	.word	0x01d8a2b6  /* ldrh r10, [r8, #0x26] */
#endif

	/* simultaneously clear the PMI reason bits and clear SMIACT */
	mov	r12, #PMMD_ON
	orr	r12, r12, #PMSR_M_PMISRC
#ifdef STRH
	strh	r12, [r8, #SEQUOIA_DATA]
#else
	.word	0xe1c8c2b6
#endif

	ldr	r12, Lfiqs_happened
	ldr	r11, [r12]
	add	r11, r11, #1
	str	r11, [r12]
	
	cmp	r9, #0
	subeqs	pc, lr, #4             /* no routine => return from trap */

	/* assume that the C routine follows the ARM procedure call standard.
	   save only user registers and let the C code save the rest.
	   r0-r3, r12, r14 clobbered.  other regs should be restored
	   by C code.
        */
	stmfd	sp!, {r0-r3, lr}       /* save user registers */
	mov	r0, r10                /* pass the PMI reason register */
	mov	lr, pc                 /* call C routine */
	mov	pc, r9
	ldmfd	sp!, {r0-r3, lr}       /* restore user registers */
	subs	pc, lr, #4             /* return from trap */
_shark_fiq_end:

/* End of fiq.S */
