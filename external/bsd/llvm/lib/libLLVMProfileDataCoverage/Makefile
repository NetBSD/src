#	$NetBSD: Makefile,v 1.1.4.2 2017/03/20 06:56:01 pgoyette Exp $

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
