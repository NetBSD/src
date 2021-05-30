#	$NetBSD: Makefile,v 1.3 2021/05/30 01:56:49 joerg Exp $

PROG_CXX=	llvm-symbolizer
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-symbolizer

SRCS=	llvm-symbolizer.cpp

LLVM_LIBS+= \
	DebugInfoSymbolize \
	DebugInfoDWARF \
	DebugInfoPDB \
	DebugInfoMSF \
	DebugInfoCodeView \
	Object \
	TextAPI \
	MCParser \
	MC \
	BitReader \
	BitstreamReader \
	IR \
	Remarks \
	BinaryFormat \
	ObjectYAML \
	TextAPI \
	Option \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

TABLEGEN_SRC=	Opts.td
TABLEGEN_OUTPUT.Opts.td= \
	Opts.inc|-gen-opt-parser-defs

.include "${.PARSEDIR}/../../tablegen.mk"
.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
