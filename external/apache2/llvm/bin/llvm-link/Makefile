#	$NetBSD: Makefile,v 1.1 2019/11/11 22:45:00 joerg Exp $

PROG_CXX=	llvm-link
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-link

SRCS=	llvm-link.cpp

LLVM_LIBS+= \
	IRReader \
	AsmParser \
	BitReader \
	BitstreamReader \
	BitWriter \
	Object \
	MCParser \
	MC \
	ipo \
	Linker \
	TransformsUtils \
	Analysis \
	Target \
	IR \
	BinaryFormat \
	Support \
	Demangle

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
