/*	Id: table.c,v 1.5 2014/12/27 21:18:19 ragge Exp 	*/	
/*	$NetBSD: table.c,v 1.1.1.1 2016/02/09 20:28:39 plunky Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * Copyright (c) 2014 Alan Cox (alan@lxorguk.ukuu.org.uk).
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
 * 3. The name of the author may not be used to endorse or promote products
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
 */


# include "pass2.h"

/*
 * TODO
 *  -	clean up TLL usage so we are ready for 64bit longlong (T32 and T64?)
 *  -	Clean up all the SHINT SHCH etc oddities for clarity
 *  -	Better optimisation of constant multiply/divide etc on 8086
 *  -	some how lose the final bogus sp adjust we generate
 *  -	write helper lib for long and longlong ops
 *  -	add an E class and virtual 4 register longlong model
 *  -   bcc fastcall format
 *
 *  -	How to do configurable code-gen
 *	For 80186 we want to use the better shifts, push constant, mul/div
 *	features and enter/leave
 *	For 80286 it's not clear we need to do anything for real mode. However
 *	for large data/huge models protected mode means minimising ds/es
 *	reloads with the same data
 *	With 80386 there is lots more - but who wants 80386/16bit optimised
 *	code !
 *
 *  -	FPU has not been tackled. FPU/FPU should be fine (barring the odd
 *	gnu style instruction forms), but FPU/other is quite different
 *	because the 4 byte conversions are now long not int and two register
 *	while 8 byte longlong ones are going to need some gymnastics once
 *	we support the virtual 64bit registers
 *
 *  -	FPU/noFPU option. Assuming FPU present on 8086 is a bit obnoxious
 *	as a default. Need to be able to generate entirely soft-float calls.
 *
 *  -   We can't currently do various conversions we'd like to have, notably
 *	things like  "and ax, $ff"  to convert AL into AX from uchar to uint
 *	(that needs core changes and also allocator changes to try and avoid
 *	high bits being occupied by other users)
 *
 *  -	Shifts should be optimised for 8/16/24/32 cases
 *
 *  -	No attempt is made to deal with larger models than tiny
 *
 *	small mode should work (just keep constant data in data segments)
 *
 *	large code/small data should be fine-ish. Really that implies 32bit
 *	void * but for most uses you want 32bit only to be function pointers
 *	even if its not to spec. Function entry/exit needs to allow for the
 *	extra 2 bytes, call becomes call far, call ptr becomes a bit trickier
 *	but not a lot. Not that ld86 can link large model yet!
 *
 *	large data is much harder because an address is 32bit, half of which
 *	needs to keep getting stuck into a segment register. However we often
 *	(usually) work with multiple pointers to the same object. Any given
 *	object has a single segment so we will badly need optimisations to
 *	"share" segment data and avoid lots of duplicate register uses and
 *	mov es, foo statements.
 *
 *	huge model makes all pointers 32bit and all objects 32bit sized. That
 *	probably makes the sharing es issue go away somewhat, but raises the
 *	ugly problems of pointer normalisation (00:10 is the same as 01:00
 *	which b*ggers up comparisons royally) and also protected mode where
 *	your segment jump as you go over 64K boundaries is different. On the
 *	bright side huge model performance will suck whatever the compiler
 *	does.
 *
 */

/* 64bit values */
# define T64 TLONGLONG|TULONGLONG
/* 32bit values */
# define T32 TLONG|TULONG
/* 16bit values */
# define T16 TINT|TUNSIGNED
/* 16bit values including pointers */
# define TP16 TINT|TUNSIGNED|TPOINT
/* 8bit values */
# define T8  TCHAR|TUCHAR

#define	 SHINT	SAREG	/* short and int */
#define	 ININT	INAREG
#define	 SHCH	SBREG	/* shape for char */
#define	 INCH	INBREG
#define	 SHLL	SCREG	/* shape for long long FIXME */
#define	 INLL	INCREG
#define	 SHL	SCREG	/* shape for long*/
#define	 INL	INCREG
#define	 SHFL	SDREG	/* shape for float/double */
#define	 INFL	INDREG	/* shape for float/double */

struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* PCONVs are usually not necessary */
{ PCONV,	INAREG,
	SAREG,	T16|TPOINT,
	SAREG,	T16|TPOINT,
		0,	RLEFT,
		"", },

/*
 * A bunch conversions of integral<->integral types
 * There are lots of them, first in table conversions to itself
 * and then conversions from each type to the others.
 */

/* itself to itself, including pointers */

/* convert (u)char to (u)char. */
{ SCONV,	INCH,
		SHCH,			T8,
		SHCH,			T8,
		0,			RLEFT,
		"", },

/* convert pointers to int. */
{ SCONV,	ININT,
		SHINT,			TPOINT|T16,
		SANY,			T16,
		0,			RLEFT,
		"", },

/* convert (u)long to (u)long. */
{ SCONV,	INL,
		SHL,			T32,
		SHL,			T32,
		0,			RLEFT,
		"", },

/* convert (u)long and (u)longlong to (u)longlong. */
{ SCONV,	INLL,
		SHLL,			T64,
		SHLL,			T64,
		0,			RLEFT,
		"", },

/* convert between float/double/long double. */
{ SCONV,	INFL,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
		0,			RLEFT,
		"ZI", },

/* convert pointers to pointers. */
{ SCONV,	ININT,
		SHINT,			TPOINT,
		SANY,			TPOINT,
		0,			RLEFT,
		"", },

/* char to something */

/* convert char to 16bit types. */

/* 8086: Only as op on AL,AX */
{ SCONV,	ININT,
		SBREG,			TCHAR,
		SAREG,			T16,
		NSPECIAL|NAREG|NASL,	RESC1,
		"	cbw\n", },

/* convert unsigned char to 16bit types.
   We can do this one with any register */
{ SCONV,	ININT,
		SHCH|SOREG|SNAME,	TUCHAR,
		SAREG,			T16,
		NSPECIAL|NAREG,		RESC1,
		"ZT", },

