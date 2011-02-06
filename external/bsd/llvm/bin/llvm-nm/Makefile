#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:47 joerg Exp $

PROG_CXX=	llvm-nm
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-nm

SRCS=	llvm-nm.cpp

LLVM_LIBS+= \
	Archive \
	BitReader \
	Core \
	Object \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
