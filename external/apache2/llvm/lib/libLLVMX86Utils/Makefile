#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:14 joerg Exp $

LIB=	LLVMX86Utils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/Utils

SRCS+=	X86ShuffleDecode.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