/* convert char to (u)long
   8086 can only do cbw/cwd on AX and AX:DX pair */
{ SCONV,	INL,
		SHCH,			TCHAR,
		SCREG,			T32,
		NSPECIAL|NCREG|NCSL,	RESC1,
		"; using AL\n	cbw\n	cwd\n", },

/* convert unsigned char to (u)long
   we can do this with any register */
{ SCONV,	INL,
		SHCH|SOREG|SNAME,	TUCHAR,
		SANY,			T32,
		NCREG|NCSL,		RESC1,
		"ZT	xor U1,U1\n", },

/* convert char (in register) to double XXX - use NTEMP */
/* FIXME : need NSPECIAL to force into AL 
	check AX:DX right way around ! */
{ SCONV,	INFL,
		SHCH|SOREG|SNAME,	TCHAR,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NASL|NDREG,	RESC2,
		"	cbw\n	cwd\n	push ax\n	push dx\n"
		"	fildl [sp]\n	add sp, #4\n", },

/* convert (u)char (in register) to double XXX - use NTEMP */
/* FIXME : needs to use ZT to get sizes right, need a register
   from somewhere to put 0 on the stack for the high bits */
{ SCONV,	INFL,
		SHCH|SOREG|SNAME,	TUCHAR,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NASL|NDREG,	RESC2,
		"	mov A1 AL\n	and A1, #0xff\n	push A1\npush #0\n"
		"	fildl [sp]\n	add sp,#4\n", },

/* 16bit to something */

/* convert 16bit to 16bit. */
{ SCONV,	INAREG,
		SAREG,			T16,
		SAREG,			T16,
		0,			RLEFT,
		"", },

/* convert 16bit (in memory) to char */
/* NOTE: uses new 'ZP' type for 'untyped right' */
{ SCONV,	INCH,
		SNAME|SOREG,		TP16,
		SHCH,			T8,
		NBREG|NBSL,		RESC1,
		"	mov A1,ZP AL\n", },

/* convert 16bit (in reg) to char. Done as a special but basically a mov */
{ SCONV,	INCH,
		SAREG|SNAME|SOREG,	TP16,
		SHCH,			T8,
		NSPECIAL|NBREG|NBSL,	RESC1,
		"ZM", },

/* convert short/int to (u)long
   This must go via AX so we can use cwd */
{ SCONV,	INL,
		SAREG,			TINT,
		SHL,			T32,
		NSPECIAL|NCREG|NCSL,	RESC1,
		"	mov A1, AL\n	cwd\n", },

/* convert unsigned short/int to (u)long
   Any pair will do. Note currently the compiler can't optimise
   out the reg->reg move involved */
{ SCONV,	INLL,
		SAREG|SOREG|SNAME,	TUNSIGNED|TPOINT,
		SHL,			T32,
		NCREG|NCSL,		RESC1,
		"	mov A1,AL\n	xor U1,U1\n", },

/* convert short/int (in memory) to float/double */
{ SCONV,	INFL,
		SOREG|SNAME,		TINT,
		SDREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,			RESC1,
		"	fild AL\n", },

/* convert short/int (in register) to float/double */
{ SCONV,	INFL,
		SAREG,			TINT,
		SDREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NDREG,		RESC1,
		"	push AL\n	fild [sp]\n"
		"	inc sp\n	inc sp\n", },

/* convert unsigned short/int/ptr to double XXX - use NTEMP */
/* FIXME: need to force this via AX so we can cwd, fix stack setup to
   push both ax.dx*/
{ SCONV,	INFL,
		SAREG|SOREG|SNAME,	TUNSIGNED|TPOINT,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NAREG|NASL|NDREG|NTEMP,	RESC2,
		"	cwd\n		push ax\n"
		"	fild [sp]\n	inc sp\n"
		"	inc sp\n", },

/* long to something */

/* convert (u)long to (u)char (mem->reg) */
{ SCONV,	INCH,
		SOREG|SNAME,		T32,
		SANY,			T8,
		NBREG|NBSL,		RESC1,
		"	mov A1, AL\n", },

/* convert (u)long to (u)char (reg->reg, hopefully nothing) */
{ SCONV,	INCH,
		SHL,			T32,
		SANY,			T8,
		NBREG|NBSL|NTEMP,	RESC1,
		"ZS", },

/* convert (u)long to (u)short (mem->reg) */
{ SCONV,	INAREG,
		SOREG|SNAME,		T32,
		SAREG,			TP16,
		NAREG|NASL,		RESC1,
		"	mov A1,AL\n", },

/* convert (u)long to 16bit (reg->reg, hopefully nothing) */
{ SCONV,	INAREG,
		SHL|SOREG|SNAME,	T32,
		SAREG,			T16,
		NAREG|NASL|NTEMP,	RESC1,
		"ZS", },

/* FIXME: float stuff is all TODO */

/* convert long long (in memory) to floating */
{ SCONV,	INFL,
		SOREG|SNAME,		TLONGLONG,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,			RESC1,
		"	fild AL\n", },

/* convert long (in register) to floating */
{ SCONV,	INFL,
		SHL,			TLONGLONG,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		NTEMP|NDREG,		RESC1,
		"	push UL\n	push AL\n"
		"	fild [sp]\n	add sp, #8\n", },

/* convert unsigned long to floating */
{ SCONV,	INFL,
		SCREG,			TULONGLONG,
		SDREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		NDREG,			RESC1,
		"ZJ", },

/* float to something */

#if 0 /* go via int by adding an extra sconv in clocal() */
/* convert float/double to (u) char. XXX should use NTEMP here */
{ SCONV,	INCH,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHCH,			T16|T8,
		NBREG,			RESC1,
		"	sub sp,#4\n	fistpl [sp]\n	pop	A1\n	pop U1\n", },

/* convert float/double to (u) int/short/char. XXX should use NTEMP here */
{ SCONV,	INCH,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHCH,			T16|T8,
		NCREG,			RESC1,
		"	sub sp,#4\n	fistpl (%sp)\n	pop A1\n	inc sp\n	inc sp\n", },
