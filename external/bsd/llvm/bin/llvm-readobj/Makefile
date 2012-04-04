#	$NetBSD: Makefile,v 1.1 2012/04/04 10:44:21 joerg Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp

LLVM_LIBS+= \
	Archive \
	BitReader \
	Object \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
