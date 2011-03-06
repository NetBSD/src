#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:42 bouyer Exp $

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
