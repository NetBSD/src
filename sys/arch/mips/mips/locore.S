/*	$NetBSD: locore.S,v 1.41 1997/07/20 19:48:03 jonathan Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
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
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */

/*
 *	Contains code that is the first executed at boot time plus
 *	assembly language support routines.
 */

#include <sys/errno.h>
#include <sys/syscall.h>

#include <mips/cpuregs.h>	/* XXX - misnomer ? */
#include <mips/regnum.h>	/* XXX - ditto */
#include <mips/asm.h>
#include <mips/pte.h>
#include <machine/param.h>

/*----------------------------------------------------------------------------
 *
 *  Macros used to save and restore registers when entering and
 *  exiting the kernel.  When coming from user space, we need
 *  to initialize the kernel stack and on leaving, return to the
 *  user's stack.
 *  When coming from kernel-space we just save and restore from the
 *  kernel stack set up by a previous user-mode exception.
 *
 *----------------------------------------------------------------------------
 */


/*
 * Restore saved user registers before returning
 * to user-space after an exception or TLB miss.
 *
 * XXX	we don't restore all the regs, because the r3000 and r4000
 *     	use different mechanisms to set up the return address.
 *     	the current Pica code uses v1 for that, so we leave
 *     	it up to the caller of this macro to restore AT and v0.
 *	Don't ask why, I don't know.
 */
#define RESTORE_USER_REGS(saveaddr)			\
	lw	v1, saveaddr+U_PCB_REGS+(V1 * 4)		; \
	lw	a0, saveaddr+U_PCB_REGS+(A0 * 4)		; \
	lw	a1, saveaddr+U_PCB_REGS+(A1 * 4)		; \
	lw	a2, saveaddr+U_PCB_REGS+(A2 * 4)		; \
	lw	a3, saveaddr+U_PCB_REGS+(A3 * 4)		; \
	lw	t0, saveaddr+U_PCB_REGS+(T0 * 4)		; \
	lw	t1, saveaddr+U_PCB_REGS+(T1 * 4)		; \
	lw	t2, saveaddr+U_PCB_REGS+(T2 * 4)		; \
	lw	t3, saveaddr+U_PCB_REGS+(T3 * 4)		; \
	lw	t4, saveaddr+U_PCB_REGS+(T4 * 4)		; \
	lw	t5, saveaddr+U_PCB_REGS+(T5 * 4)		; \
	lw	t6, saveaddr+U_PCB_REGS+(T6 * 4)		; \
	lw	t7, saveaddr+U_PCB_REGS+(T7 * 4)		; \
	lw	s0, saveaddr+U_PCB_REGS+(S0 * 4)		; \
	lw	s1, saveaddr+U_PCB_REGS+(S1 * 4)		; \
	lw	s2, saveaddr+U_PCB_REGS+(S2 * 4)		; \
	lw	s3, saveaddr+U_PCB_REGS+(S3 * 4)		; \
	lw	s4, saveaddr+U_PCB_REGS+(S4 * 4)		; \
	lw	s5, saveaddr+U_PCB_REGS+(S5 * 4)		; \
	lw	s6, saveaddr+U_PCB_REGS+(S6 * 4)		; \
	lw	s7, saveaddr+U_PCB_REGS+(S7 * 4)		; \
	lw	t8, saveaddr+U_PCB_REGS+(T8 * 4)		; \
	lw	t9, saveaddr+U_PCB_REGS+(T9 * 4)		; \
	lw	gp, saveaddr+U_PCB_REGS+(GP * 4)		; \
	lw	sp, saveaddr+U_PCB_REGS+(SP * 4)		; \
	lw	s8, saveaddr+U_PCB_REGS+(S8 * 4)		; \
	lw	ra, saveaddr+U_PCB_REGS+(RA * 4)


/*
 * Restore call-used registers(?) before returning
 * to the previous  kernel stackframe after a  after an exception or 
 * TLB miss from kernel space.
 *
 * XXX	we don't restore all the regs, because the r3000 and r4000
 *     	use different mechanisms to set up the return address.
 *     	the current Pica code uses v1 for that, so we leave
 *     	it up to the caller of this macro to restore AT and v0.
 *	Don't ask why, I don't konw.
 */
#define	RESTORE_KERN_REGISTERS(offset)			\
	lw	v1, offset + 8(sp)		; \
	lw	a0, offset + 12(sp)		; \
	lw	a1, offset + 16(sp)		; \
	lw	a2, offset + 20(sp)		; \
	lw	a3, offset + 24(sp)		; \
	lw	t0, offset + 28(sp)		; \
	lw	t1, offset + 32(sp)		; \
	lw	t2, offset + 36(sp)		; \
	lw	t3, offset + 40(sp)		; \
	lw	t4, offset + 44(sp)		; \
	lw	t5, offset + 48(sp)		; \
	lw	t6, offset + 52(sp)		; \
	lw	t7, offset + 56(sp)		; \
	lw	t8, offset + 60(sp)		; \
	lw	t9, offset + 64(sp)		; \
	lw	ra, offset + 68(sp)



/*----------------------------------------------------------------------------
 *
 * start -- boostrap kernel entry point
 *
 *----------------------------------------------------------------------------
 */
#include "assym.h"

	.set	noreorder

/*
 * Amount to take off of the stack for the benefit of the debugger.
 */
#define START_FRAME	((4 * 4) + 4 + 4)

	.globl	start
	.globl	_C_LABEL(kernel_text)
start:
_C_LABEL(kernel_text):
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
/*
 * Initialize stack and call machine startup.
 */
	la	sp, start - START_FRAME
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)
#endif
	li	t0, MIPS_SR_COP_1_BIT		# Disable interrupts and
	mtc0	t0, MIPS_COP_0_STATUS_REG	#   enable the fp coprocessor
	nop
	nop
	mfc0	t0, MIPS_COP_0_PRID		# read processor ID register

	nop					# XXX r4000 pipeline:
	nop					# wait for new status to
	nop					# to be effective
	nop
	cfc1	t1, MIPS_FPU_ID			# read FPU ID register
	sw	t0, _C_LABEL(cpu_id)		# save PRID register
	sw	t1, _C_LABEL(fpu_id)		# save FPU ID register
	sw	zero, START_FRAME - 4(sp)	# Zero out old ra for debugger
	jal	_C_LABEL(mach_init)		# mach_init(argc, argv, envp)
	sw	zero, START_FRAME - 8(sp)	# Zero out old fp for debugger

	li	sp, KERNELSTACK - START_FRAME	# switch to standard stack
	jal	_C_LABEL(main)			# main(regs)
	move	a0, zero
	PANIC("main() returned")		# main never returns
	.set	at

/*
 * This code is copied the user's stack for returning from signal handlers
 * (see sendsig() and sigreturn()). We have to compute the address
 * of the sigcontext struct for the sigreturn call.
 *
 * NB: we cannot profile sigcode(), it executes from userspace.
 */
NLEAF(sigcode)
	addu	a0, sp, 16		# address of sigcontext
	li	v0, SYS_sigreturn	# sigreturn(scp)
	syscall
	break	0			# just in case sigreturn fails
ALEAF(esigcode)
END(sigcode)

/*
 * GCC2 seems to want to call __main in main() for some reason.
 */
LEAF(__main)
	j	ra
	nop
END(__main)

/*
 * Primitives
 */
