/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: locon.s,v 1.2 1994/08/16 23:47:29 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		
#include "vax/include/mtpr.h"

		.text

/******************************************************************************

	_conout(char* message);

	Routine to print message to console.

******************************************************************************/

		.globl	_consinit
		.globl	_conout

_conout:	.word   0x3
		movl	4(ap),r0
1:		tstb	(r0)		# Test for end of string.
		beql	3f		# If end, exit.
2:		mfpr	$ PR_TXCS,r1	# Wait until ready to Tx.
#		bbc	$7,r1,2b
		.long   0xf95107e1
		clrl	r1		# Need to make a 32 bit write with
		movb	(r0)+,r1	# all except 8 low bits cleared.
		mtpr	r1,$ PR_TXDB	# Send the character.
		brb	1b		# Loop for next character.
3:		ret

_consinit:	.word 0x0
		ret	# No init currently needed.

		.globl	_ocnputc
_ocnputc:	.word 0x3
		movl    4(ap),r0
4:              mfpr    $ PR_TXCS,r1
		bbc     $7,r1,4b
		mtpr    r0,$ PR_TXDB
		cmpb	r0,$10
		beql	5f
		ret
5:		movl	$13,r0
		brb	4b

	.data

		.space 512
temp_stack:

		.globl	_ocngetc
_ocngetc:	.word 0x2
6:		mfpr	$ PR_RXCS,r1
		bbc	$7,r1,6b
		mfpr	$ PR_RXDB,r0
		bicl2	$0xffffff80,r0
		ret