#endif

/* convert float/double to long XXX should use NTEMP here */
{ SCONV,	INAREG,
	SHFL,	TLDOUBLE|TDOUBLE|TFLOAT,
	SAREG,	TLONG,
		NAREG,	RESC1,
		"	sub sp, #12\n"
		"	fstcw sp\n"
		"	fstcw 4[sp]\n"
		"	mov byte ptr 1[sp], #12\n"
		"	fldcw sp\n"
		"	fistp 8[sp]\n"
		"	movl A1, 8(p)\n"
		"	fldcw 4[sp]\n"
		"	add sp, #4\n", },

/* convert float/double to unsigned long. XXX should use NTEMP here */
{ SCONV,       INAREG,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SAREG,			TULONG,
		NAREG,			RESC1,
		"	sub sp, #16\n"
		"	fnstcw [sp]\n"
		"	fnstcw 4[sp]\n"
		"	mov byte ptr 1[sp], #12\n"
		"	fldcw [sp[\n"
		"	fistpq 8[sp]\n"
		"	mov word ptr 8[sp],A1\n"
		"	mov word ptr 10[sp],U1\n"
		"	fldcw 4[sp]\n"
		"	add sp, #16\n", },

/* convert float/double (in register) to long long */
{ SCONV,	INLL,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHLL,			TLONGLONG,
		NCREG,			RESC1,
		"	sub sp, #16\n"
		"	fnstcw [sp]\n"
		"	fnstcw [sp]\n"
		"	mov byte ptr 1[sp], #12\n"
		"	fldcw [sp]\n"
		"	fistpq 8[sp]\n"
		"	movl 8[sp],A1\n"
		"	movl 10[sp],U1\n"
		/* need to put 12/14 somewhere FIXME */
		"	fldcw 4[sp]\n"
		"	add sp, #16\n", },

/* convert float/double (in register) to unsigned long long */
{ SCONV,	INLL,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHLL,			TULONGLONG,
		NCREG,			RESC1,
		"	sub sp, #16\n"
		"	fnstcw [sp]\n"
		"	fnstcw 4[sp]\n"
		"	mov byte ptr 1[sp], #15\n"	/* 64-bit prec */
		"	fldcw [sp]\n"
		"	movl $0x5f000000, 8(%sp)\n"	/* (float)(1<<63) */
		"	fsubs 8[sp]\n"	/* keep in range of fistpq */
		"	fistpq 8[sp]\n"
		"	xor byte ptr 15[sp], #0x80\n"	/* addq $1>>63 to 8(%sp) */
		"	movl A1, 8[sp]\n"
		"	movl U1, 10[sp]\n"
		"	fldcw 4[sp]\n"
		"	add sp, #16\n", },
 


/* slut sconv */

/*
 * Subroutine calls.
 */

{ UCALL,	FOREFF,
		SCON,			TANY,
		SANY,			TANY,
		0,	0,
		"	call CL\nZC", },

{ CALL,		FOREFF,
		SCON,			TANY,
		SANY,			TANY,
		0,	0,
		"	call CL\nZC", },

//{ UCALL,	FOREFF,
//	SCON,	TANY,
//	SAREG,	TP16,
//		0,	0,
//		"	call CL\nZC", },

{ CALL,	INAREG,
		SCON,			TANY,
		SAREG,			TP16,
		NAREG|NASL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INAREG,
		SCON,			TANY,
		SAREG,			TP16,
		NAREG|NASL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ CALL,	INBREG,
		SCON,			TANY,
		SBREG,			T8,
		NBREG,			RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INBREG,
		SCON,			TANY,
		SBREG,			T8,
		NBREG,			RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ CALL,		INCREG,
		SCON,			TANY,
		SCREG,			TANY,
		NCREG|NCSL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INCREG,
		SCON,			TANY,
		SCREG,			TANY,
		NCREG|NCSL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ CALL,	INDREG,
		SCON,			TANY,
		SDREG,			TANY,
		NDREG|NDSL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ UCALL,	INDREG,
		SCON,			TANY,
		SDREG,			TANY,
		NDREG|NDSL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ CALL,		FOREFF,
		SAREG,			TANY,
		SANY,			TANY,
		0,	0,
		"	call *AL\nZC", },

{ UCALL,	FOREFF,
		SAREG,			TANY,
		SANY,			TANY,
		0,	0,
		"	call *AL\nZC", },

