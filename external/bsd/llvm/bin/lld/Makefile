#	$NetBSD: Makefile,v 1.4 2014/05/30 18:28:13 joerg Exp $

PROG_CXX=	lld
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/tools/lld

SRCS=	lld.cpp

LLD_LIBS+= \
	lldDriver \
	lldReaderWriterELF \
	lldReaderWriterELFHexagon \
	lldReaderWriterELFMips \
	lldReaderWriterELFPPC \
	lldReaderWriterELFX86 \
	lldReaderWriterELFX86_64 \
	lldReaderWriterMachO \
	lldReaderWriterPECOFF \
	lldReaderWriter \
	lldReaderWriterYAML \
	lldPasses \
	lldCore \
	lldReaderWriterNative \

LLVM_LIBS+= \
	Object \
	BitReader \
	IR \
	DebugInfo \
	Option \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
