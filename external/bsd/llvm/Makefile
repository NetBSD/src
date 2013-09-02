#	$NetBSD: Makefile,v 1.4 2013/09/02 14:34:55 joerg Exp $

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
	svn co -r ${CLANG_REVISION} ${SVN_ROOT}/cfe/trunk ${CLANG_SRCDIR}
	svn co -r ${COMPILER_RT_REVISION} ${SVN_ROOT}/compiler-rt/trunk \
	    ${COMPILER_RT_SRCDIR}
	svn co -r ${LLD_REVISION} ${SVN_ROOT}/lld/trunk ${LLD_SRCDIR}
	svn co -r ${LLVM_REVISION} ${SVN_ROOT}/llvm/trunk ${LLVM_SRCDIR}

checkout-mclinker:
	if [ -d ${MCLINKER_SRCDIR}/.git ]; then \
		cd ${MCLINKER_SRCDIR}; git pull ${MCLINKER_ROOT} ; \
	else \
		git clone ${MCLINKER_ROOT} ${MCLINKER_SRCDIR}; \
	fi
	cd ${MCLINKER_SRCDIR} && git checkout ${MCLINKER_REVISION}

.include <bsd.subdir.mk>
