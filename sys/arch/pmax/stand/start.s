/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Header: /sprite/src/boot/decprom/ds3100.md/RCS/start.s,
 *	v 1.1 90/02/16 16:19:39 shirriff Exp SPRITE (DECWRL)
 * from: @(#)start.s	7.5 (Berkeley) 2/15/93
 * $Id: start.s,v 1.1.1.1 1993/10/12 03:22:44 deraadt Exp $
 *
 * start.s -
 *
 *     Contains code that is the first executed at boot time.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#include <machine/regdef.h>
#include <machine/machConst.h>
#include <machine/machAsmDefs.h>
#include <pmax/stand/dec_prom.h>

/*
 * Amount to take off of the stack for the benefit of the debugger.
 */
#define START_FRAME	((4 * 4) + 4 + 4)

	.globl	start
start:
	.set	noreorder
	li	v0, MACH_SR_BOOT_EXC_VEC
	mtc0	v0, MACH_COP_0_STATUS_REG	# Disable interrupts
	la	sp, start - START_FRAME
	sw	zero, START_FRAME - 4(sp)	# Zero out old ra for debugger
	sw	zero, START_FRAME - 8(sp)	# Zero out old fp for debugger
#ifdef DS3100
	la	a3, callvec			# init call vector
#endif
	sw	a3, callv			# save call vector
	move	s0, a0				# save argc
	move	s1, a1				# save argv
	la	a0, edata			# clear BSS
	la	a1, end
	jal	bzero				# bzero(edata, end - edata)
	subu	a1, a1, a0
	move	a0, s0				# restore argc
	jal	main				# main(argc, argv)
	move	a1, s1				# restore argv
	li	v0, DEC_PROM_RESTART		# done, so call prom
	j	v0
	nop

/* dummy routine for gcc2 */
	.globl	__main
__main:
	j	ra
	nop

LEAF(prom_restart)
	li	v0, DEC_PROM_RESTART
	j	v0
	nop
END(prom_restart)

LEAF(prom_open)
	li	v0, DEC_PROM_OPEN
	j	v0
	nop
END(prom_open)

LEAF(prom_lseek)
	li	v0, DEC_PROM_LSEEK
	j	v0
	nop
END(prom_lseek)

LEAF(prom_read)
	li	v0, DEC_PROM_READ
	j	v0
	nop
END(prom_read)

LEAF(printf)
	lw	v0, callv	# get pointer to call back vectors
	sw	a1, 4(sp)	# store args on stack for printf
	lw	v0, 48(v0)	# offset for callv->printf
	sw	a2, 8(sp)
	j	v0		# call PROM printf
	sw	a3, 12(sp)
END(printf)
