#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:24 tls Exp $

LIB=	LLVMLineEditor

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/LineEditor

SRCS+=	LineEditor.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
