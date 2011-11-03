#	$NetBSD: Makefile,v 1.2 2011/11/03 17:52:53 joerg Exp $

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
