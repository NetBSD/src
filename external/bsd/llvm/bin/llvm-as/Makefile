#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:45 joerg Exp $

PROG_CXX=	llvm-as
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-as

SRCS=	llvm-as.cpp

LLVM_LIBS+= \
	AsmParser \
	BitWriter \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
