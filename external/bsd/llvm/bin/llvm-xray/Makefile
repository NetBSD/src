#	$NetBSD: Makefile,v 1.2 2017/08/01 20:07:39 joerg Exp $

PROG_CXX=	llvm-xray
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-xray

SRCS=	func-id-helper.cc \
	llvm-xray.cc \
	xray-account.cc \
	xray-color-helper.cc \
	xray-converter.cc \
	xray-extract.cc \
	xray-graph.cc \
	xray-graph-diff.cc \
	xray-registry.cc

LLVM_LIBS+= \
	XRay \
	DebugInfoSymbolize \
	DebugInfoDWARF \
	DebugInfoPDB \
	DebugInfoPDBNative \
	Object \
	BitReader \
	MCParser \
	IR \
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