/*
 * netorder = htonl(hostorder)
 * hostorder = ntohl(netorder)
 */
LEAF(htonl)				# a0 = 0x11223344, return 0x44332211
ALEAF(ntohl)
	srl	v1, a0, 24		# v1 = 0x00000011
	sll	v0, a0, 24		# v0 = 0x44000000
	or	v0, v0, v1
	and	v1, a0, 0xff00
	sll	v1, v1, 8		# v1 = 0x00330000
	or	v0, v0, v1
	srl	v1, a0, 8
	and	v1, v1, 0xff00		# v1 = 0x00002200
	j	ra
	or	v0, v0, v1
END(htonl)

/*
 * netorder = htons(hostorder)
 * hostorder = ntohs(netorder)
 */
LEAF(htons)
ALEAF(ntohs)
	srl	v0, a0, 8
	and	v0, v0, 0xff
	sll	v1, a0, 8
	and	v1, v1, 0xff00
	j	ra
	or	v0, v0, v1
END(htons)

/*
 * strlen(str)
 */
LEAF(strlen)
	addu	v1, a0, 1
1:
	lb	v0, 0(a0)		# get byte from string
	addu	a0, a0, 1		# increment pointer
	bne	v0, zero, 1b		# continue if not end
	nop
	j	ra
	subu	v0, a0, v1		# compute length - 1 for '\0' char
END(strlen)

/*
 * NOTE: this version assumes unsigned chars in order to be "8 bit clean".
 */
LEAF(strcmp)
1:
	lbu	t0, 0(a0)		# get two bytes and compare them
	lbu	t1, 0(a1)
	beq	t0, zero, LessOrEq	# end of first string?
	nop
	bne	t0, t1, NotEq
	nop
	lbu	t0, 1(a0)		# unroll loop
	lbu	t1, 1(a1)
	beq	t0, zero, LessOrEq	# end of first string?
	addu	a0, a0, 2
	beq	t0, t1, 1b
	addu	a1, a1, 2
NotEq:
	j	ra
	subu	v0, t0, t1
LessOrEq:
	j	ra
	subu	v0, zero, t1
END(strcmp)

/*
 * bzero(s1, n)
 */
LEAF(bzero)
	blt	a1, 12, smallclr	# small amount to clear?
	subu	a3, zero, a0		# compute # bytes to word align address
	and	a3, a3, 3
	beq	a3, zero, 1f		# skip if word aligned
	subu	a1, a1, a3		# subtract from remaining count
	swr	zero, 0(a0)		# clear 1, 2, or 3 bytes to align
	addu	a0, a0, a3
1:
	and	v0, a1, 3		# compute number of words left
	subu	a3, a1, v0
	move	a1, v0
	addu	a3, a3, a0		# compute ending address
2:
	addu	a0, a0, 4		# clear words
	bne	a0, a3, 2b		#  unrolling loop does not help
	sw	zero, -4(a0)		#  since we are limited by memory speed
smallclr:
	ble	a1, zero, 2f
	addu	a3, a1, a0		# compute ending address
1:
	addu	a0, a0, 1		# clear bytes
	bne	a0, a3, 1b
	sb	zero, -1(a0)
2:
	j	ra
	nop
END(bzero)

/*
 * bcmp(s1, s2, n)
 */
LEAF(bcmp)
	blt	a2, 16, smallcmp	# is it worth any trouble?
	xor	v0, a0, a1		# compare low two bits of addresses
	and	v0, v0, 3
	subu	a3, zero, a1		# compute # bytes to word align address
	bne	v0, zero, unalignedcmp	# not possible to align addresses
	and	a3, a3, 3

	beq	a3, zero, 1f
	subu	a2, a2, a3		# subtract from remaining count
	move	v0, v1			# init v0,v1 so unmodified bytes match
	lwr	v0, 0(a0)		# read 1, 2, or 3 bytes
	lwr	v1, 0(a1)
	addu	a1, a1, a3
	bne	v0, v1, nomatch
	addu	a0, a0, a3
1:
	and	a3, a2, ~3		# compute number of whole words left
	subu	a2, a2, a3		#   which has to be >= (16-3) & ~3
	addu	a3, a3, a0		# compute ending address
2:
	lw	v0, 0(a0)		# compare words
	lw	v1, 0(a1)
	addu	a0, a0, 4
	bne	v0, v1, nomatch
	addu	a1, a1, 4
	bne	a0, a3, 2b
	nop
	b	smallcmp		# finish remainder
	nop
unalignedcmp:
	beq	a3, zero, 2f
	subu	a2, a2, a3		# subtract from remaining count
	addu	a3, a3, a0		# compute ending address
1:
	lbu	v0, 0(a0)		# compare bytes until a1 word aligned
	lbu	v1, 0(a1)
	addu	a0, a0, 1
	bne	v0, v1, nomatch
	addu	a1, a1, 1
	bne	a0, a3, 1b
	nop
2:
	and	a3, a2, ~3		# compute number of whole words left
	subu	a2, a2, a3		#   which has to be >= (16-3) & ~3
	addu	a3, a3, a0		# compute ending address
3:
	lwr	v0, 0(a0)		# compare words a0 unaligned, a1 aligned
	lwl	v0, 3(a0)
	lw	v1, 0(a1)
	addu	a0, a0, 4
	bne	v0, v1, nomatch
	addu	a1, a1, 4
	bne	a0, a3, 3b
	nop
smallcmp:
	ble	a2, zero, match
	addu	a3, a2, a0		# compute ending address
1:
	lbu	v0, 0(a0)
	lbu	v1, 0(a1)
	addu	a0, a0, 1
	bne	v0, v1, nomatch
	addu	a1, a1, 1
	bne	a0, a3, 1b
	nop
match:
	j	ra
	move	v0, zero
nomatch:
	j	ra
	li	v0, 1
END(bcmp)

/*
 * memcpy(to, from, len)
 * {ov}bcopy(from, to, len)
 */
LEAF(memcpy)
	move	v0, a0			# swap from and to
	move	a0, a1
	move	a1, v0
ALEAF(bcopy)
ALEAF(ovbcopy)
	addu	t0, a0, a2		# t0 = end of s1 region
	sltu	t1, a1, t0
	sltu	t2, a0, a1
	and	t1, t1, t2		# t1 = true if from < to < (from+len)
	beq	t1, zero, forward	# non overlapping, do forward copy
	slt	t2, a2, 12		# check for small copy

	ble	a2, zero, 2f
	addu	t1, a1, a2		# t1 = end of to region
1:
	lb	v1, -1(t0)		# copy bytes backwards,
	subu	t0, t0, 1		#   doesnt happen often so do slow way
	subu	t1, t1, 1
	bne	t0, a0, 1b
	sb	v1, 0(t1)
2:
	j	ra
	nop
forward:
	bne	t2, zero, smallcpy	# do a small bcopy
	xor	v1, a0, a1		# compare low two bits of addresses
	and	v1, v1, 3
	subu	a3, zero, a1		# compute # bytes to word align address
	beq	v1, zero, aligned	# addresses can be word aligned
	and	a3, a3, 3

	beq	a3, zero, 1f
	subu	a2, a2, a3		# subtract from remaining count
	lwr	v1, 0(a0)		# get next 4 bytes (unaligned)
	lwl	v1, 3(a0)
	addu	a0, a0, a3
	swr	v1, 0(a1)		# store 1, 2, or 3 bytes to align a1
	addu	a1, a1, a3