{ CALL,		INAREG,
		SAREG,			TANY,
		SANY,			TANY,
		NAREG|NASL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INAREG,
		SAREG,			TANY,
		SANY,			TANY,
		NAREG|NASL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INBREG,
		SAREG,			TANY,
		SANY,			TANY,
		NBREG|NBSL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INBREG,
		SAREG,			TANY,
		SANY,			TANY,
		NBREG|NBSL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INCREG,
		SAREG,			TANY,
		SANY,			TANY,
		NCREG|NCSL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INCREG,
		SAREG,			TANY,
		SANY,			TANY,
		NCREG|NCSL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ CALL,		INDREG,
		SAREG,			TANY,
		SANY,			TANY,
		NDREG|NDSL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ UCALL,	INDREG,
		SAREG,			TANY,
		SANY,			TANY,
		NDREG|NDSL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

{ STCALL,	FOREFF,
		SCON,			TANY,
		SANY,			TANY,
		NAREG|NASL,	0,
		"	call CL\nZC", },

{ STCALL,	INAREG,
		SCON,			TANY,
		SANY,			TANY,
		NAREG|NASL,		RESC1,	/* should be 0 */
		"	call CL\nZC", },

{ STCALL,	INAREG,
		SNAME|SAREG,		TANY,
		SANY,			TANY,
		NAREG|NASL,		RESC1,	/* should be 0 */
		"	call *AL\nZC", },

/*
 * The next rules handle all binop-style operators.
 *
 * 8 and 16bit we do directly, 32bit is a mix of helpers and
 * direct operations. 64bit TODO, FPU lives in it's own little
 * world.
 */
{ PLUS,		INL|FOREFF,
		SHL,			T32,
		SHL|SNAME|SOREG,	T32,
		0,			RLEFT,
		"	add AL,AR\n	adc UL,UR\n", },

{ PLUS,		INL|FOREFF,
		SHL|SNAME|SOREG,	T32,
		SHL|SCON,		T32,
		0,			RLEFT,
		"	add AL,AR\n	adc UL,UR\n", },

/* Special treatment for long  XXX - fix commutative check */
{ PLUS,		INL|FOREFF,
		SHL|SNAME|SOREG,	T32,
		SHL,			T32,
		0,			RRIGHT,
		"	add AL,AR\n	adc UL,UR\n", },

{ PLUS,		INFL,
		SHFL,			TDOUBLE,
		SNAME|SOREG,		TDOUBLE,
		0,			RLEFT,
		"	faddl AR\n", },

{ PLUS,		INFL|FOREFF,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		0,			RLEFT,
		"	faddp\n", },

{ PLUS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TP16,
		SONE,			TANY,
		0,			RLEFT,
		"	inc AL\n", },

{ PLUS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TP16,
		STWO,			TANY,
		0,			RLEFT,
		"	inc AL\n	inc AL\n", },

{ PLUS,		INCH|FOREFF,
		SHCH|SNAME|SOREG,	T8,
		SONE,			TANY,
		0,			RLEFT,
		"	inc AL\n", },

{ PLUS,		INCH|FOREFF,
		SHCH|SNAME|SOREG,	T8,
		STWO,			TANY,
		0,			RLEFT,
		"	inc AL\n	inc AL\n", },

{ PLUS,		INAREG,
		SAREG,			T16,
		SAREG,			T16,
		NAREG|NASL|NASR,	RESC1,
		"	lea A1, [AL+AR]\n", },

{ MINUS,	INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TP16,
		SONE,			TANY,
		0,			RLEFT,
		"	dec AL\n", },

{ MINUS,	INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TP16,
		STWO,			TANY,
		0,			RLEFT,
		"	dec AL\n	dec AL\n", },

{ MINUS,	INCH|FOREFF,
		SHCH|SNAME|SOREG,	T8,
		SONE,			TANY,
		0,			RLEFT,
		"	dec AL\n", },

{ MINUS,	INCH|FOREFF,
		SHCH|SNAME|SOREG,	T8,
		STWO,			TANY,
		0,			RLEFT,
		"	dec AL\n	decl AL\n", },

/* address as register offset, negative */
{ MINUS,	INLL|FOREFF,
		SHL,			T32,
		SHL|SNAME|SOREG,	T32,
		0,			RLEFT,
		"	sub AL,AR\n	sbb UL,UR\n", },

{ MINUS,	INL|FOREFF,
		SHL|SNAME|SOREG,	T32,
		SHL|SCON,		T32,
		0,			RLEFT,
		"	sub AL,AR\n	sbb UL,UR\n", },

{ MINUS,	INFL,
		SHFL,			TDOUBLE,
		SNAME|SOREG,		TDOUBLE,
		0,			RLEFT,
		"	fsubl AR\n", },

{ MINUS,	INFL|FOREFF,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		0,			RLEFT,
		"	fsubZAp\n", },

/* Simple r/m->reg ops */
/* m/r |= r */

{ OPSIMP,	INAREG|FOREFF|FORCC,
		SHINT|SNAME|SOREG,	TP16,
		SHINT,			TP16,
		0,			RLEFT|RESCC,
		"	Ow AL,AR\n", },

/* r |= r/m */
{ OPSIMP,	INAREG|FOREFF|FORCC,
		SHINT,			T16,
		SHINT|SNAME|SOREG,	T16,
		0,			RLEFT|RESCC,
		"	Ow AL,AR\n", },

/* m/r |= r */
{ OPSIMP,	INCH|FOREFF|FORCC,
		SHCH,			T8,
		SHCH|SNAME|SOREG,	T8,
		0,			RLEFT|RESCC,
		"	Ob AL,AR\n", },

/* r |= r/m */
{ OPSIMP,	INCH|FOREFF|FORCC,
		SHCH,			T8,
		SHCH|SNAME|SOREG,	T8,
		0,			RLEFT|RESCC,
		"	Ob AL,AR\n", },

/* m/r |= const */
{ OPSIMP,	INAREG|FOREFF|FORCC,
		SHINT|SNAME|SOREG,	TP16,
		SCON,			TANY,
		0,			RLEFT|RESCC,
		"	Ow AL,AR\n", },

{ OPSIMP,	INCH|FOREFF|FORCC,
		SHCH|SNAME|SOREG,	T8,
		SCON,			TANY,
		0,			RLEFT|RESCC,
		"	Ob AL,AR\n", },

/* r |= r/m */
{ OPSIMP,	INL|FOREFF,
		SHL,			T32,
		SHL|SNAME|SOREG,	T32,
		0,			RLEFT,
		"	Ow AL,AR\n	Ow UL,UR\n", },

/* m/r |= r/const */
{ OPSIMP,	INL|FOREFF,
		SHL|SNAME|SOREG,	T32,
		SHL|SCON,		T32,
		0,			RLEFT,
		"	Ow AL,AR\n	Ow UL,UR\n", },

/* Try use-reg instructions first */
{ PLUS,		INAREG,
		SAREG,			TP16,
		SCON,			TANY,
		NAREG|NASL,		RESC1,
		"	lea A1, CR[AL]\n", },

{ MINUS,	INAREG,
		SAREG,			TP16,
		SPCON,			TANY,
		NAREG|NASL,		RESC1,
		"	lea A1, -CR[AL]\n", },


/*
 * The next rules handle all shift operators.
 */
/* (u)long left shift is emulated, longlong to do */

/* For 8086 we only have shift by 1 or shift by CL

   We need a way to optimise shifts by 8/16/24/32/etc
   shifts by 8 16 24 and 32 as moves between registers for
   the bigger types. (eg >> 16 on a long might be mov dx, ax,
   xor ax, ax) */
   
{ LS,		INCREG,
		SCREG,			T32,
		SHCH,			T8,
		0,			RLEFT,
		"ZO", },

/* Register 8086 timing is 2 for #1 versus 8 + 4/bin for multiple
   so a lot of shifts would in fact best be done without using the cl
   variant, especially including the cost of a) loading cl b) probably
   having to boot something out of cx in the first place. For memory
   its 15+EA v 20 + EA + 4/bit, so the other way.
   
   8,16,24 should of course be done by loads.. FIXME 
   
   Also the compiler keeps generating mov dh, #8, mov cl, dh.. FIXME
   
   For 80186 onwards we have shl reg, immediate (other than 1), 186 shift
   is also much faster */


/* r/m <<= const 1*/
{ LS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	T16,
		SONE,			TANY,
		0,			RLEFT,
		"	shl AL,#1\n", },

/* r <<= const 2 */
{ LS,		INAREG|FOREFF,
		SAREG,			T16,
		STWO,			TANY,
		0,			RLEFT,
		"	shl AL,#1\n	shl AL,#1\n", },

/* r/m <<= r */
{ LS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	T16,
		SHCH,			T8,
		NSPECIAL,		RLEFT,
		"	shl AL,AR\n", },

{ LS,		INCH|FOREFF,
		SHCH|SNAME|SOREG,	T8,
		SONE,			TANY,
		NSPECIAL,		RLEFT,
		"	sal AL,#1\n", },

{ LS,		INCH|FOREFF,
		SHCH,			T8,
		STWO,			TANY,
		NSPECIAL,		RLEFT,
		"	sal AL,#1\n	sal AL,#1", },

{ LS,		INCH|FOREFF,
		SHCH|SNAME|SOREG,	T8,
		SHCH,			T8,
		NSPECIAL,		RLEFT,
		"	sal AL,AR\n", },

/* (u)long right shift is emulated. Use a 16bit register so the push
   comes out sanely */
{ RS,		INCREG,
		SCREG,			T32,
		SHCH,			T8,
		0,			RLEFT,
		"ZO", },

{ RS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TINT,
		SHCH,			T8,
		NSPECIAL,		RLEFT,
		"	sar AL,AR\n", },

{ RS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TINT,
		SONE,			TANY,
		NSPECIAL,		RLEFT,
		"	sar AL,#1\n", },

{ RS,		INAREG|FOREFF,
		SAREG,			TINT,
		STWO,			TANY,
		NSPECIAL,		RLEFT,
		"	sar AL,#1\n	sar AL,#1", },

{ RS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TUNSIGNED|TPOINT,
		SHCH,			T8,
		NSPECIAL,		RLEFT,
		"	shr AL,AR\n", },

{ RS,		INAREG|FOREFF,
		SAREG|SNAME|SOREG,	TUNSIGNED,
		SONE,			TANY,
		0,			RLEFT,
		"	shr AL,#1\n", },

{ RS,		INAREG|FOREFF,
		SAREG,			TUNSIGNED,
		STWO,			TANY,
		0,			RLEFT,
		"	shr AL,#1	shr AL,#1\n", },

{ RS,	INCH|FOREFF,
		SHCH|SNAME|SOREG,	TCHAR,
		SHCH,			T8,
		NSPECIAL,		RLEFT,
		"	sar AL,AR\n", },

{ RS,	INCH|FOREFF,
		SHCH|SNAME|SOREG,	TCHAR,
		SONE,			TANY,
		NSPECIAL,		RLEFT,
		"	sar AL,#1\n", },

{ RS,	INCH|FOREFF,
		SHCH|SNAME|SOREG,	TCHAR,
		STWO,			TANY,
		NSPECIAL,		RLEFT,
		"	sar AL,#1\n	sar AL,#1\n", },

{ RS,	INCH|FOREFF,
		SHCH|SNAME|SOREG,	TUCHAR,
		SHCH,			T8,
		NSPECIAL,		RLEFT,
		"	shr AL,AR\n", },

{ RS,		INCH|FOREFF,
		SHCH|SNAME|SOREG,	TUCHAR,
		SONE,			TANY,
		NSPECIAL,		RLEFT,
		"	shr AL,#1\n", },

{ RS,		INCH|FOREFF,
		SHCH|SNAME|SOREG,	TUCHAR,
		STWO,			TANY,
		NSPECIAL,		RLEFT,
		"	shr AL,#1\n	shr AL,#1\n", },

/*
 * The next rules takes care of assignments. "=".
 */
 
{ ASSIGN,	FORCC|FOREFF|INL,
		SHL,			T32,
		SMIXOR,			TANY,
		0,			RDEST,
		"	xor AL,AL\n	xor UL,UL\n", },

{ ASSIGN,	FORCC|FOREFF|INL,
		SHL,			T32,
		SMILWXOR,		TANY,
		0,			RDEST,
		"	xor AL,AL\n	mov UR,UL\n", },

{ ASSIGN,	FORCC|FOREFF|INL,
		SHL,			T32,
		SMIHWXOR,		TANY,
		0,			RDEST,
		"	mov AL,AR\n	xor UL,UL\n", },

{ ASSIGN,	FOREFF|INL,
		SHL,			T32,
		SCON,			TANY,
		0,			RDEST,
		"	mov AL,AR\n	mov UL,UR\n", },

{ ASSIGN,	FOREFF,
		SHL|SNAME|SOREG,	T32,
		SCON,			TANY,
		0,			0,
		"	mov AL,AR\n	mov UL,UR\n", },

{ ASSIGN,	FOREFF,
		SAREG|SNAME|SOREG,	TP16,
		SCON,			TANY,
		0,			0,
		"	mov AL, AR\n", },

{ ASSIGN,	FOREFF|INAREG,
		SAREG,			TP16,
		SCON,			TANY,
		0,			RDEST,
		"	mov AL, AR\n", },

{ ASSIGN,	FOREFF,
		SHCH|SNAME|SOREG,	T8,
		SCON,			TANY,
		0,	0,
		"	mov AL, AR\n", },

{ ASSIGN,	FOREFF|INCH,
		SHCH,			T8,
		SCON,			TANY,
		0,			RDEST,
		"	mov AL, AR\n", },

{ ASSIGN,	FOREFF|INL,
		SNAME|SOREG,		T32,
		SHL,			T32,
		0,			RDEST,
		"	mov AL,AR\n	mov UL,UR\n", },

{ ASSIGN,	FOREFF|INL,
		SHL,			T32,
		SHL,			T32,
		0,			RDEST,
		"ZH", },

{ ASSIGN,	FOREFF|INAREG,
		SAREG|SNAME|SOREG,	TP16,
		SAREG,			TP16,
		0,			RDEST,
		"	mov AL,AR\n", },

{ ASSIGN,	FOREFF|INAREG,
		SAREG,			TP16,
		SAREG|SNAME|SOREG,	TP16,
		0,			RDEST,
		"	mov AL,AR\n", },

{ ASSIGN,	FOREFF|INCH,
		SHCH|SNAME|SOREG,	T8,
		SHCH,			T8|T16,
		0,			RDEST,
		"	mov AL,AR\n", },

{ ASSIGN,	INDREG|FOREFF,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			RDEST,
		"", }, /* This will always be in the correct register */

/* order of table entries is very important here! */
{ ASSIGN,	INFL,
		SNAME|SOREG,		TLDOUBLE,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			RDEST,
		"	fstpt AL\n	fldt AL\n", }, /* XXX */

{ ASSIGN,	FOREFF,
		SNAME|SOREG,		TLDOUBLE,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			0,
		"	fstpt AL\n", },

{ ASSIGN,	INFL,
		SNAME|SOREG,		TDOUBLE,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			RDEST,
		"	fstl AL\n", },

{ ASSIGN,	FOREFF,
		SNAME|SOREG,		TDOUBLE,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			0,
		"	fstpl AL\n", },

{ ASSIGN,	INFL,
		SNAME|SOREG,		TFLOAT,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			RDEST,
		"	fsts AL\n", },

{ ASSIGN,	FOREFF,
		SNAME|SOREG,		TFLOAT,
		SHFL,			TFLOAT|TDOUBLE|TLDOUBLE,
		0,			0,
		"	fstps AL\n", },
/* end very important order */

{ ASSIGN,	INFL|FOREFF,
		SHFL,			TLDOUBLE,
		SHFL|SOREG|SNAME,	TLDOUBLE,
		0,			RDEST,
		"	fldt AR\n", },

{ ASSIGN,	INFL|FOREFF,
		SHFL,			TDOUBLE,
		SHFL|SOREG|SNAME,	TDOUBLE,
		0,			RDEST,
		"	fldl AR\n", },

{ ASSIGN,	INFL|FOREFF,
		SHFL,			TFLOAT,
		SHFL|SOREG|SNAME,	TFLOAT,
		0,			RDEST,
		"	flds AR\n", },

/* Do not generate memcpy if return from funcall */
#if 0
{ STASG,	INAREG|FOREFF,
		SOREG|SNAME|SAREG,	TPTRTO|TSTRUCT,
		SFUNCALL,		TPTRTO|TSTRUCT,
		0,			RRIGHT,
		"", },
#endif

{ STASG,	INAREG|FOREFF,
		SOREG|SNAME,		TANY,
		SAREG,			TPTRTO|TANY,
		NSPECIAL|NAREG,		RDEST,
		"F	mov A1,si\nZQF	mov si,A1\n", },

/*
 * DIV/MOD/MUL 
 */
/* long div is emulated */
{ DIV,	INCREG,
		SCREG|SNAME|SOREG|SCON, T32,
		SCREG|SNAME|SOREG|SCON, T32,
		NSPECIAL|NCREG|NCSL|NCSR,	RESC1,
		"ZO", },

/* REVIEW We can only do (i)divb ax/byte  and (i)divw (dx:ax)/word
   and the results are always in ah/al (remainer/mod)
   or dx:ax (dx = remainer, ax = mod)
   
   Power of two needs to be done by shifts. For other cases of constants
   we need to implement two things
   1. Spotting add sequences for constants with few 1 bits on one side
   2. Spotting cases we can compute the magic constant to multiply with for
      the same result */
   
   
{ DIV,	INAREG,
		SAREG,			TUNSIGNED|TPOINT,
		SAREG|SNAME|SOREG,	TUNSIGNED|TPOINT,
		NSPECIAL,		RDEST,
		"	xor dx,dx\n	divw AR\n", },

{ DIV,	INCH,
		SHCH,			TUCHAR,
		SHCH|SNAME|SOREG,	TUCHAR,
		NSPECIAL,		RDEST,
		"	xor ah,ah\n	divb AR\n", },

{ DIV,	INFL,
		SHFL,			TDOUBLE,
		SNAME|SOREG,		TDOUBLE,
		0,			RLEFT,
		"	fdivl AR\n", },

{ DIV,	INFL,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		0,			RLEFT,
		"	fdivZAp\n", },

/* (u)long mod is emulated */
{ MOD,		INCREG,
		SCREG|SNAME|SOREG|SCON,	T32,
		SCREG|SNAME|SOREG|SCON,	T32,
		NSPECIAL|NCREG|NCSL|NCSR,	RESC1,
		"ZO", },

{ MOD,		INAREG,
		SAREG,			TP16,
		SAREG|SNAME|SOREG,	TP16,
		NAREG|NSPECIAL,		RESC1,
		"	xor dx,dx\n	divw AR\n", },

{ MOD,	INCH,
		SHCH,			TUCHAR,
		SHCH|SNAME|SOREG,	TUCHAR,
		NBREG|NSPECIAL,		RESC1,
		"	xor ah,ah\n	divb AR\n", },

/* (u)long mul is emulated */
/* On 8086 we can only do multiplies of al * value into ax (for 8bit)
   or ax * value into dx:ax for 16bit 
   
   80186 allows us to do a signed multiply of a register with a constant
   into a second register
   
   Same about shifts, and add optimisations applies here too */
   
/* 32bit mul is emulated (for now) */
{ MUL,		INCREG,
		SCREG|SNAME|SOREG|SCON,		T32,
		SCREG|SNAME|SOREG|SCON,		T32,
		NSPECIAL|NCREG|NCSL|NCSR,	RESC1,
		"ZO", },

/* FIMXE: need special rules */
{ MUL,	INAREG,
		SAREG,			T16|TPOINT,
		SAREG|SNAME|SOREG,	T16|TPOINT,
		NSPECIAL,		RDEST,
		"	mul AR\n", },

{ MUL,	INCH,
		SHCH,			T8,
		SHCH|SNAME|SOREG,	T8,
		NSPECIAL,		RDEST,
		"	mulb AR\n", },

{ MUL,	INFL,
		SHFL,			TDOUBLE,
		SNAME|SOREG,		TDOUBLE,
		0,			RLEFT,
		"	fmull AR\n", },

{ MUL,	INFL,
		SHFL	,		TLDOUBLE|TDOUBLE|TFLOAT,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		0,			RLEFT,
		"	fmulp\n", },

/*
 * Indirection operators.
 */
{ UMUL,	INLL,
		SANY,			TANY,
		SOREG,			T32,
		NCREG,			RESC1,
		"	mov UL,U1\n	mov AL,A1\n", },

{ UMUL,	INAREG,
		SANY,			TP16,
		SOREG,			TP16,
		NAREG|NASL,		RESC1,
		"	mov AL,A1\n", },

{ UMUL,	INCH,
		SANY,			TANY,
		SOREG,			T8,
		NBREG|NBSL,		RESC1,
		"	mov AL,A1\n", },

{ UMUL,	INAREG,
		SANY,			TANY,
		SOREG,			T16,
		NAREG|NASL,		RESC1,
		"	mov AL,A1\n", },

{ UMUL,	INFL,
		SANY,			TANY,
		SOREG,			TLDOUBLE,
		NDREG|NDSL,		RESC1,
		"	fldt AL\n", },

{ UMUL,	INFL,
		SANY,			TANY,
		SOREG,			TDOUBLE,
		NDREG|NDSL,		RESC1,
		"	fldl AL\n", },

{ UMUL,	INFL,
		SANY,			TANY,
		SOREG,			TFLOAT,
		NDREG|NDSL,		RESC1,
		"	flds AL\n", },

/*
 * Logical/branching operators
 */

/* Comparisions, take care of everything */
{ OPLOG,	FORCC,
		SHL|SOREG|SNAME,	T32,
		SHL,			T32,
		0,	0,
		"ZD", },

{ OPLOG,	FORCC,
		SAREG|SOREG|SNAME,	TP16,
		SCON|SAREG,		TP16,
		0, 			RESCC,
		"	cmp AL,AR\n", },


{ OPLOG,	FORCC,
		SCON|SAREG,		TP16,
		SAREG|SOREG|SNAME,	TP16,
		0, 			RESCC,
		"	cmp AL,AR\n", },

{ OPLOG,	FORCC,
		SBREG|SOREG|SNAME,	T8,
		SCON|SBREG,		TANY,
		0, 			RESCC,
		"	cmpb AL,AR\n", },

{ OPLOG,	FORCC,
		SCON|SBREG,		T8,
		SBREG|SOREG|SNAME,	TANY,
		0, 			RESCC,
		"	cmpb AL,AR\n", },

{ OPLOG,	FORCC,
		SDREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		SDREG,			TLDOUBLE|TDOUBLE|TFLOAT,
		0, 			RNOP,
		"ZG", },

{ OPLOG,	FORCC,
		SANY,			TANY,
		SANY,			TANY,
		REWRITE,	0,
		"diediedie!", },

/* AND/OR/ER/NOT */

/* FIXME: pcc generates nonsense ands, but they should be fixed somewhere
   if possible. In particular it will do the classic 32bit and with a 16bit
   0xFFFF by generating and ax,#ffff and bx,#0
   	eg compiling
   		sum1 = (sum1 & 0xFFFF) + (sum1 >> 16);
	writes a pile of crap code.
*/
   	
   	
{ AND,	INCREG|FOREFF,
		SCREG,			T32,
		SCREG|SOREG|SNAME,	T32,
		0,			RLEFT,
		"	and AR,AL\n	and UR,UL\n", },

{ AND,	INAREG|FOREFF,
		SAREG,			T16,
		SAREG|SOREG|SNAME,	T16,
		0,			RLEFT,
		"	and AR,AL\n", },

{ AND,	INAREG|FOREFF,  
		SAREG|SOREG|SNAME,	T16,
		SCON|SAREG,		T16,
		0,			RLEFT,
		"	and AR,AL\n", },

{ AND,	INBREG|FOREFF,
		SBREG|SOREG|SNAME,	T8,
		SCON|SBREG,		T8,
		0,			RLEFT,
		"	and AR,AL\n", },

{ AND,	INBREG|FOREFF,
		SBREG,			T8,
		SBREG|SOREG|SNAME,	T8,
		0,			RLEFT,
		"	and AR,AL\n", },
/* AND/OR/ER/NOT */

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
		SCON,			TANY,
		SANY,			TANY,
		0,			RNOP,
		"	jmp LL\n", },

