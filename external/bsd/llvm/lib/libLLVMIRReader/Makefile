#	$NetBSD: Makefile,v 1.1.4.2 2013/06/23 06:28:23 tls Exp $

LIB=	LLVMIRReader

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/IRReader

SRCS+=	IRReader.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
