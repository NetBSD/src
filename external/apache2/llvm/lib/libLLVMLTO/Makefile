#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:07 joerg Exp $

LIB=	LLVMLTO

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/LTO

SRCS+=	Caching.cpp \
	LTOBackend.cpp \
	LTOCodeGenerator.cpp \
	LTO.cpp \
	LTOModule.cpp \
	SummaryBasedOptimizations.cpp \
	ThinLTOCodeGenerator.cpp \
	UpdateCompilerUsed.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
