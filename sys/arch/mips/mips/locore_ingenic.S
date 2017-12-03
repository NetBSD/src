/*	$NetBSD: locore_ingenic.S,v 1.1.10.2 2017/12/03 11:36:28 jdolecek Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <mips/asm.h>
RCSID("$NetBSD: locore_ingenic.S,v 1.1.10.2 2017/12/03 11:36:28 jdolecek Exp $")

#include <mips/cpuregs.h>
#include <mips/ingenic/ingenic_coreregs.h>

#include "assym.h"

	.set	noreorder
	.set	noat
	.set	arch=mips32r2

	.text

/*
 * uint32_t mips_cp0_corectrl_read(void)
 *
 *      Return the current value of the CP0 Core Control register.
 */
LEAF(mips_cp0_corectrl_read)
        mfc0    v0, CP0_CORE_CTRL
        jr      ra
         nop
END(mips_cp0_corectrl_read)

/*
 * void mips_cp0_corectrl_write(uint32_t)
 *
 *      Set the value of the CP0 Core Control register.
 */
LEAF(mips_cp0_corectrl_write)
        mtc0    a0, CP0_CORE_CTRL
	jr.hb	ra
         nop
END(mips_cp0_corectrl_write)

/*
 * uint32_t mips_cp0_corestatus_read(void)
 *
 *      Return the current value of the CP0 Core Status register.
 */
LEAF(mips_cp0_corestatus_read)
        mfc0    v0, CP0_CORE_STATUS
        jr      ra
         nop
END(mips_cp0_corestatus_read)

/*
 * void mips_cp0_corestatus_write(uint32_t)
 *
 *      Set the value of the CP0 Core Status register.
 */
LEAF(mips_cp0_corestatus_write)
        mtc0    a0, CP0_CORE_STATUS
	jr.hb	ra
         nop
END(mips_cp0_corestatus_write)


/*
 * uint32_t mips_cp0_corereim_read(void)
 *
 *      Return the current value of the CP0 Reset Entry & IRQ Mask register.
 */
LEAF(mips_cp0_corereim_read)
        mfc0    v0, CP0_CORE_REIM
        jr      ra
         nop
END(mips_cp0_corereim_read)

/*
 * void mips_cp0_corereim_write(uint32_t)
 *
 *      Set the value of the CP0 Core Reset Entry & IRQ Mask register.
 */
LEAF(mips_cp0_corereim_write)
        mtc0    a0, CP0_CORE_REIM
	jr.hb	ra
         nop
END(mips_cp0_corereim_write)


/*
 * uintptr_t mips_cp0_corembox_read(u_int sel)
 *
 *	Return the current value of the selected CP0 Mailbox register.
 */
LEAF(mips_cp0_corembox_read)
	sll	a0, 2
	PTR_LA	t9, 1f
	PTR_ADDU t9, a0
	jr	t9
	 nop
1:
	jr	ra
	 mfc0	v0, CP0_CORE0_MBOX
	jr	ra
	 mfc0	v0, CP0_CORE1_MBOX
	jr	ra
END(mips_cp0_corembox_read)

/*
 * void mips_cp0_watchlo_write(u_int sel, uinte32_t val)
 *
 *	Set the current value of the selected CP0 Mailbox register.
 */
LEAF(mips_cp0_corembox_write)
	sll	a0, 2
	PTR_LA	t9, 1f
	PTR_ADDU t9, a0
	jr	t9
	 nop
1:
	jr.hb	ra
	 mtc0	a1, CP0_CORE0_MBOX
	jr.hb	ra
	 mtc0	a1, CP0_CORE1_MBOX
	jr.hb	ra
END(mips_cp0_corembox_write)
