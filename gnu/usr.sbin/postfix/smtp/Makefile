#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:13 tv Exp $

PROG=	smtp

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	smtp.c smtp_connect.c smtp_proto.c smtp_chat.c smtp_session.c \
	smtp_addr.c smtp_trouble.c smtp_unalias.c smtp_state.c \
	smtp_sasl_proto.c smtp_sasl_glue.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
