#	$NetBSD: Makefile,v 1.1 2018/12/31 19:33:05 maya Exp $

.include <bsd.init.mk>

.PATH:	${LLVM_SRCDIR}/include/llvm/ProfileData

SUBDIR+=	Coverage

INCS=	GCOV.h \
	InstrProf.h \
	InstrProfData.inc \
	InstrProfReader.h \
	InstrProfWriter.h \
	ProfileCommon.h \
	SampleProf.h \
	SampleProfReader.h \
	SampleProfWriter.h


INCSDIR=	/usr/include/llvm/ProfileData

.include <bsd.subdir.mk>
.include <bsd.prog.mk>
