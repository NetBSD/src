#	$NetBSD: Makefile,v 1.4 2017/01/11 12:02:26 joerg Exp $

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
