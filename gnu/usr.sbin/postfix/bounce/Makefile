#	$NetBSD: Makefile,v 1.1 2000/04/30 21:21:53 perry Exp $

PROG=	bounce

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	bounce.c bounce_append_service.c bounce_notify_service.c \
	bounce_cleanup.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
