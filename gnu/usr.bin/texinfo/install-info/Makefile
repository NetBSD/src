#	$NetBSD: Makefile,v 1.1 1999/02/11 15:42:15 tv Exp $

PROG=		install-info
SRCS=		install-info.c
LDADD+=		-lz
DPADD+=		${LIBZ}

NOMAN=

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/util
