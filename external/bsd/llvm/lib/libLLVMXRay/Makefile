#	$NetBSD: Makefile,v 1.2 2017/08/01 20:07:42 joerg Exp $

LIB=	LLVMXRay

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/XRay

SRCS+=	InstrumentationMap.cpp \
	Trace.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