1:
	and	v1, a2, 3		# compute number of words left
	subu	a3, a2, v1
	move	a2, v1
	addu	a3, a3, a0		# compute ending address
2:
	lwr	v1, 0(a0)		# copy words a0 unaligned, a1 aligned
	lwl	v1, 3(a0)
	addu	a0, a0, 4
#ifdef MIPS3
	sw	v1, 0(a1)
	addu	a1, a1, 4
	bne	a0, a3, 2b
	nop				# We have to do this mmu-bug
#else
	addu	a1, a1, 4
	bne	a0, a3, 2b
	sw	v1, -4(a1)
#endif
	b	smallcpy
	nop
aligned:
	beq	a3, zero, 1f
	subu	a2, a2, a3		# subtract from remaining count
	lwr	v1, 0(a0)		# copy 1, 2, or 3 bytes to align
	addu	a0, a0, a3
	swr	v1, 0(a1)
	addu	a1, a1, a3
1:
	and	v1, a2, 3		# compute number of whole words left
	subu	a3, a2, v1
	move	a2, v1
	addu	a3, a3, a0		# compute ending address
2:
	lw	v1, 0(a0)		# copy words
	addu	a0, a0, 4
#ifdef MIPS3
	sw	v1, 0(a1)
	bne	a0, a3, 2b
	addu	a1, a1, 4
#else
	addu	a1, a1, 4
	bne	a0, a3, 2b
	sw	v1, -4(a1)
#endif
smallcpy:
	ble	a2, zero, 2f
	addu	a3, a2, a0		# compute ending address
1:
	lbu	v1, 0(a0)		# copy bytes
	addu	a0, a0, 1
#ifdef MIPS3
	sb	v1, 0(a1)
	addu	a1, a1, 1
	bne	a0, a3, 1b
	nop				# MMU BUG ? (cant do -1(a1) !!
#else
	addu	a1, a1, 1
	bne	a0, a3, 1b
	sb	v1, -1(a1)
#endif
2:
	j	ra
	nop
END(memcpy)

/*
 * The following primitives manipulate the run queues.  whichqs tells which
 * of the 32 queues qs have processes in them.  Setrunqueue puts processes
 * into queues, remrunqueue removes them from queues.  The running process is
 * on no queue, other processes are on a queue related to p->p_priority,
 * divided by 4 actually to shrink the 0-127 range of priorities into the 32
 * available queues.
 */
/*
 * setrunqueue(p)
 *	proc *p;
 *
 * Call should be made at splclock(), and p->p_stat should be SRUN.
 */
NON_LEAF(setrunqueue, STAND_FRAME_SIZE, ra)
	subu	sp, sp, STAND_FRAME_SIZE
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
	lw	t0, P_BACK(a0)		## firewall: p->p_back must be 0
	sw	ra, STAND_RA_OFFSET(sp)	##
	beq	t0, zero, 1f		##
	lbu	t0, P_PRIORITY(a0)	# put on p->p_priority / 4 queue
	PANIC("setrunqueue")		##
1:
	li	t1, 1			# compute corresponding bit
	srl	t0, t0, 2		# compute index into 'whichqs'
	sll	t1, t1, t0
	lw	t2, _C_LABEL(whichqs)	# set corresponding bit
	nop
	or	t2, t2, t1
	sw	t2, _C_LABEL(whichqs)
	sll	t0, t0, 3		# compute index into 'qs'
	la	t1, _C_LABEL(qs)
	addu	t0, t0, t1		# t0 = qp = &qs[pri >> 2]
	lw	t1, P_BACK(t0)		# t1 = qp->ph_rlink
	sw	t0, P_FORW(a0)		# p->p_forw = qp
	sw	t1, P_BACK(a0)		# p->p_back = qp->ph_rlink
	sw	a0, P_FORW(t1)		# p->p_back->p_forw = p;
	sw	a0, P_BACK(t0)		# qp->ph_rlink = p
	j	ra
	addu	sp, sp, STAND_FRAME_SIZE
END(setrunqueue)

/*
 * remrunqueue(p)
 *
 * Call should be made at splclock().
 */
NON_LEAF(remrunqueue, STAND_FRAME_SIZE, ra)
	subu	sp, sp, STAND_FRAME_SIZE
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
	lbu	t0, P_PRIORITY(a0)	# get from p->p_priority / 4 queue
	li	t1, 1			# compute corresponding bit
	srl	t0, t0, 2		# compute index into 'whichqs'
	lw	t2, _C_LABEL(whichqs)	# check corresponding bit
	sll	t1, t1, t0
	and	v0, t2, t1
	sw	ra, STAND_RA_OFFSET(sp)	##
	bne	v0, zero, 1f		##
	lw	v0, P_BACK(a0)		# v0 = p->p_back
	PANIC("remrunqueue")		## it wasnt recorded to be on its q
1:
	lw	v1, P_FORW(a0)		# v1 = p->p_forw
	nop
	sw	v1, P_FORW(v0)		# p->p_back->p_forw = p->p_forw;
	sw	v0, P_BACK(v1)		# p->p_forw->p_back = p->r_rlink
	sll	t0, t0, 3		# compute index into 'qs'
	la	v0, _C_LABEL(qs)
	addu	t0, t0, v0		# t0 = qp = &qs[pri >> 2]
	lw	v0, P_FORW(t0)		# check if queue empty
	nop
	bne	v0, t0, 2f		# No. qp->ph_link != qp
	nop
	xor	t2, t2, t1		# clear corresponding bit in 'whichqs'
	sw	t2, _C_LABEL(whichqs)
2:
	sw	zero, P_BACK(a0)	## for firewall checking
	j	ra
	addu	sp, sp, STAND_FRAME_SIZE
END(remrunqueue)

/*
 * When no processes are on the runq, cpu_switch branches to idle
 * to wait for something to come ready.
 * Note: this is really a part of cpu_switch() but defined here for kernel
 * profiling.
 */
LEAF(idle)
	li	t0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	mtc0	t0, MIPS_COP_0_STATUS_REG	# enable all interrupts
	nop
	sw	zero, _C_LABEL(curproc)		# set curproc NULL for stats
1:
	lw	t0, _C_LABEL(whichqs)		# look for non-empty queue
	nop
	beq	t0, zero, 1b
	nop
	b	sw1
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable all interrupts
END(idle)

/*
 * cpu_switch(struct proc *)
 * Find the highest priority process and resume it.
 */
NON_LEAF(cpu_switch, STAND_FRAME_SIZE, ra)
	lw	a0, _C_LABEL(curpcb)
	mfc0	t0, MIPS_COP_0_STATUS_REG
	sw	s0, U_PCB_CONTEXT+0(a0)
	sw	s1, U_PCB_CONTEXT+4(a0)
	sw	s2, U_PCB_CONTEXT+8(a0)
	sw	s3, U_PCB_CONTEXT+12(a0)
	sw	s4, U_PCB_CONTEXT+16(a0)
	sw	s5, U_PCB_CONTEXT+20(a0)
	sw	s6, U_PCB_CONTEXT+24(a0)
	sw	s7, U_PCB_CONTEXT+28(a0)
	sw	sp, U_PCB_CONTEXT+32(a0)
	sw	s8, U_PCB_CONTEXT+36(a0)
	sw	ra, U_PCB_CONTEXT+40(a0)
	sw	t0, U_PCB_CONTEXT+44(a0)
	subu	sp, sp, STAND_FRAME_SIZE
	sw	ra, STAND_RA_OFFSET(sp)
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
	lw	t0, cnt+V_SWTCH
	lw	t1, _C_LABEL(whichqs)
	addu	t0, t0, 1
	sw	t0, cnt+V_SWTCH
	beq	t1, zero, idle
	mtc0	zero, MIPS_COP_0_STATUS_REG
