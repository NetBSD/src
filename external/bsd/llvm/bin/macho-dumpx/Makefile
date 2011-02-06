#	$NetBSD: Makefile,v 1.1 2011/02/06 01:13:48 joerg Exp $

ROG_CXX=	machdo-dumpx
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/macho-dump

SRCS=	macho-dump.cpp

LLVM_LIBS+= \
	Object \
	Core \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
