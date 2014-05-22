#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-nm
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-nm

SRCS=	llvm-nm.cpp

LLVM_LIBS+= \
	BitReader \
	IR \
	Object \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
