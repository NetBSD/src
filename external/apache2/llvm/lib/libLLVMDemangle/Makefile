#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:07 cjep Exp $

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
