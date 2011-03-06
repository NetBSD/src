#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:40 bouyer Exp $

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
