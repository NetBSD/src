#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:52 joerg Exp $

LIB=	LLVMDemangle

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/Demangle

SRCS+=	Demangle.cpp \
	ItaniumDemangle.cpp \
	MicrosoftDemangle.cpp \
	MicrosoftDemangleNodes.cpp \
	RustDemangle.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