/*
 * Entered here from idle() and switch_exit().
 */
sw1:
	nop					# wait for intrs disabled
	nop
	nop					# extra cycles on r4000	
	nop					# extra cycles on r4000	

	lw	t0, _C_LABEL(whichqs)		# look for non-empty queue
	li	t2, -1				# t2 = lowest bit set
	beq	t0, zero, _C_LABEL(idle)	# if none, idle
	move	t3, t0				# t3 = saved whichqs
1:
	addu	t2, t2, 1
	and	t1, t0, 1			# bit set?
	beq	t1, zero, 1b
	srl	t0, t0, 1			# try next bit
/*
 * Remove process from queue.
 */
	sll	t0, t2, 3
	la	t1, _C_LABEL(qs)
	addu	t0, t0, t1			# t0 = qp = &qs[highbit]
	lw	a0, P_FORW(t0)			# a0 = p = highest pri process
	nop
	lw	v0, P_FORW(a0)			# v0 = p->p_forw
	bne	t0, a0, 2f			# make sure something in queue
	sw	v0, P_FORW(t0)			# qp->ph_link = p->p_forw;
	PANIC("cpu_switch")			# nothing in queue
2:
	sw	t0, P_BACK(v0)			# p->p_forw->p_back = qp
	bne	v0, t0, 3f			# queue still not empty
	sw	zero, P_BACK(a0)		## for firewall checking
	li	v1, 1				# compute bit in 'whichqs'
	sll	v1, v1, t2
	xor	t3, t3, v1			# clear bit in 'whichqs'
	sw	t3, _C_LABEL(whichqs)
3:
/*
 * Switch to new context.
 */
	sw	zero, _C_LABEL(want_resched)
	jal	_C_LABEL(pmap_alloc_tlbpid)	# v0 = TLB PID
	move	s0, a0				# BDSLOT: save p
	sw	s0, _C_LABEL(curproc)		# set curproc
	lw	t0, P_MD_UPTE+0(s0)		# t0 = first u. pte
	lw	t1, P_MD_UPTE+4(s0)		# t1 = 2nd u. pte
	lw	s0, P_ADDR(s0)
	lw	s1, _C_LABEL(mips_locore_jumpvec) + MIPSX_CPU_SWITCH_RESUME
	sw	s0, _C_LABEL(curpcb)		# set curpcb
	jr	s1				# CPU-specific: resume process
	nop
END(cpu_switch)

/*
 * savectx(struct user *up)
 */
LEAF(savectx)
	mfc0	v0, MIPS_COP_0_STATUS_REG
	sw	s0, U_PCB_CONTEXT+0(a0)
	sw	s1, U_PCB_CONTEXT+4(a0)
	sw	s2, U_PCB_CONTEXT+8(a0)
	sw	s3, U_PCB_CONTEXT+12(a0)
	sw	s4, U_PCB_CONTEXT+16(a0)
	sw	s5, U_PCB_CONTEXT+20(a0)
	sw	s6, U_PCB_CONTEXT+24(a0)
	sw	s7, U_PCB_CONTEXT+28(a0)
	sw	sp, U_PCB_CONTEXT+32(a0)
	sw	s8, U_PCB_CONTEXT+36(a0)
	sw	ra, U_PCB_CONTEXT+40(a0)
	sw	v0, U_PCB_CONTEXT+44(a0)
	j	ra
	move	v0, zero
END(savectx)

#ifdef DDB
/*
 * setjmp(label_t *)
 * longjmp(label_t *)
 */
LEAF(setjmp)
	mfc0	v0, MIPS_COP_0_STATUS_REG
	sw	s0, 0(a0)
	sw	s1, 4(a0)
	sw	s2, 8(a0)
	sw	s3, 12(a0)
	sw	s4, 16(a0)
	sw	s5, 20(a0)
	sw	s6, 24(a0)
	sw	s7, 28(a0)
	sw	sp, 32(a0)
	sw	s8, 36(a0)
	sw	ra, 40(a0)
	sw	v0, 44(a0)
	j	ra
	move	v0, zero
END(setjmp)

LEAF(longjmp)
	lw	v0, 44(a0)
	lw	ra, 40(a0)
	lw	s0, 0(a0)
	lw	s1, 4(a0)
	lw	s2, 8(a0)
	lw	s3, 12(a0)
	lw	s4, 16(a0)
	lw	s5, 20(a0)
	lw	s6, 24(a0)
	lw	s7, 28(a0)
	lw	sp, 32(a0)
	lw	s8, 36(a0)
	mtc0	v0, MIPS_COP_0_STATUS_REG
	j	ra
	li	v0, 1
END(longjmp)
#endif

#if 0 /* NOTUSED */
/*
 * Insert 'p' after 'q'.
 *	_insque(p, q)
 *		caddr_t p, q;
 */
LEAF(_insque)
	lw	v0, 0(a1)		# v0 = q->next
	sw	a1, 4(a0)		# p->prev = q
	sw	v0, 0(a0)		# p->next = q->next
	sw	a0, 4(v0)		# q->next->prev = p
	j	ra
	sw	a0, 0(a1)		# q->next = p
END(_insque)

/*
 * Remove item 'p' from queue.
 *	_remque(p)
 *		caddr_t p;
 */
LEAF(_remque)
	lw	v0, 0(a0)		# v0 = p->next
	lw	v1, 4(a0)		# v1 = p->prev
	nop
	sw	v0, 0(v1)		# p->prev->next = p->next
	j	ra
	sw	v1, 4(v0)		# p->next->prev = p->prev
END(_remque)
#endif

/*
 * copystr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NIL) in *lencopied.  If the 
 * string is too long, return ENAMETOOLONG; else return 0.
 */
LEAF(copystr)
	move	t0, a2
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)			# each byte until NIL
	addu	a0, a0, 1
	bne	a2, zero, 1b			# less than maxlen
	addu	a1, a1, 1  
	li	v0, ENAMETOOLONG		# run out of space
2:
	beq	a3, zero, 3f			# return num. of copied bytes
	subu	a2, t0, a2			# if the 4th arg was non-NULL
	sw	a2, 0(a3) 
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	nop
END(copystr)

/*
 * copyinstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyinstr) 
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copystrerr)
	blt	a0, zero, _C_LABEL(copystrerr)
	sw	v0, U_PCB_ONFAULT(v1)
	move	t0, a2
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)
	addu	a0, a0, 1
	bne	a2, zero, 1b
	addu	a1, a1, 1
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	subu	a2, t0, a2
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	sw	zero, U_PCB_ONFAULT(v1)
END(copyinstr)

/*
 * copyoutstr(caddr_t from, caddr_t to, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyoutstr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copystrerr)
	blt	a1, zero, _C_LABEL(copystrerr)
	sw	v0, U_PCB_ONFAULT(v1)
	move	t0, a2
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)
	addu	a0, a0, 1
	bne	a2, zero, 1b
	addu	a1, a1, 1
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	subu	a2, t0, a2
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	sw	zero, U_PCB_ONFAULT(v1)
END(copyoutstr)

LEAF(copystrerr)
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, EFAULT			# return EFAULT
END(copystrerr)

/*
 * Copy specified amount of data from user space into the kernel
 *	copyin(from, to, len)
 *		caddr_t from;	(user source address)
 *		caddr_t to;	(kernel destination address)
 *		unsigned len;
 */
