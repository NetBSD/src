#	$NetBSD: Makefile,v 1.5 2017/08/01 20:07:38 joerg Exp $

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
	BinaryFormat \
	MC \
	MCParser \
	Support \
	Demangle

LDADD+=	-lz
DPADD+=	${LIBZ}

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
