|
|	copy binary image and execute
|
|	written by Yasha (ITOH Yasufumi)
|	public domain
|
|	$NetBSD: trampoline.S,v 1.2 2001/06/12 16:57:28 minoura Exp $

#include <machine/asm.h>
#include "trampoline.h"

|    arg+0		bsr trampoline
|	+4	%a3+0	temporary stack address
|	+8	%a3+4	processor type
|	+12	%a3+8	struct execkern_arg
|	+52		trampoline

ENTRY_NOPROFILE(trampoline)
	oriw	#0x0700,%sr		| keep out interrupts

	moveal	%sp@+,%a3
	moveal	%a3@+,%sp		| set temporary stack

	movel	%a3@+,%d2		| MPU type

	|
	| turn off MMU
	|
	moveq	#MPU_68030,%d0
	cmpl	%d2,%d0
	bccs	Lmmu030
	| 040/060
	moveq	#0,%d0
	.long	0x4E7B0003		| movec	%d0,%tc
	.long	0x4E7B0004		| movec	%d0,%itt0
	.long	0x4E7B0005		| movec	%d0,%itt1
	.long	0x4E7B0006		| movec	%d0,%dtt0
	.long	0x4E7B0007		| movec	%d0,%dtt1
	.long	0x4E7B0806		| movec	%d0,%urp
	.long	0x4E7B0807		| movec	%d0,%srp
	bras	Lmmudone
Lmmu030:
	| 020/030
	clrl	%sp@-
	.long	0xF0174000		| pmove	%sp@,%tc
	cmpl	%d0,%d2
	bnes	Lmmu020			| 68851 has no tt regs
	.long	0xF0170800		| pmove	%sp@,%tt0
	.long	0xF0170C00		| pmove	%sp@,%tt1
Lmmu020:
	movel	#0x7fff0001,%sp@-	| null root pointer
	.long	0xF0174C00		| pmove	%sp@,%crp
	.long	0xF0174800		| pmove	%sp@,%srp
	addql	#8,%sp
Lmmudone:

	| minimize supervisor protection
	clrb	AREA_SET_REG

	| reset VBR (for compatibility)
	subal	%a1,%a1
	.long	0x4E7B9801		| movec	%a1,%vbr

	| then transfer and exec kernel

#define XK_NO_C_INTERFACE		/* pass arg with %a3 */
#include "../common/execkern.S"

GLOBAL(end_trampoline)
