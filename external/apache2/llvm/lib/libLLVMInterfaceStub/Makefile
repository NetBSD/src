#	$NetBSD: Makefile,v 1.1 2021/05/30 01:56:53 joerg Exp $

LIB=	LLVMInterfaceStub

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/InterfaceStub

SRCS+=	ELFObjHandler.cpp \
	ELFStub.cpp \
	TBEHandler.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
