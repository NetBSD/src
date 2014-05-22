#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-ar
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-ar

SRCS=	llvm-ar.cpp

LLVM_LIBS+= \
	Object \
	BitReader \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
