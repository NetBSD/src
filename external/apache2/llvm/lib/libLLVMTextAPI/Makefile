#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:10:10 cjep Exp $

LIB=	LLVMTextAPI

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/TextAPI
SRCS+=	Architecture.cpp \
	ArchitectureSet.cpp \
	InterfaceFile.cpp \
	PackedVersion.cpp \
	Platform.cpp \
	Symbol.cpp \
	Target.cpp \
	TextStub.cpp \
	TextStubCommon.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
