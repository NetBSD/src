#	$NetBSD: Makefile,v 1.1.2.3 2010/11/21 18:31:38 riz Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	local

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	alias.c command.c dotforward.c file.c forward.c \
	include.c indirect.c local.c mailbox.c recipient.c resolve.c token.c \
	deliver_attr.c maildir.c biff_notify.c unknown.c \
	local_expand.c bounce_workaround.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
