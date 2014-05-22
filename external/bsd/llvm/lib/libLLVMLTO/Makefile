#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:14:42 yamt Exp $

LIB=	LLVMLTO

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/LTO

SRCS+=	LTOCodeGenerator.cpp \
	LTOModule.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