#if defined(GCC_COMPAT) || defined(LANG_F77)
{ GOTO, 	FOREFF,
		SAREG,			TANY,
		SANY,			TANY,
		0,			RNOP,
		"	jmp *AL\n", },
#endif

/*
 * Convert LTYPE to reg.
 */
{ OPLTYPE,	FORCC|INL,
		SCREG,			T32,
		SMIXOR,			TANY,
		NCREG,			RESC1,
		"	xor U1,U1\n	xor A1,A1\n", },

{ OPLTYPE,	FORCC|INL,
		SCREG,			T32,
		SMILWXOR,		TANY,
		NCREG,			RESC1,
		"	mov U1,UL\n	xor A1,A1\n", },

{ OPLTYPE,	FORCC|INL,
		SCREG,			T32,
		SMIHWXOR,		TANY,
		NCREG,			RESC1,
		"	xor U1,U1\n	mov A1,AL\n", },

{ OPLTYPE,	INL,
		SANY,			TANY,
		SCREG,			T32,
		NCREG,			RESC1,
		"ZK", },

{ OPLTYPE,	INL,
		SANY,			TANY,
		SCON|SOREG|SNAME,	T32,
		NCREG,			RESC1,
		"	mov U1,UL\n	mov A1,AL\n", },

{ OPLTYPE,	FORCC|INAREG,
		SAREG,			TP16,
		SMIXOR,			TANY,
		NAREG|NASL,		RESC1,
		"	xor A1,A1\n", },

