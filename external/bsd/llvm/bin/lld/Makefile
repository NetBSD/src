#	$NetBSD: Makefile,v 1.7 2017/01/11 12:02:25 joerg Exp $

PROG_CXX=	lld
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLD_SRCDIR}/tools/lld

SRCS=	lld.cpp

LLD_LIBS+= \
	lldDriver \
	lldReaderWriterELF \
	lldReaderWriterELFAArch64 \
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
	lldConfig

LLVM_LIBS+= \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	DebugInfo \
	Option \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
