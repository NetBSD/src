#	$NetBSD: Makefile,v 1.2 2001/03/13 18:13:23 itojun Exp $

PROG=	smtp

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	smtp.c smtp_connect.c smtp_proto.c smtp_chat.c smtp_session.c \
	smtp_addr.c smtp_trouble.c smtp_unalias.c smtp_state.c \
	smtp_sasl_proto.c smtp_sasl_glue.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
