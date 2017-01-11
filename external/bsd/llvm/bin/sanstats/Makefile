#	$NetBSD: Makefile,v 1.1 2017/01/11 12:02:27 joerg Exp $

PROG_CXX=	sanstats
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/sanstats

SRCS=	sanstats.cc

LLVM_LIBS+= \
	DebugInfoSymbolize \
	DebugInfoDWARF \
	DebugInfoPDB \
	DebugInfoPDBRaw \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

LDADD+=	-lz
.if !defined(HOSTPROG)
DPADD+=	${LIBZ}
.endif

.include <bsd.prog.mk>
