#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 16:13:23 yamt Exp $

PROG_CXX=	llvm-profdata
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-profdata

SRCS=	llvm-profdata.cpp

LLVM_LIBS+= \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
