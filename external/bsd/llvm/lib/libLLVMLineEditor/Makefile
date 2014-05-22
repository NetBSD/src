#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMLineEditor

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/LineEditor

SRCS+=	LineEditor.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
