#	$NetBSD: Makefile,v 1.1.6.1 2011/11/10 14:31:23 yamt Exp $

LIB=	LLVMLinker

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Linker

SRCS+=	LinkArchives.cpp \
	LinkItems.cpp \
	LinkModules.cpp \
	Linker.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
