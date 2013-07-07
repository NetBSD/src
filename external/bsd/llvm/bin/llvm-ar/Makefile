#	$NetBSD: Makefile,v 1.3 2013/07/07 21:36:43 joerg Exp $

PROG_CXX=	llvm-ar
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/llvm-ar

SRCS=	llvm-ar.cpp \
	Archive.cpp \
	ArchiveReader.cpp \
	ArchiveWriter.cpp

LLVM_LIBS+= \
	BitReader \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
