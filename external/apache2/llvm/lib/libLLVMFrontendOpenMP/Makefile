#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMFrontendOpenMP

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Frontend/OpenMP

SRCS+=	OMP.cpp \
	OMPContext.cpp \
	OMPIRBuilder.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
