#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:08 tv Exp $

PROG=	lmtp

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	lmtp.c lmtp_connect.c lmtp_proto.c lmtp_chat.c lmtp_session.c \
	lmtp_addr.c lmtp_trouble.c lmtp_state.c lmtp_sasl_glue.c \
	lmtp_sasl_proto.c

DPADD+=	${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
