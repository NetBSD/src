#	$NetBSD: Makefile,v 1.1.12.1 2013/02/25 00:26:33 tls Exp $

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
