| Copyright (c) 1993 Paul Mackerras.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions
| are met:
| 1. Redistributions of source code must retain the above copyright
|    notice, this list of conditions and the following disclaimer.
| 2. Redistributions in binary form must reproduce the above copyright
|    notice, this list of conditions and the following disclaimer in the
|    documentation and/or other materials provided with the distribution.
| 3. The name of the author may not be used to endorse or promote products
|    derived from this software withough specific prior written permission
|
| THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
| IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
| OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
| IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
| NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
| DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
| THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
| THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
|	$Id: sias.s,v 1.1 1994/02/22 23:50:40 paulus Exp $

|
| Assembly-lang buffer R/W for SCSI
| si{get,put} ptr,nb
| return the number of bytes xferred

	.globl	_siget
_siget:	link	a6,#0
	movel	a2,a7@-
	movel	a6@(8),a1	| buffer
	movel	a6@(12),d0	| # bytes
	beqs	9f
	movel	d0,d1
	subql	#1,d1
	movel	#0xe98000,a0	| SBIC ptr
	lea	a0@(2),a2
	moveb	#0x19,a0@
1:	btst	#0,a0@
	beqs	3f
	moveb	a2@,a1@+
	dbra	d1,1b
3:	addqw	#1,d1
	subl	d1,d0
	movel	a7@,a2
9:	unlk	a6
	rts

	.globl	_siput
_siput:	link	a6,#0
	movel	a2,a7@-
	movel	a6@(8),a1	| buffer
	movel	a6@(12),d0	| # bytes
	beqs	9f
	movel	d0,d1
	subql	#1,d1
	movel	#0xe98000,a0	| SBIC ptr
	lea	a0@(2),a2
	moveb	#0x19,a0@
1:	btst	#0,a0@
	beqs	3f
	moveb	a1@+,a2@
2:	dbra	d1,1b
3:	addqw	#1,d1
	subl	d1,d0
	movel	a7@,a2
9:	unlk	a6
	rts
