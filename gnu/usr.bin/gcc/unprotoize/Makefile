#	$NetBSD: Makefile,v 1.3 2001/12/21 08:06:15 thorpej Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=           unprotoize
SRCS=		unprotoize.c ${G_PROTO_OBJS:.o=.c}

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
