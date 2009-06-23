#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:52 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	local

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	alias.c command.c dotforward.c file.c forward.c \
	include.c indirect.c local.c mailbox.c recipient.c resolve.c token.c \
	deliver_attr.c maildir.c biff_notify.c unknown.c \
	local_expand.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