NON_LEAF(copyin, STAND_FRAME_SIZE, ra)
	subu	sp, sp, STAND_FRAME_SIZE
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
	sw	ra, STAND_RA_OFFSET(sp)
	blt	a0, zero, _C_LABEL(copyerr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copyerr)
	jal	_C_LABEL(bcopy)			# call bcopy()
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, _C_LABEL(curpcb)
	lw	ra, STAND_RA_OFFSET(sp)
	addu	sp, sp, STAND_FRAME_SIZE
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(copyin)

/*
 * Copy specified amount of data from kernel to the user space
 *	copyout(from, to, len)
 *		caddr_t from;	(kernel source address)
 *		caddr_t to;	(user destination address)
 *		unsigned len;
 */
NON_LEAF(copyout, STAND_FRAME_SIZE, ra)
	subu	sp, sp, STAND_FRAME_SIZE
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
	sw	ra, STAND_RA_OFFSET(sp)
	blt	a1, zero, _C_LABEL(copyerr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copyerr)
	jal	_C_LABEL(bcopy)			# call bcopy()
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, _C_LABEL(curpcb)
	lw	ra, STAND_RA_OFFSET(sp)
	addu	sp, sp, STAND_FRAME_SIZE
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(copyout)

LEAF(copyerr)
	lw	v1, _C_LABEL(curpcb)
	lw	ra, STAND_RA_OFFSET(sp)
	addu	sp, sp, STAND_FRAME_SIZE
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, EFAULT			# return EFAULT
END(copyerr)

/*
 * fuswintr(caddr_t uaddr);
 * Fetch a short from the user's address space.
 * Can be called during an interrupt.
 */
LEAF(fuswintr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswintrberr)
	blt	a0, zero, _C_LABEL(fswintrberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fuswintr)

/*
 * suswintr(caddr_t uaddr, short x);
 * Store a short in the user's address space.
 * Can be called during an interrupt.
 */
LEAF(suswintr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswintrberr)
	blt	a0, zero, _C_LABEL(fswintrberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suswintr)


/*
 * fuword(caddr_t uaddr);
 * Fetch an int from the user's address space.
 */
LEAF(fuword)
ALEAF(fuiword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lw	v0, 0(a0)			# fetch word
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fuword)
/*
 * fusword(caddr_t uaddr);
 * Fetch a short from the user's address space.
 */
LEAF(fusword)
ALEAF(fuisword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fusword)

/*
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */
LEAF(fubyte)
ALEAF(fuibyte)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lbu	v0, 0(a0)			# fetch byte
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fubyte)

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */
LEAF(suword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sw	a1, 0(a0)			# store word
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suword)

/*
 * Have to flush instruction cache afterwards.
 */
LEAF(suiword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sw	a1, 0(a0)			# store word
	sw	zero, U_PCB_ONFAULT(v1)
	move	v0, zero
	lw	v1, _C_LABEL(mips_locore_jumpvec) + MIPSX_FLUSHICACHE
	jr	v1				# NOTE: must not clobber v0!
	li	a1, 4				# size of word
END(suiword)

/*
 * susword(caddr_t uaddr, short x);
 * Store an short in the user's address space.
 */
LEAF(susword)
ALEAF(suisword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(susword)

/*
 * subyte(caddr_t uaddr, char x);
 * Store a byte in the user's address space.
 */
LEAF(subyte)
ALEAF(suibyte)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sb	a1, 0(a0)			# store byte
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(subyte)

/*
 * badaddr(caddr_t addr, int len);
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access (1=byte, 2=short, 4=long)
 */
LEAF(badaddr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(baderr)
	bne	a1, 1, 2f
	sw	v0, U_PCB_ONFAULT(v1)
	b	5f
	lbu	v0, (a0)
2:
	bne	a1, 2, 4f
	nop
	b	5f
	lhu	v0, (a0)
4:
	lw	v0, (a0)
5:
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero		# made it w/o errors
END(badaddr)

/*
 * Error routine for {f,s}uswintr.  The fault handler in trap.c
 * checks for pcb_onfault set to this fault handler and
 * "bails out" before calling the VM fault handler.
 * (We can not call VM code from interrupt level.)
 */
LEAF(fswintrberr)
	nop
ALEAF(fswberr)
ALEAF(baderr)
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, -1
END(fswintrberr)

/*
 * mips1 or mips3 specific locore code
 */

#ifdef MIPS1
# include "locore_r2000.S"
#endif

#ifdef MIPS3		/* R4000 support  */
# include "locore_r4000.S"
#endif		/* R4000 support */


/*
 * Set/clear software interrupt routines.
 */

LEAF(setsoftclock)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	or	v0, v0, MIPS_SOFT_INT_MASK_0	# set soft clock interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(setsoftclock)

LEAF(clearsoftclock)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	and	v0, v0, ~MIPS_SOFT_INT_MASK_0	# clear soft clock interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(clearsoftclock)

LEAF(setsoftnet)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	or	v0, v0, MIPS_SOFT_INT_MASK_1	# set soft net interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(setsoftnet)

LEAF(clearsoftnet)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# save status register
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable interrupts (2 cycles)
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE_REG	# read cause register
	nop
	and	v0, v0, ~MIPS_SOFT_INT_MASK_1	# clear soft net interrupt
	mtc0	v0, MIPS_COP_0_CAUSE_REG	# save it
	mtc0	v1, MIPS_COP_0_STATUS_REG
	j	ra
	nop
END(clearsoftnet)

/*
 * Set/change interrupt priority routines.
 */

#if 0 /* NOTUSED */
LEAF(MachEnableIntr)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	nop
	or	v0, v0, MIPS_SR_INT_IE
	mtc0	v0, MIPS_COP_0_STATUS_REG	# enable all interrupts
	j	ra
	nop
END(MachEnableIntr)
#endif

LEAF(spl0)
ALEAF(spllow)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	nop
	or	t0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	mtc0	t0, MIPS_COP_0_STATUS_REG	# enable all interrupts
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(spl0)

LEAF(splsoftclock)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~MIPS_SOFT_INT_MASK_0	# disable soft clock
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splsoftclock)

LEAF(splsoftnet)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splsoftnet)

/*
 * hardware-level spls for hardware where the device interrupt priorites
 * are ordered, and map onto mips interrupt pins in increasing priority.
 * This maps directly onto BSD spl levels.
 */

/*
 * Block out int2 (hardware interrupt 0) and lower mips levels.
 */
LEAF(cpu_spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl0)

/*
 * Block out Int3 (hardware interrupt 1) and lower mips levels.
 */
LEAF(cpu_spl1)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL1)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl1)

LEAF(cpu_spl2)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL2)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl2)

LEAF(cpu_spl3)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL3)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl3)

LEAF(cpu_spl4)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL4)

	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl4)

LEAF(cpu_spl5)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_SPL5)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(cpu_spl5)

