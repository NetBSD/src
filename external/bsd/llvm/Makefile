#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:43 joerg Exp $

SUBDIR=	include \
	.WAIT \
	lib \
	.WAIT \
	bin

.include "Makefile.inc"

checkout:
	svn co -r ${LLVM_REVISION} http://llvm.org/svn/llvm-project/llvm/trunk ${LLVM_SRCDIR}
	svn co -r ${CLANG_REVISION} http://llvm.org/svn/llvm-project/cfe/trunk ${CLANG_SRCDIR}

.include <bsd.subdir.mk>
