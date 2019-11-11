#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:11 joerg Exp $

LIB=	LLVMProfileDataCoverage

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/lib/ProfileData/Coverage

SRCS+=	CoverageMapping.cpp \
	CoverageMappingReader.cpp \
	CoverageMappingWriter.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