/*
 * hardware-level spls for hardware where teh interrupt priorites
 * DO NOT  map onto levels.  
 *
 * For now, that means DEcstations that use only two distinct CPU
 * levels, one for TOD clock interrupts, and a second for all other
 * external devices (via an external controller.
 * XXX  the spl handling really needs re-writing from scratch.
 */
LEAF(Mach_spl0)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_0|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl0)

LEAF(Mach_spl1)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_1|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl1)

LEAF(Mach_spl2)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_2|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl2)

LEAF(Mach_spl3)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_3|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl3)

LEAF(Mach_spl4)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_4|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl4)

LEAF(Mach_spl5)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~(MIPS_INT_MASK_5|MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0)
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(Mach_spl5)


/*
 * We define an alternate entry point after mcount is called so it
 * can be used in mcount without causeing a recursive loop.
 */
LEAF(splhigh)
ALEAF(_splhigh)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# read status register
	li	t0, ~MIPS_SR_INT_IE	# disable all interrupts
	and	t0, t0, v0
	mtc0	t0, MIPS_COP_0_STATUS_REG	# save it
	nop					# 3 ins to disable on r4x00
	j	ra
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
END(splhigh)

/*
 * Restore saved interrupt mask.
 */
LEAF(splx)
ALEAF(_splx)
	mfc0	v0, MIPS_COP_0_STATUS_REG
	li	t0, ~(MIPS_INT_MASK | MIPS_SR_INT_IE)
	and	t0, t0, v0
	or	t0, t0, a0
	mtc0	t0, MIPS_COP_0_STATUS_REG
	nop					# 3 ins to disable
	j	ra
	nop
END(splx)



#if 0 /* NOTUSED */
/*----------------------------------------------------------------------------
 *
 * MachGetCauseReg --
 *
 *	Return the current value of the cause register.
 *
 *	MachGetCauseReg(void)
 *
 * Results:
 *	current value of Cause register.None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(MachGetCauseReg)
	mfc0	v0, MIPS_COP_0_CAUSE_REG
	j	ra
	nop
END(MachGetCauseReg)
#endif

/*----------------------------------------------------------------------------
 *
 * switchfpregs --
 *
 *	Save the current state into 'from' and restore it from 'to'.
 *
 *	switchfpregs(from, to)
 *		struct proc *from;
 *		struct proc *to;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(switchfpregs)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# Disable interrupt and
	li	t0, MIPS_SR_COP_1_BIT		# enable the coprocessor
	mtc0	t0, MIPS_COP_0_STATUS_REG	#   old SR is saved in v0

	beq	a0, zero, 1f			# skip save if NULL pointer
	nop
/*
 * First read out the status register to make sure that all FP operations
 * have completed.
 */
	lw	a0, P_ADDR(a0)			# get pointer to pcb for proc
	cfc1	t0, MIPS_FPU_CSR		# stall til FP done
	cfc1	t0, MIPS_FPU_CSR		# now get status
	lw	t1, U_PCB_REGS+(SR * 4)(a0)	# get CPU status register
	li	t2, ~MIPS_SR_COP_1_BIT
	and	t1, t1, t2			# clear COP_1 enable bit
	sw	t1, U_PCB_REGS+(SR * 4)(a0)	# save new status register
/*
 * Save the floating point registers.
 */
	sw	t0, U_PCB_FPREGS+(32 * 4)(a0)	# save FP status
	swc1	$f0, U_PCB_FPREGS+(0 * 4)(a0)
	swc1	$f1, U_PCB_FPREGS+(1 * 4)(a0)
	swc1	$f2, U_PCB_FPREGS+(2 * 4)(a0)
	swc1	$f3, U_PCB_FPREGS+(3 * 4)(a0)
	swc1	$f4, U_PCB_FPREGS+(4 * 4)(a0)
	swc1	$f5, U_PCB_FPREGS+(5 * 4)(a0)
	swc1	$f6, U_PCB_FPREGS+(6 * 4)(a0)
	swc1	$f7, U_PCB_FPREGS+(7 * 4)(a0)
	swc1	$f8, U_PCB_FPREGS+(8 * 4)(a0)
	swc1	$f9, U_PCB_FPREGS+(9 * 4)(a0)
	swc1	$f10, U_PCB_FPREGS+(10 * 4)(a0)
	swc1	$f11, U_PCB_FPREGS+(11 * 4)(a0)
	swc1	$f12, U_PCB_FPREGS+(12 * 4)(a0)
	swc1	$f13, U_PCB_FPREGS+(13 * 4)(a0)
	swc1	$f14, U_PCB_FPREGS+(14 * 4)(a0)
	swc1	$f15, U_PCB_FPREGS+(15 * 4)(a0)
	swc1	$f16, U_PCB_FPREGS+(16 * 4)(a0)
	swc1	$f17, U_PCB_FPREGS+(17 * 4)(a0)
	swc1	$f18, U_PCB_FPREGS+(18 * 4)(a0)
	swc1	$f19, U_PCB_FPREGS+(19 * 4)(a0)
	swc1	$f20, U_PCB_FPREGS+(20 * 4)(a0)
	swc1	$f21, U_PCB_FPREGS+(21 * 4)(a0)
	swc1	$f22, U_PCB_FPREGS+(22 * 4)(a0)
	swc1	$f23, U_PCB_FPREGS+(23 * 4)(a0)
	swc1	$f24, U_PCB_FPREGS+(24 * 4)(a0)
	swc1	$f25, U_PCB_FPREGS+(25 * 4)(a0)
	swc1	$f26, U_PCB_FPREGS+(26 * 4)(a0)
	swc1	$f27, U_PCB_FPREGS+(27 * 4)(a0)
	swc1	$f28, U_PCB_FPREGS+(28 * 4)(a0)
	swc1	$f29, U_PCB_FPREGS+(29 * 4)(a0)
	swc1	$f30, U_PCB_FPREGS+(30 * 4)(a0)
	swc1	$f31, U_PCB_FPREGS+(31 * 4)(a0)

