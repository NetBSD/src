#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:07 joerg Exp $

LIB=	LLVMLineEditor

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/LineEditor

SRCS+=	LineEditor.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
