#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:38 bouyer Exp $

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