1:
/*
 *  Restore the floating point registers.
 */
	lw	a1, P_ADDR(a1)
	nop
	lw	t0, U_PCB_FPREGS+(32 * 4)(a1)	# get status register
	lwc1	$f0, U_PCB_FPREGS+(0 * 4)(a1)
	lwc1	$f1, U_PCB_FPREGS+(1 * 4)(a1)
	lwc1	$f2, U_PCB_FPREGS+(2 * 4)(a1)
	lwc1	$f3, U_PCB_FPREGS+(3 * 4)(a1)
	lwc1	$f4, U_PCB_FPREGS+(4 * 4)(a1)
	lwc1	$f5, U_PCB_FPREGS+(5 * 4)(a1)
	lwc1	$f6, U_PCB_FPREGS+(6 * 4)(a1)
	lwc1	$f7, U_PCB_FPREGS+(7 * 4)(a1)
	lwc1	$f8, U_PCB_FPREGS+(8 * 4)(a1)
	lwc1	$f9, U_PCB_FPREGS+(9 * 4)(a1)
	lwc1	$f10, U_PCB_FPREGS+(10 * 4)(a1)
	lwc1	$f11, U_PCB_FPREGS+(11 * 4)(a1)
	lwc1	$f12, U_PCB_FPREGS+(12 * 4)(a1)
	lwc1	$f13, U_PCB_FPREGS+(13 * 4)(a1)
	lwc1	$f14, U_PCB_FPREGS+(14 * 4)(a1)
	lwc1	$f15, U_PCB_FPREGS+(15 * 4)(a1)
	lwc1	$f16, U_PCB_FPREGS+(16 * 4)(a1)
	lwc1	$f17, U_PCB_FPREGS+(17 * 4)(a1)
	lwc1	$f18, U_PCB_FPREGS+(18 * 4)(a1)
	lwc1	$f19, U_PCB_FPREGS+(19 * 4)(a1)
	lwc1	$f20, U_PCB_FPREGS+(20 * 4)(a1)
	lwc1	$f21, U_PCB_FPREGS+(21 * 4)(a1)
	lwc1	$f22, U_PCB_FPREGS+(22 * 4)(a1)
	lwc1	$f23, U_PCB_FPREGS+(23 * 4)(a1)
	lwc1	$f24, U_PCB_FPREGS+(24 * 4)(a1)
	lwc1	$f25, U_PCB_FPREGS+(25 * 4)(a1)
	lwc1	$f26, U_PCB_FPREGS+(26 * 4)(a1)
	lwc1	$f27, U_PCB_FPREGS+(27 * 4)(a1)
	lwc1	$f28, U_PCB_FPREGS+(28 * 4)(a1)
	lwc1	$f29, U_PCB_FPREGS+(29 * 4)(a1)
	lwc1	$f30, U_PCB_FPREGS+(30 * 4)(a1)
	lwc1	$f31, U_PCB_FPREGS+(31 * 4)(a1)

	and	t0, t0, ~MIPS_FPU_EXCEPTION_BITS
	ctc1	t0, MIPS_FPU_CSR
	nop

	j	ra
	mtc0	v0, MIPS_COP_0_STATUS_REG	# Restore the status register.
END(switchfpregs)

/*----------------------------------------------------------------------------
 *
 * savefpregs --
 *
 *	Save the current floating point coprocessor state.
 *
 *	savefpregs(p)
 *		struct proc *p;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	fpcurproc is cleared.
 *
 *----------------------------------------------------------------------------
 */
LEAF(savefpregs)
	mfc0	v0, MIPS_COP_0_STATUS_REG	# Disable interrupts and
	li	t0, MIPS_SR_COP_1_BIT		#  enable the coprocessor
	mtc0	t0, MIPS_COP_0_STATUS_REG
	sw	zero, _C_LABEL(fpcurproc)	# indicate state has been saved
/*
 * First read out the status register to make sure that all FP operations
 * have completed.
 */
	lw	a0, P_ADDR(a0)			# get pointer to pcb for proc
	cfc1	t0, MIPS_FPU_CSR		# stall til FP done
	cfc1	t0, MIPS_FPU_CSR		# now get status
	lw	t1, U_PCB_REGS+(SR * 4)(a0)	# get CPU status register
	li	t2, ~MIPS_SR_COP_1_BIT
	and	t1, t1, t2			# clear COP_1 enable bit
	sw	t1, U_PCB_REGS+(SR * 4)(a0)	# save new status register
/*
 * Save the floating point registers.
 */
	sw	t0, U_PCB_FPREGS+(32 * 4)(a0)	# save FP status
	swc1	$f0, U_PCB_FPREGS+(0 * 4)(a0)
	swc1	$f1, U_PCB_FPREGS+(1 * 4)(a0)
	swc1	$f2, U_PCB_FPREGS+(2 * 4)(a0)
	swc1	$f3, U_PCB_FPREGS+(3 * 4)(a0)
	swc1	$f4, U_PCB_FPREGS+(4 * 4)(a0)
	swc1	$f5, U_PCB_FPREGS+(5 * 4)(a0)
	swc1	$f6, U_PCB_FPREGS+(6 * 4)(a0)
	swc1	$f7, U_PCB_FPREGS+(7 * 4)(a0)
	swc1	$f8, U_PCB_FPREGS+(8 * 4)(a0)
	swc1	$f9, U_PCB_FPREGS+(9 * 4)(a0)
	swc1	$f10, U_PCB_FPREGS+(10 * 4)(a0)
	swc1	$f11, U_PCB_FPREGS+(11 * 4)(a0)
	swc1	$f12, U_PCB_FPREGS+(12 * 4)(a0)
	swc1	$f13, U_PCB_FPREGS+(13 * 4)(a0)
	swc1	$f14, U_PCB_FPREGS+(14 * 4)(a0)
	swc1	$f15, U_PCB_FPREGS+(15 * 4)(a0)
	swc1	$f16, U_PCB_FPREGS+(16 * 4)(a0)
	swc1	$f17, U_PCB_FPREGS+(17 * 4)(a0)
	swc1	$f18, U_PCB_FPREGS+(18 * 4)(a0)
	swc1	$f19, U_PCB_FPREGS+(19 * 4)(a0)
	swc1	$f20, U_PCB_FPREGS+(20 * 4)(a0)
	swc1	$f21, U_PCB_FPREGS+(21 * 4)(a0)
	swc1	$f22, U_PCB_FPREGS+(22 * 4)(a0)
	swc1	$f23, U_PCB_FPREGS+(23 * 4)(a0)
	swc1	$f24, U_PCB_FPREGS+(24 * 4)(a0)
	swc1	$f25, U_PCB_FPREGS+(25 * 4)(a0)
	swc1	$f26, U_PCB_FPREGS+(26 * 4)(a0)
	swc1	$f27, U_PCB_FPREGS+(27 * 4)(a0)
	swc1	$f28, U_PCB_FPREGS+(28 * 4)(a0)
	swc1	$f29, U_PCB_FPREGS+(29 * 4)(a0)
	swc1	$f30, U_PCB_FPREGS+(30 * 4)(a0)
	swc1	$f31, U_PCB_FPREGS+(31 * 4)(a0)

	j	ra
	mtc0	v0, MIPS_COP_0_STATUS_REG	# Restore the status register.
END(savefpregs)

/*----------------------------------------------------------------------------
 *
 * MachFPInterrupt --
 * MachFPTrap --
 *
 *	Handle a floating point interrupt (r3k) or trap (r4k).
 *	the handlers  are indentical, only the reporting mechanisms differ.
 *
 *	MachFPInterrupt(status, cause, pc, frame)
 *		unsigned status;
 *		unsigned cause;
 *		unsigned pc;
 *		int *frame;
 *
 *	MachFPTrap(status, cause, pc, frame)
 *		unsigned status;
 *		unsigned cause;
 *		unsigned pc;
 *		int *frame;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NON_LEAF(MachFPInterrupt, STAND_FRAME_SIZE, ra)
	# XXX should use ANONLEAF (or ANESTED) instead of ALEAF.
ALEAF(MachFPTrap)

	subu	sp, sp, STAND_FRAME_SIZE
	mfc0	t0, MIPS_COP_0_STATUS_REG
	sw	ra, STAND_RA_OFFSET(sp)
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

	or	t0, t0, MIPS_SR_COP_1_BIT
	mtc0	t0, MIPS_COP_0_STATUS_REG
	nop
	nop
	nop				# 1st extra nop for r4k
	nop				# 2nd extra nop for r4k

	cfc1	t0, MIPS_FPU_CSR	# stall til FP done
	cfc1	t0, MIPS_FPU_CSR	# now get status
	nop
	sll	t2, t0, (31 - 17)	# unimplemented operation?
	bgez	t2, 3f			# no, normal trap
	nop
/*
 * We got an unimplemented operation trap so
 * fetch the instruction, compute the next PC and emulate the instruction.
 */
	bgez	a1, 1f			# Check the branch delay bit.
	nop
