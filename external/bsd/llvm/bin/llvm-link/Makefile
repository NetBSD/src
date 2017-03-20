#	$NetBSD: Makefile,v 1.5.2.1 2017/03/20 06:52:28 pgoyette Exp $

PROG_CXX=	llvm-link
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-link

SRCS=	llvm-link.cpp

LLVM_LIBS+= \
	Object \
	MC \
	IRReader \
	AsmParser \
	BitReader \
	BitWriter \
	ipo \
	Linker \
	TransformsUtils \
	Analysis \
	Target \
	IR \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
