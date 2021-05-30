#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMFileCheck

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/FileCheck

SRCS+=	FileCheck.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
