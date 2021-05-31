#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:08 cjep Exp $

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
