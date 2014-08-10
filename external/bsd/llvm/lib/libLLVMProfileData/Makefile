#	$NetBSD: Makefile,v 1.1.2.2 2014/08/10 07:09:09 tls Exp $

LIB=	LLVMProfileData

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ProfileData

SRCS+=	InstrProf.cpp \
	InstrProfReader.cpp \
	InstrProfWriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
