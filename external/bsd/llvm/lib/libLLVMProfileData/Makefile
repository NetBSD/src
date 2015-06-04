#	$NetBSD: Makefile,v 1.2.2.1 2015/06/04 20:05:44 snj Exp $

LIB=	LLVMProfileData

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ProfileData

SRCS+=	CoverageMapping.cpp \
	CoverageMappingReader.cpp \
	CoverageMappingWriter.cpp \
	InstrProf.cpp \
	InstrProfReader.cpp \
	InstrProfWriter.cpp \
	SampleProf.cpp \
	SampleProfReader.cpp \
	SampleProfWriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
