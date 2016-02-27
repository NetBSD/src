#	$NetBSD: Makefile,v 1.5 2016/02/27 21:47:33 joerg Exp $

LIB=	LLVMLinker

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Linker

SRCS+=	IRMover.cpp \
	LinkModules.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
