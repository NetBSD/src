#	$NetBSD: crtend.S,v 1.2 1998/05/06 05:32:08 ross Exp $
#
# Warning -- 
#
# See comments in crtbegin.c prior to attempting work on this module.
#
# This module has been modified by hand.
#
	.file	1 "crtend.c"
	.version	"01.01"
	.set noat
gcc2_compiled.:
__gnu_compiled_c:
.section	.ctors,"aw",@progbits
	.align 3
	.type	 __CTOR_LIST__,@object
	.size	 __CTOR_LIST__,8
__CTOR_LIST__:
	.quad 0
.section	.dtors,"aw",@progbits
	.align 3
	.type	 __DTOR_LIST__,@object
	.size	 __DTOR_LIST__,8
__DTOR_LIST__:
	.quad 0
	.ident	"GCC: (GNU) 2.7.2.2"
#
# Now catch fallthru from crtbegin and any inline code
#
.section	.init,"ax",@progbits
	.globl	You_did_not_link_crtend	# it is absolutely critical that
You_did_not_link_crtend:		# ...crtbegin be paired with
	br	$26,L1			# ...this module!
L1:	ldq	$26,L2-L1($26)
	jmp	$31,($26)
	.globl	_init_return
L2:	.long	_init_return
#
# And do the fallthru trap for both .init and .fini
#
.section	.fini,"ax",@progbits
	br	$26,L3
L3:	ldq	$26,L4-L3($26)
	jmp	$31,($26)
	.globl	_fini_return
L4:	.long	_fini_return
	.globl	You_did_not_link_crtbegin
	.long	You_did_not_link_crtbegin
