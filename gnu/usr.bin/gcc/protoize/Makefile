#	$NetBSD: Makefile,v 1.1 2001/12/19 23:54:29 tv Exp $

.include <bsd.own.mk>

PROG=           gcc
SRCS=		protoize.c ${G_PROTO_OBJS:.o=.c}

NOMAN=		# defined

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
