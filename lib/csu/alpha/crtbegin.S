#	$NetBSD: crtbegin.S,v 1.2 1998/05/06 05:32:08 ross Exp $
#
# Warning -- 
#
# This module has been modified by hand from a prototype crtbegin.c
# for the purpose of the following low-level mechanisms:
# 
# [1] Fall through the .init section from _init() to a return block
# in crtend(). This allows compilers for object-oriented languages
# to generate inline initialization code. The mechanism is an
# alternative to the one presently used by g++ for file level
# constructors and destructors. It is apparently the original
# intention of the ELF .init and .fini segments that this option
# be open to compilers. Work based on a suggestion by jtc@netbsd.org.
# 
# [2] The incorrect cross-elf-section bsr was fixed, this had
# the side-effect of limiting program text size to 4 MB.
# 
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
	.globl	You_did_not_link_crtend
	.long	You_did_not_link_crtend
#
# See comment below on _fini_return
#
	.globl	_init_return
_init_return:
You_did_not_link_crtbegin:
	.globl	You_did_not_link_crtbegin
	ldq $26,0($30)
	addq $30,16,$30
	ret $31,($26),1
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
	lda $26,__ctors..ng
	lda $2,initialized.6
	ldl $1,0($2)
	bne $1,_init_return
	bis $31,1,$1
	stl $1,0($2)
	jsr $26,($26),__ctors..ng
	nop
	.end _init
.section	.fini,"ax",@progbits
	.globl	You_did_not_link_crtend	
	.long	You_did_not_link_crtend	
	.align 3
#
# A jump back to _init_return and _fini_return are made here so that
# corresponding stack adjustments are not split over multiple modules.
#
	.globl	_fini_return
_fini_return:
	ldq $26,0($30)
	addq $30,16,$30
	ret $31,($26),1
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
	lda $26,__dtors..ng
	jsr $26,($26),__dtors..ng
	nop
	.end _fini
	.ident	"GCC: (GNU) 2.7.2.2"
