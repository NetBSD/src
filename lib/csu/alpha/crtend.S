#	$NetBSD: crtend.S,v 1.1 1998/05/06 00:59:18 ross Exp $
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
