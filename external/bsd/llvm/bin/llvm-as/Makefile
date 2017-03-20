#	$NetBSD: Makefile,v 1.2.14.1 2017/03/20 06:52:27 pgoyette Exp $

PROG_CXX=	llvm-as
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-as

SRCS=	llvm-as.cpp

LLVM_LIBS+= \
	AsmParser \
	BitWriter \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
