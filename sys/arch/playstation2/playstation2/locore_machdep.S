/*	$NetBSD: locore_machdep.S,v 1.1 2001/10/16 15:38:55 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include "opt_builtin_kernel_loader.h"

#include <mips/asm.h>
#include <mips/cpuregs.h>

#define D_STAT_REG	0xb000e010
#define I_MASK_REG	0xb000f010

	.set	noreorder
	.text
	.align 6	/* align cache line size (64B) */
	
/*
 * _intr_resume, _intr_suspend:	
 *	 Fast interrupt locking functions.
 */	
LEAF_NOPROFILE(_intr_resume)
	beqz	a0, 1f
	nop
	ei
1:	
	j	ra
	 nop
END(_intr_resume)

LEAF_NOPROFILE(_intr_suspend)
	mfc0	v0, MIPS_COP_0_STATUS_REG	
	lui	t0, 0x1		/* EIE bit */
	and	v0, v0, t0
1:	di
	sync.p
	mfc0	t1, MIPS_COP_0_STATUS_REG
	and	t2, t0, t1
	beq	t2, t0, 1b
	 nop
	j	ra
	 nop
END(_intr_suspend)

/*
 * md_imask_update:
 *	this function may be called under EXL=1. don't access KSEG2.
 *	
 * void
 * md_imask_update()
 * {
 * 	register u_int32_t cur_mask, bit;
 * 
 * 	cur_mask = (_reg_read_4(I_MASK_REG) & 0xffff) |
 *	    (_reg_read_4(D_STAT_REG) & 0xffff0000);
 *
 *	bit = (cur_mask ^ ~md_imask) | (cur_mask & md_imask);
 *
 * 	_reg_write_4(I_MASK_REG, bit & __intc_enabled_channel);
 * 	_reg_write_4(D_STAT_REG, bit & D_STAT_CIM(__dmac_enabled_channel));
 * }
 */	 	
LEAF_NOPROFILE(md_imask_update)	
	lw	a0, _C_LABEL(__intc_enabled_channel)
	li	a1, I_MASK_REG
	lw	a3, _C_LABEL(__dmac_enabled_channel)
	li	a2, D_STAT_REG	
	lw	t1, 0(a2)	/* D_STAT */
	lui	t2, 0xffff	
	lw	t0, 0(a1)	/* I_MASK */
	sll	a3, a3, 16
	and	t1, t1, t2
	lw	t3, _C_LABEL(md_imask)
	nor	t2, zero, t2
	and	t0, t0, t2
	or	t0, t0, t1	/* cur_mask */
	nor	t4, zero, t3
	xor	t4, t4, t0	
	and	t3, t3, t0
	or	t0, t3, t4
	and	t3, t0, a0	/* INTC */
	and	t4, t0, a3	/* DMAC */
	sw	t3, 0(a1)
	sw	t4, 0(a2)
	sync.p	
	j	ra
	nop
END(md_imask_update)		

/*	
 * spllowersofthigh:
 *	this function may be called under EXL=1. don't access KSEG2.	
 * void
 * spllowersofthigh()
 * {
 * 	extern void _splnone(void);
 * 
 * 	md_imask = __icu_mask[IPL_SOFTSERIAL];
 * 	md_imask_update(); 
 * 	_splnone();
 * }
 */	
NESTED_NOPROFILE(spllowersofthigh, CALLFRAME_SIZ, ra)
	.mask	0x80000000, -4
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	la	a0, _C_LABEL(__icu_mask)
	lw	a0, 16(a0)
	sw	a0, _C_LABEL(md_imask)
	jal	md_imask_update
	nop
	lw	ra, CALLFRAME_RA(sp)	
	li	t0, (MIPS_INT_MASK | MIPS_SR_INT_IE)	
	mtc0	t0, MIPS_COP_0_STATUS
	sync.p
	j	ra
	addu	sp, sp, CALLFRAME_SIZ
END(spllowersofthigh)

#ifdef BUILTIN_KERNEL_LOADER	
/*
 * kloader_boot(vaddr_t entry_addr, struct kloader_page_tag *p)	
 *	must be PIC.
 */
EXPORT(kloader_boot_start)	
LEAF_NOPROFILE(kloader_boot)
	di			# disable cache
	/*
	 * 1. load kernel image.
	 */
	move	t6, a1		# p
1:
	beqz	t6, 3f
	 move	t7, t6
	lw	t6, 0(t7)	# p = next
	lw	t0, 4(t7)	# src
	lw	t4, 8(t7)	# dst
	lw	t2, 12(t7)	# sz
	addu	t5, t4, t2	# dst + sz
2:
	lw	t3, 0(t0)	# copy
	sw	t3, 0(t4)
	addiu	t4, t4, 4
	addiu	t0, t0, 4
	bltu	t4, t5, 2b
	 nop
	b	1b
	 nop
3:
	nop

	/* 
	 * 2. Cache flush
	 */
	li	t1, R5900_C_SIZE_I
	li	t2, R5900_C_SIZE_D

	/*
	 * Flush the data cache.
	 */
	li	t0, MIPS_KSEG0_START
	srl	t2, 1		# Two way set assoc
	addu	t1, t0, t2	# End address
	sync.l
	sync.p
1:
	# line +0
	cache	R5900_C_IWBINV_D,  0(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D,  1(t0);sync.l;sync.p	# way 1
	# line +1
	cache	R5900_C_IWBINV_D, 64(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D, 65(t0);sync.l;sync.p	# way 1
	# line +2
	cache	R5900_C_IWBINV_D,128(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D,129(t0);sync.l;sync.p	# way 1
	# line +3
	cache	R5900_C_IWBINV_D,192(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D,193(t0);sync.l;sync.p	# way 1
	addu	t0, t0, 256	
	bne	t0, t1, 1b
	 nop
	
	/*
	 * Flush the instruction cache.
	 */
	li	t0, MIPS_KSEG0_START
	srl	t1, 1			# Two way set assoc
	addu	t1, t0, t1		# End address
	sync.l
	sync.p
1:
	#	[12:6]	... line
	#	[0]	... way
	# line +0
	cache	R5900_C_IINV_I,  0(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I,  1(t0);sync.l;sync.p	# way 1
	# line +1	
	cache	R5900_C_IINV_I, 64(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I, 65(t0);sync.l;sync.p	# way 1
	# line +2
	cache	R5900_C_IINV_I,128(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I,129(t0);sync.l;sync.p	# way 1
	# line +3	
	cache	R5900_C_IINV_I,192(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I,193(t0);sync.l;sync.p	# way 1
	addu	t0, t0, 256	
	bne	t0, t1, 1b
	 nop

	/* 
	 * 3. jump to kernel entry 
	 */
	jr	a0
	nop
	/* NOTREACHED */
END(kloader_boot)
EXPORT(kloader_boot_end)	
	
#endif /* BUILTIN_KERNEL_LOADER */

/*
 * Unused. but for vmstat(8)
 */
	.data
	.align 6	/* align cache line size (64B) */	
	
	.globl _C_LABEL(intrcnt)
	.globl _C_LABEL(eintrcnt)
	.globl _C_LABEL(intrnames)
	.globl _C_LABEL(eintrnames)

_C_LABEL(intrnames):
_C_LABEL(eintrnames):
_C_LABEL(intrcnt):
_C_LABEL(eintrcnt):

