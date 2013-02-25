#	$NetBSD: Makefile,v 1.1.12.1 2013/02/25 00:26:35 tls Exp $

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
