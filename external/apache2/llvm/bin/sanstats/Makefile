#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:01 joerg Exp $

PROG_CXX=	sanstats
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/sanstats

SRCS=	sanstats.cc

LLVM_LIBS+= \
	DebugInfoSymbolize \
	DebugInfoDWARF \
	DebugInfoPDB \
	DebugInfoPDBNative \
	Object \
	MCParser \
	MC \
	BitReader \
	BitstreamReader \
	IR \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
