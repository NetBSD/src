#	$NetBSD: Makefile,v 1.4 2017/01/11 12:02:26 joerg Exp $

PROG_CXX=	llvm-dis
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dis

SRCS=	llvm-dis.cpp

LLVM_LIBS+= \
	Analysis \
	BitReader \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
