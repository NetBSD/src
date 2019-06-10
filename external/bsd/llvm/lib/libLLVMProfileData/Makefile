#	$NetBSD: Makefile,v 1.4.14.1 2019/06/10 21:50:54 christos Exp $

LIB=	LLVMProfileData

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ProfileData

SRCS+=	GCOV.cpp \
	InstrProf.cpp \
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
