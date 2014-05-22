#	$NetBSD: Makefile,v 1.1.6.1 2014/05/22 16:13:23 yamt Exp $

ROG_CXX=	machdo-dumpx
NOMAN=		yes

.include <bsd.init.mk>

.PATH: ${LLVM_SRCDIR}/tools/macho-dump

SRCS=	macho-dump.cpp

LLVM_LIBS+= \
	Object \
	IR \
	Support

.include "${.PARSEDIR}/../../link.mk"

.include <bsd.prog.mk>
