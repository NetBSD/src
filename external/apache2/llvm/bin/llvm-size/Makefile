#	$NetBSD: Makefile,v 1.1.4.1 2021/05/31 22:06:59 cjep Exp $

PROG_CXX=	llvm-size
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-size

SRCS=	llvm-size.cpp

LLVM_LIBS+= \
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

.include <bsd.prog.mk>
