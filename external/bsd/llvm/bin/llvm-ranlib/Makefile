#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:47 joerg Exp $

PROG_CXX=	llvm-ranlib
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-ranlib

SRCS=	llvm-ranlib.cpp

LLVM_LIBS+= \
	Archive \
	BitReader \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