{ OPLTYPE,	INAREG,
		SANY,			TANY,
		SAREG|SCON|SOREG|SNAME,	TP16,
		NAREG|NASL,		RESC1,
		"	mov A1,AL\n", },

{ OPLTYPE,	INBREG,
		SANY,			TANY,
		SBREG|SOREG|SNAME|SCON,	T8,
		NBREG,			RESC1,
		"	mov A1,AL\n", },

{ OPLTYPE,	FORCC|INAREG,
		SAREG,			T16,
		SMIXOR,			TANY,
		NAREG,			RESC1,
		"	xor A1,A1\n", },

{ OPLTYPE,	INAREG,
		SANY,			TANY,
		SAREG|SOREG|SNAME|SCON,	T16,
		NAREG,			RESC1,
		"	mov A1,AL\n", },

{ OPLTYPE,	INDREG,
		SANY,			TLDOUBLE,
		SOREG|SNAME,		TLDOUBLE,
		NDREG,			RESC1,
		"	fldt AL\n", },

{ OPLTYPE,	INDREG,
		SANY,			TDOUBLE,
		SOREG|SNAME,		TDOUBLE,
		NDREG,			RESC1,
		"	fldl AL\n", },

{ OPLTYPE,	INDREG,
		SANY,			TFLOAT,
		SOREG|SNAME,		TFLOAT,
		NDREG,			RESC1,
		"	flds AL\n", },

