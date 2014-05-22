#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-dis
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dis

SRCS=	llvm-dis.cpp

LLVM_LIBS+= \
	Analysis \
	BitReader \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
