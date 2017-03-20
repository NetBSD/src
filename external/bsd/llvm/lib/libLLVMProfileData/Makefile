#	$NetBSD: Makefile,v 1.3.2.1 2017/03/20 06:56:01 pgoyette Exp $

LIB=	LLVMProfileData

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ProfileData

SRCS+=	InstrProf.cpp \
	InstrProfReader.cpp \
	InstrProfWriter.cpp \
	ProfileSummaryBuilder.cpp \
	SampleProf.cpp \
	SampleProfReader.cpp \
	SampleProfWriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
