#	$NetBSD: Makefile,v 1.1 2010/06/17 06:10:44 mrg Exp $

NOMAN=	1

PROG=	pigz
SRCS=	pigz.c yarn.c

LDADD+=	-lz -lpthread
DPADD+=	${LIBM} ${LIBPTHREAD}

.include <bsd.prog.mk>

DIST=	${NETBSDSRCDIR}/external/zlib/pigz/dist
.PATH:	${DIST}
