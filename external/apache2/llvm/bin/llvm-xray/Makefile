#	$NetBSD: Makefile,v 1.2 2021/05/30 01:56:49 joerg Exp $

PROG_CXX=	llvm-xray
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-xray

SRCS=	func-id-helper.cpp \
	llvm-xray.cpp \
	xray-account.cpp \
	xray-color-helper.cpp \
	xray-converter.cpp \
	xray-extract.cpp \
	xray-fdr-dump.cpp \
	xray-graph.cpp \
	xray-graph-diff.cpp \
	xray-registry.cpp \
	xray-stacks.cpp

LLVM_LIBS+= \
	XRay \
	DebugInfoSymbolize \
	DebugInfoDWARF \
	DebugInfoPDB \
	DebugInfoMSF \
	DebugInfoCodeView \
	Object \
	TextAPI \
	BitReader \
	BitstreamReader \
	MCParser \
	IR \
	Remarks \
	MC \
	BinaryFormat \
	Support \
	Demangle

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
