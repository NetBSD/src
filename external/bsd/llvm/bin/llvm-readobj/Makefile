#	$NetBSD: Makefile,v 1.1.4.1 2013/02/25 00:26:34 tls Exp $

PROG_CXX=	llvm-readobj
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-readobj

SRCS=	llvm-readobj.cpp

LLVM_LIBS+= \
	Archive \
	BitReader \
	Object \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
