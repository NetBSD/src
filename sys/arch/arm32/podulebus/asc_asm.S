/* $NetBSD: asc_asm.S,v 1.1 1996/01/31 23:25:29 mark Exp $ */

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * asc_asm.S
 *
 * Assembly routines for the Acorn SCSI card driver
 *
 * Created      : 01/07/95
 * Last updated : 01/07/95
 *
 *    $Id: asc_asm.S,v 1.1 1996/01/31 23:25:29 mark Exp $
 */

#include "assym.s"
#include <arm32/podulebus/ascreg.h>

#define	SCSI_DATA_IN	0x0800	/* expect data to come INTO memory	*/
#define	SCSI_DATA_OUT	0x1000	/* expect data to flow OUT of memory	*/

sp	.req	13
lr	.req	14
pc	.req	15

	.text
	.align	0

/*
 * asc_asm_tx_data
 *
 * On entry :
 * r0	- sbic_base
 * r1	- txaddr
 * r2	- txlen
 * r3	- rw
 *
 * On exit:
 * r0	- txlen left
 */

	.global	_asc_asm_tx_data
_asc_asm_tx_data:
	mov	ip, sp
	stmfd	sp!, {r4, r5, r6, fp, ip, lr, pc}
	sub	fp, ip, #4

	tst	r3, #SCSI_DATA_OUT	/* Data out xfer ? */
	bne	asc_data_out
	tst	r3, #SCSI_DATA_IN	/* Data in xfer ? */
	bne	asc_data_in

	mvn	r0, #0			/* Exit -ve indicating error */

	ldmea	fp, {r4, r5, r6, fp, sp, pc}

asc_data_out:
/* sc_sbic_base = offset 0x80 (This is just for my reference) */
/* r4 - scratch */

/* Select and transfer with ATN */
	mov	r4, #SBIC_COMMAND
	strb	r4, [r0, #SBIC_ADDRREG]
	mov	r4, #SBIC_Sel_tx_wATN
	strb	r4, [r0, #SBIC_DATAREG]

	cmp	r2, #0x00000000
	ble	asc_data_out_complete

/* r5 - address of SBIC data register */

	mov	r5, #SBIC_DATA

/* r4 - holds aux status */
/* r6 - scratch (holds byte being transferred) */

asc_data_out_loop:
	ldrb	r4, [r0, #SBIC_AUX_STATUS]
/*	tst	r4, #SBIC_ASR_INT
	bne	asc_data_out_complete

	tst	r4, #SBIC_ASR_DBR
	beq	asc_data_out_loop

	ldrb	r6, [r1], #0x0001
	strb	r5, [r0, #SBIC_ADDRREG]
	strb	r6, [r0, #SBIC_DATAREG]

*/

	tst	r4, #SBIC_ASR_DBR

	ldrneb	r6, [r1], #0x0001
	strneb	r5, [r0, #SBIC_ADDRREG]
	strneb	r6, [r0, #SBIC_DATAREG]

	tst	r4, #SBIC_ASR_INT
	bne	asc_data_out_complete

	subs	r2, r2, #0x00000001
	bne	asc_data_out_loop
	
asc_data_out_complete:
	mov	r0, r2		/* Exit returning bytes left */

	ldmea	fp, {r4, r5, r6, fp, sp, pc}

asc_data_in:
/* r4 - scratch */

/* Select and transfer with ATN */
	mov	r4, #SBIC_COMMAND
	strb	r4, [r0, #SBIC_ADDRREG]
	mov	r4, #SBIC_Sel_tx_wATN
	strb	r4, [r0, #SBIC_DATAREG]

	cmp	r2, #0x00000000
	ble	asc_data_in_complete

/* r5 - address of SBIC data register */

	mov	r5, #SBIC_DATA

/* r4 - hold aux status */
/* r6 - scratch (holds byte being transferred) */

asc_data_in_loop:
	ldrb	r4, [r0, #SBIC_AUX_STATUS]
/*	tst	r4, #SBIC_ASR_INT
	bne	asc_data_in_complete

	tst	r4, #SBIC_ASR_DBR
	beq	asc_data_in_loop

	strb	r5, [r0, #SBIC_ADDRREG]
	ldrb	r6, [r0, #SBIC_DATAREG]
	strb	r6, [r1], #0x0001
*/
	tst	r4, #SBIC_ASR_DBR

	strneb	r5, [r0, #SBIC_ADDRREG]
	ldrneb	r6, [r0, #SBIC_DATAREG]
	strneb	r6, [r1], #0x0001

	tst	r4, #SBIC_ASR_INT
	bne	asc_data_in_complete

	subs	r2, r2, #0x00000001
	bne	asc_data_in_loop

asc_data_in_complete:
	mov	r0, r2		/* Exit returning bytes left */

	ldmea	fp, {r4, r5, r6, fp, sp, pc}
