#	$NetBSD: Makefile,v 1.1 2011/10/11 13:53:57 joerg Exp $

PROG_CXX=	llvm-dwarfdump
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-dwarfdump

SRCS=	llvm-dwarfdump.cpp

LLVM_LIBS+= \
	DebugInfo \
	Object \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
