#	$NetBSD: Makefile,v 1.1 2000/04/30 21:22:17 perry Exp $

PROG=	smtp

DIST=	${.CURDIR}/../../../dist/postfix/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	smtp.c quote_821_local.c smtp_connect.c smtp_proto.c smtp_chat.c \
	smtp_session.c smtp_addr.c smtp_trouble.c smtp_unalias.c smtp_state.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
