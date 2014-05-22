#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:13:22 yamt Exp $

PROG_CXX=	llvm-as
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-as

SRCS=	llvm-as.cpp

LLVM_LIBS+= \
	AsmParser \
	BitWriter \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
