#	$NetBSD: Makefile,v 1.1.2.2 2021/05/31 22:10:08 cjep Exp $

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
