#	$NetBSD: Makefile,v 1.1 2013/09/02 14:34:55 joerg Exp $

PROG_CXX=	lld
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/tools/lld

SRCS=	lld.cpp

LLD_LIBS+= \
	lldDriver \
	lldReaderWriterELF \
	lldReaderWriterELFHexagon \
	lldReaderWriterELFPPC \
	lldReaderWriterELFX86 \
	lldReaderWriterELFX86_64 \
	lldReaderWriterMachO \
	lldReaderWriterNative \
	lldReaderWriterPECOFF \
	lldReaderWriter \
	lldReaderWriterYAML \
	lldPasses \
	lldCore

LLVM_LIBS+= \
	Object \
	Option \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
