#	$NetBSD: Makefile,v 1.1 2017/01/11 12:02:31 joerg Exp $

LIB=	LLVMTransformsCoroutines

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Transforms/Coroutines

SRCS+=	CoroCleanup.cpp \
	CoroEarly.cpp \
	CoroElide.cpp \
	CoroFrame.cpp \
	CoroSplit.cpp \
	Coroutines.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
