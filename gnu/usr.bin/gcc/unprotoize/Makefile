#	$NetBSD: Makefile,v 1.2 2001/12/21 08:03:36 thorpej Exp $

.include <bsd.own.mk>

PROG=           unprotoize
SRCS=		unprotoize.c ${G_PROTO_OBJS:.o=.c}

NOMAN=		# defined

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
