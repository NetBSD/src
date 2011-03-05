#	$NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:08:47 bouyer Exp $

LIB=	LLVMX86Utils

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Target/X86/Utils

SRCS+=	X86ShuffleDecode.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
