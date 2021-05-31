#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:07:00 cjep Exp $

PROG_CXX=	sanstats
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/sanstats

SRCS=	sanstats.cpp

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
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
