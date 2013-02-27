#	$NetBSD: Makefile,v 1.3 2013/02/27 21:25:08 joerg Exp $

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

checkout-mclinker:
	if [ -d ${MCLINKER_SRCDIR}/.git ]; then \
		cd ${MCLINKER_SRCDIR}; git pull ${MCLINKER_ROOT} ; \
	else \
		git clone ${MCLINKER_ROOT} ${MCLINKER_SRCDIR}; \
	fi
	cd ${MCLINKER_SRCDIR} && git checkout ${MCLINKER_REVISION}

.include <bsd.subdir.mk>
