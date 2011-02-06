#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:46 joerg Exp $

PROG_CXX=	llvm-dis
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dis

SRCS=	llvm-dis.cpp

LLVM_LIBS+= \
	BitReader \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