/* Only used in ?: constructs. The stack already contains correct value */
{ OPLTYPE,	INDREG,
		SANY,			TFLOAT|TDOUBLE|TLDOUBLE,
		SDREG,			TFLOAT|TDOUBLE|TLDOUBLE,
		NDREG,			RESC1,
		"", },

/*
 * Negate a word.
 */

{ UMINUS,	INCREG|FOREFF,
		SCREG,			T32,
		SCREG,			T32,
		0,			RLEFT,
		"	neg AL\n	adc UL,#0\n	neg UL\n", },

{ UMINUS,	INAREG|FOREFF,
		SAREG,			TP16,
		SAREG,			TP16,
		0,			RLEFT,
		"	neg AL\n", },

{ UMINUS,	INBREG|FOREFF,
		SBREG,			T8,
		SBREG,			T8,
		0,			RLEFT,
		"	neg AL\n", },

{ UMINUS,	INFL|FOREFF,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		SHFL,			TLDOUBLE|TDOUBLE|TFLOAT,
		0,			RLEFT,
		"	fchs\n", },

{ COMPL,	INCREG,
		SCREG,			T32,
		SANY,			TANY,
		0,			RLEFT,
		"	not AL\n	not UL\n", },

{ COMPL,	INAREG,
		SAREG,			T16,
		SANY,			TANY,
		0,			RLEFT,
		"	not AL\n", },

