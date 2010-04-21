#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:29 matt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	local

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	alias.c command.c dotforward.c file.c forward.c \
	include.c indirect.c local.c mailbox.c recipient.c resolve.c token.c \
	deliver_attr.c maildir.c biff_notify.c unknown.c \
	local_expand.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
