#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:01 pgoyette Exp $

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
