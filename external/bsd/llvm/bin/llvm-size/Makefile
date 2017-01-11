#	$NetBSD: Makefile,v 1.3 2017/01/11 12:02:27 joerg Exp $

PROG_CXX=	llvm-size
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-size

SRCS=	llvm-size.cpp

LLVM_LIBS+= \
	Object \
	MCParser \
	MC \
	BitReader \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
