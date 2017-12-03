/* $NetBSD: start.S,v 1.1.1.1.6.2 2017/12/03 11:36:11 jdolecek Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define STACK_SIZE 8192

#include "include/asm.h"


	  .bss

	  .comm stack_bottom,STACK_SIZE

	.text

LEAF(start)

	la	gp,_gp

	la	sp,stack_bottom+STACK_SIZE-32

/*
 * On entry: a0 = the firmware handle, a2 = firmware's
 * for secondary bootstraps, a1 is the booted device handle
 * Don't trash a0..a3 until main is called!
 */


#if defined(STANDALONE_PROGRAM)
	/*
	 * This should not be used anywhere.
	 */

#error "don't do this."

#else /* defined(STANDALONE_PROGRAM) */

#if !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK)

        la      sp,start

#endif /* !defined(PRIMARY_BOOTBLOCK) && !defined(UNIFIED_BOOTBLOCK) */

#endif /* defined(STANDALONE_PROGRAM) */

/*
 * Zero BSS 
 */

	la	t0,_edata		/* t0 = address */
	la	t1,_end

1:	sw	zero,0(t0)
	sw	zero,4(t0)
	sw	zero,8(t0)
	sw	zero,16(t0)
	add	t0,16
	ble	t0,t1,1b


/*
 * Run the main program.  
 */

	jal     main			/* transfer to C */

XLEAF(halt)
	li	t0,0xBFC00000		/* transfer back to firmware */
	j	t0
END(start)



LEAF(__main)
	j	ra
END(__main)

LEAF(_rtt)
	li	t0,0xBFC00000
	j	t0
END(_rtt)
