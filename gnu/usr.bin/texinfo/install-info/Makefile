#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:29 lukem Exp $

PROG=		install-info
SRCS=		install-info.c
LDADD+=		-lz
DPADD+=		${LIBZ}

MKMAN=		no

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/util
