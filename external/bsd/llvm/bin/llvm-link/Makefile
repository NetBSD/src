#	$NetBSD: Makefile,v 1.6 2017/01/11 12:02:26 joerg Exp $

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
