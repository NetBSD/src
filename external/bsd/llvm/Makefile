#	$NetBSD: Makefile,v 1.1.6.1 2012/10/30 18:55:47 yamt Exp $

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