{ COMPL,	INBREG,
		SBREG,			T8,
		SANY,			TANY,
		0,			RLEFT,
		"	not AL\n", },

/*
 * Arguments to functions.
 *
 * char has already been promoted to integer types
 */
 
/* Push immediate not 8086... Loading a register and pushing costs us
   4 + 11 clocks, loading memory would cost us 16 + EA */
{ FUNARG,	FOREFF,
		/*SCON|*/SCREG|SNAME|SOREG,	T32,
		SANY,			T32,
		0,			RNULL,
		"	push UL\n	push AL\n", },

{ FUNARG,	FOREFF,
		/*SCON|*/SAREG|SNAME|SOREG,	T16|TPOINT,
		SANY,			TP16,
		0,			RNULL,
		"	push AL\n", },

/* FIXME: FPU needs reworking into 4 regs or a memcpy */
{ FUNARG,	FOREFF,
		SNAME|SOREG,		TDOUBLE,
		SANY,			TDOUBLE,
		0,			0,
		"	pushl UL\n	pushl AL\n", },

{ FUNARG,	FOREFF,
		SDREG,			TDOUBLE,
		SANY,			TDOUBLE,
		0,			0,
		"	sub sp,#8\n	fstpl [sp]\n", },

{ FUNARG,	FOREFF,
		SNAME|SOREG,		TFLOAT,
		SANY,			TFLOAT,
		0,	0,
		"	push UL\n	push AL\n", },

{ FUNARG,	FOREFF,
		SDREG,			TFLOAT,
		SANY,			TFLOAT,
		0,			0,
		"	sub sp,#4\n	fstps [sp]\n", },

{ FUNARG,	FOREFF,
		SDREG,			TLDOUBLE,
		SANY,			TLDOUBLE,
		0,			0,
		"	sub sp,#12\n	fstpt [sp]\n", },

{ STARG,	FOREFF,
		SAREG,			TPTRTO|TSTRUCT,
		SANY,			TSTRUCT,
		NSPECIAL,		0,
		"ZF", },

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ FLD, DF(FLD), },

{ OPLEAF, DF(NAME), },

/* { INIT, DF(INIT), }, */

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" },
};

int tablesize = sizeof(table)/sizeof(table[0]);
