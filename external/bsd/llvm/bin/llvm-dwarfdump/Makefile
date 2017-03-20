#	$NetBSD: Makefile,v 1.3.2.1 2017/03/20 06:52:28 pgoyette Exp $

PROG_CXX=	llvm-dwarfdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwarfdump

SRCS=	llvm-dwarfdump.cpp

LLVM_LIBS+= \
	DebugInfoDWARF \
	Object \
	BitReader \
	IR \
	MC \
	MCParser \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
