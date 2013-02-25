#	$NetBSD: Makefile,v 1.1.12.1 2013/02/25 00:26:34 tls Exp $

PROG_CXX=	llvm-ranlib
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-ranlib

SRCS=	llvm-ranlib.cpp

LLVM_LIBS+= \
	Archive \
	BitReader \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
