#	$NetBSD: Makefile,v 1.3.14.1 2017/03/20 06:52:28 pgoyette Exp $

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
