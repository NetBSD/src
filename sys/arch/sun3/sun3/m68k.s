
ENTRY(getvbr)
	movc vbr, d0
	rts

ENTRY(setvbr)
	movl sp@(4), d0
	movc d0, vbr
	rts

/* void control_copy_byte(caddr_t from, caddr_t to, int size)*/

ENTRY(control_copy_byte)
	movl sp@(4), a0			|a0 = from
	movl sp@(8), a1			|a1 = to 
	movl sp@(12), d1		|d1 = size	
	movl d2, sp@-			| save reg so we can use it for temp
	movc sfc, d0			| save sfc
	movl #FC_CONTROL, d2
	movc d2, sfc
	subqw #1, d1

loop:   movsb a0@+, d2
	movb  d2, a1@+
	dbra d1, loop

	movc d0, sfc
	movl sp@+, d2
	rts

/*	
 * unsigned char get_control_byte (char *)
 */	

ENTRY(get_control_byte)
	movl sp@(4), a0
	moveq #0, d0
	movsb a0@, d0
	rts
	
/*
 * unsigned int get_control_word (char *)
 */	

ENTRY(get_control_word)
	movl sp@(4), a0
	movsl a0@, d0
	rts

/*	
 * void set_control_byte (char *, unsigned char)
 */

ENTRY(set_control_byte)
	movl sp@(4), a0
	movl sp@(8), d0
	movsb d0, a0@
	rts

/*
 * void set_control_word (char *, unsigned int)
 */

ENTRY(set_control_word)
	movl sp@(4), a0
	movl sp@(8), d0
	movsl d0, a0@
	rts

/*
 * Get callers current SP value.
 * Note that simply taking the address of a local variable in a C function
 * doesn't work because callee saved registers may be outside the stack frame
 * defined by A6 (e.g. GCC generated code).
 *
 * [I don't think the ENTRY() macro will do the right thing with this -- glass]
 */
	.globl	_getsp; .align 2
_getsp:
	movl	sp,d0			| get current SP
	addql	#4,d0			| compensate for return address
	rts

	.globl	_getsfc, _getdfc
.align 2
_getsfc:
	movc	sfc,d0
	rts
.align 2
_getdfc:
	movc	dfc,d0
	rts


/*
 * non-local gotos
 */
ENTRY(setjmp)
	movl	sp@(4),a0	| savearea pointer
	moveml	#0xFCFC,a0@	| save d2-d7/a2-a7
	movl	sp@,a0@(48)	| and return address
	moveq	#0,d0		| return 0
	rts

ENTRY(qsetjmp)
	movl	sp@(4),a0	| savearea pointer
	lea	a0@(40),a0	| skip regs we do not save
	movl	a6,a0@+		| save FP
	movl	sp,a0@+		| save SP
	movl	sp@,a0@		| and return address
	moveq	#0,d0		| return 0
	rts

ENTRY(longjmp)
	movl	sp@(4),a0
	moveml	a0@+,#0xFCFC
	movl	a0@,sp@
	moveq	#1,d0
	rts
