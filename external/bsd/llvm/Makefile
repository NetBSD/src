#	$NetBSD: Makefile,v 1.2 2012/06/17 16:51:31 joerg Exp $

.include <bsd.init.mk>

SUBDIR=	include \
	.WAIT \
	lib \
	.WAIT \
	bin

.if ${MKCLANGRT:Uno} == "yes"
SUBDIR+=	librt
.endif

.include "Makefile.inc"

checkout:
	svn co -r ${LLVM_REVISION} ${SVN_ROOT}/llvm/trunk ${LLVM_SRCDIR}
	svn co -r ${CLANG_REVISION} ${SVN_ROOT}/cfe/trunk ${CLANG_SRCDIR}
	svn co -r ${COMPILER_RT_REVISION} ${SVN_ROOT}/compiler-rt/trunk \
	    ${COMPILER_RT_SRCDIR}

.include <bsd.subdir.mk>
