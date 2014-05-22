#	$NetBSD: Makefile,v 1.1.10.2 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMIRReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/IRReader

SRCS+=	IRReader.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
