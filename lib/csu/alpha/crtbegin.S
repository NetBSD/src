#	$NetBSD: crtbegin.S,v 1.1 1998/05/06 00:59:18 ross Exp $
#
# Warning -- 
#
# See comments in crtbegin.c prior to attempting work on this module.
#
# This module has been modified by hand.
#
	.file	1 "crtbegin.c"
	.version	"01.01"
	.set noat
gcc2_compiled.:
__gnu_compiled_c:
	
	.section ".note.netbsd.ident", "a"
	.p2align 2

	# NetBSD note: OS version
	.long	7				# name size
	.long	4				# desc size
	.long	1 # type
	.ascii "NetBSD\0\0"			# name (padded to % 4 == 0)
	.long	199804			# desc (padded to % 4 == 0)

	# NetBSD note: emulation name
	.long	7				# name size
	.long	7				# desc size
	.long	2 # type
	.ascii	"NetBSD\0\0"		# name (padded to % 4 == 0)
	.ascii	"netbsd\0\0"		# desc (padded to % 4 == 0)

.section	.ctors,"aw",@progbits
	.align 3
	.type	 __CTOR_LIST__,@object
	.size	 __CTOR_LIST__,8
__CTOR_LIST__:
	.quad -1
.section	.dtors,"aw",@progbits
	.align 3
	.type	 __DTOR_LIST__,@object
	.size	 __DTOR_LIST__,8
__DTOR_LIST__:
	.quad -1
.text
	.align 3
	.ent __dtors
__dtors:
	ldgp $29,0($27)
__dtors..ng:
	lda $30,-32($30)
	.frame $30,32,$26,0
	stq $26,0($30)
	stq $9,8($30)
	stq $10,16($30)
	.mask 0x4000600,-32
	.prologue 1
	lda $2,__DTOR_LIST__
	ldq $9,0($2)
	addq $9,1,$1
	bne $1,$34
	bis $31,1,$9
	ldq $1,8($2)
	beq $1,$36
	.align 5
$37:
	addq $9,1,$9
	s8addq $9,$2,$1
	ldq $1,0($1)
	bne $1,$37
$36:
	subq $9,1,$9
$34:
	lda $1,__DTOR_LIST__
	s8addq $9,$1,$10
	br $31,$44
	.align 4
	.align 5
$42:
	ldq $27,0($10)
	subq $10,8,$10
	jsr $26,($27),0
	ldgp $29,0($26)
$44:
	subq $9,1,$9
	addq $9,1,$1
	bne $1,$42
	ldq $26,0($30)
	ldq $9,8($30)
	ldq $10,16($30)
	addq $30,32,$30
	ret $31,($26),1
	.end __dtors
	.align 3
	.ent __ctors
__ctors:
	ldgp $29,0($27)
__ctors..ng:
	lda $30,-16($30)
	.frame $30,16,$26,0
	stq $26,0($30)
	stq $9,8($30)
	.mask 0x4000200,-16
	.prologue 1
	lda $1,__CTOR_LIST__
	addq $1,8,$9
	ldq $1,8($1)
	beq $1,$47
	.align 5
$48:
	ldq $27,0($9)
	addq $9,8,$9
	jsr $26,($27),0
	ldgp $29,0($26)
	ldq $1,0($9)
	bne $1,$48
$47:
	ldq $26,0($30)
	ldq $9,8($30)
	addq $30,16,$30
	ret $31,($26),1
	.end __ctors
.data
	.align 2
	.type	 initialized.6,@object
	.size	 initialized.6,4
initialized.6:
	.long 0
.section	.init,"ax",@progbits
	.align 3
	.globl _init
	.ent _init
_init:
	ldgp $29,0($27)
_init..ng:
	lda $30,-16($30)
	.frame $30,16,$26,0
	stq $26,0($30)
	.mask 0x4000000,-16
	.prologue 1
	lda $2,initialized.6
	ldl $1,0($2)
	bne $1,$51
	bis $31,1,$1
	stl $1,0($2)
	bsr $26,__ctors..ng
$51:
	ldq $26,0($30)
	addq $30,16,$30
	ret $31,($26),1
	.end _init
.section	.fini,"ax",@progbits
	.align 3
	.globl _fini
	.ent _fini
_fini:
	ldgp $29,0($27)
_fini..ng:
	lda $30,-16($30)
	.frame $30,16,$26,0
	stq $26,0($30)
	.mask 0x4000000,-16
	.prologue 1
	bsr $26,__dtors..ng
	ldq $26,0($30)
	addq $30,16,$30
	ret $31,($26),1
	.end _fini
	.ident	"GCC: (GNU) 2.7.2.2"