/*
 * The instruction is in the branch delay slot so the branch will have to
 * be emulated to get the resulting PC.
 */
	sw	a2, STAND_FRAME_SIZE + 8(sp)
	sw	a3, STAND_FRAME_SIZE + 12(sp)
	move	a0, a3				# 1st arg is p. to trapframe
	move	a1, a2				# 2nd arg is instruction PC
	move	a2, t0				# 3rd arg is FP CSR
	jal	_C_LABEL(MachEmulateBranch)	# compute PC after branch
	move	a3, zero			# 4th arg is FALSE
/*
 * Now load the floating-point instruction in the branch delay slot
 * to be emulated.
 */
	lw	a2, STAND_FRAME_SIZE + 8(sp)	# restore EXC pc
	lw	a3, STAND_FRAME_SIZE + 12(sp)
	b	2f
	lw	a0, 4(a2)			# a0 = coproc instruction
/*
 * This is not in the branch delay slot so calculate the resulting
 * PC (epc + 4) into v0 and continue to MachEmulateFP().
 */
1:
	lw	a0, 0(a2)			# a0 = coproc instruction
	addu	v0, a2, 4			# v0 = next pc
2:
	sw	v0, (PC * 4)(a3)		# save new pc
/*
 * Check to see if the instruction to be emulated is a floating-point
 * instruction.
 */
	srl	t0, a0, MIPS_OPCODE_SHIFT
	beq	t0, MIPS_OPCODE_C1, 4f		# this should never fail
	nop
/*
 * Send a floating point exception signal to the current process.
 */
3:
	lw	a0, _C_LABEL(curproc)		# get current process
	cfc1	a2, MIPS_FPU_CSR		# code = FP execptions
	ctc1	zero, MIPS_FPU_CSR		# Clear exceptions
	jal	_C_LABEL(trapsignal)
	li	a1, SIGFPE
	b	FPReturn
	nop

/*
 * Finally, we can call MachEmulateFP() where a0 is the instruction to emulate.
 */
4:
	jal	_C_LABEL(MachEmulateFP)
	nop

/*
 * Turn off the floating point coprocessor and return.
 */
FPReturn:
	mfc0	t0, MIPS_COP_0_STATUS_REG
	lw	ra, STAND_RA_OFFSET(sp)
	and	t0, t0, ~MIPS_SR_COP_1_BIT
	mtc0	t0, MIPS_COP_0_STATUS_REG
	j	ra
	addu	sp, sp, STAND_FRAME_SIZE
END(MachFPInterrupt)

#ifdef MDB
/*
 * Trap into the debugger.
 *
L* void
L* mdbpanic()
L* {
L* }
 */
LEAF(mdbpanic)
	break	MIPS_BREAK_SOVER_VAL
	j	ra
	nop
END(mdbpanic)

#endif	/* MDB */

#if defined(DEBUG) || defined(MDB) || defined(DDB)
/*
 * Stacktrace support hooks which use type punnign to access
 * the caller's registers.
 */


/*
 * stacktrace() -- print a stack backtrace to the console. 
 *	implicitly accesses caller's a0-a3.
 */
NON_LEAF(stacktrace, STAND_FRAME_SIZE+20, ra)

	subu	sp, sp, STAND_FRAME_SIZE+20	# four arg-passing slots

	move	t0, ra				# save caller's PC
	addu	t1, sp, STAND_FRAME_SIZE+20	# compute caller's SP
	move	t2, s8				# non-virtual frame pointer

	la	v0, _C_LABEL(printf)

	sw	ra, 36(sp)			# save return address

	/* a0-a3 are still caller's a0-a3, pass in-place as given. */
	sw	t0, 16(sp)			#   push caller's PC
	sw	t1, 20(sp)			#   push caller's SP
	sw	t2, 24(sp)			#   push caller's FP, in case
	sw	zero, 28(sp)			#   caller's RA on stack
	jal	_C_LABEL(stacktrace_subr)
	sw	v0, 32(sp)			#   push printf

	lw	ra, 36(sp)
	addu	sp, sp, STAND_FRAME_SIZE+20
	j	ra
	nop
END(stacktrace)


/*
 * logstacktrace() -- log a stack traceback to msgbuf.
 *	implicitly accesses caller's a0-a3.
 */
NON_LEAF(logstacktrace, STAND_FRAME_SIZE+20, ra)

	subu	sp, sp, STAND_FRAME_SIZE+20	# four arg-passing slots

	move	t0, ra				# save caller's PC
	addu	t1, sp, STAND_FRAME_SIZE+20	# compute caller's SP
	move	t2, s8				# non-virtual frame pointer

	la	v0, _C_LABEL(printf)

	sw	ra, 36(sp)			# save return address

	/* a0-a3 are still caller's a0-a3, pass in-place as given. */
	sw	t0, 16(sp)			#   push caller's PC
	sw	t1, 20(sp)			#   push caller's SP
	sw	t2, 24(sp)			#   push caller's FP, in case
	sw	zero, 28(sp)			#   RA on stack
	jal	_C_LABEL(stacktrace_subr)
	sw	v0, 32(sp)			#   push printf

	lw	ra, 36(sp)
	addu	sp, sp, STAND_FRAME_SIZE+20
	j	ra
	nop
END(logstacktrace)

#endif	/* DEBUG */


/*
 * The variables below are used to communicate the CPU and FPU
 * version/revision level to  higher-level software.
 * XXX  will need reworking for  SMP.
 */
	.data
	.globl	_C_LABEL(esym)
_C_LABEL(esym):
	.word 0

	.globl	_C_LABEL(cpu_id)
	.globl	_C_LABEL(fpu_id)
	.globl	_C_LABEL(cpu_arch)
_C_LABEL(cpu_id):
	.word	0
_C_LABEL(fpu_id):
	.word	0
_C_LABEL(cpu_arch):
	.word	0

	.globl	_C_LABEL(mips_L1DataCacheSize)
_C_LABEL(mips_L1DataCacheSize):
	.word	0

.globl	_C_LABEL(mips_L1InstCacheSize)
_C_LABEL(mips_L1InstCacheSize):
	.word	0

	.globl	_C_LABEL(mips_L1DataCacheLSize)
_C_LABEL(mips_L1DataCacheLSize):
	.word	0

	.globl	_C_LABEL(mips_L1InstCacheLSize)
_C_LABEL(mips_L1InstCacheLSize):
	.word	0

	.globl	_C_LABEL(mips_CacheAliasMask)
_C_LABEL(mips_CacheAliasMask):
	.word 0

	.globl	_C_LABEL(mips_L2CacheSize)
_C_LABEL(mips_L2CacheSize):
	.word	0

	.globl	_C_LABEL(mips_L2CacheLSize)
_C_LABEL(mips_L2CacheLSize):
	.word	0

/*
 * The variables below are used to communicate the CPU and FPU
 * version/revision level to  higher-level software.
 * XXX  will need reworking for  SMP.
 */


/*
 * Port-specific locore code moved to sys/arch/<port>/<port>/locore_machdep.S
 */
