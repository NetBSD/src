#	$NetBSD: Makefile,v 1.1 2000/04/30 21:21:59 perry Exp $

PROG=	local

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	alias.c command.c delivered.c dotforward.c file.c forward.c \
	include.c indirect.c local.c mailbox.c recipient.c resolve.c token.c \
	deliver_attr.c maildir.c biff_notify.c unknown.c \
	local_expand.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
