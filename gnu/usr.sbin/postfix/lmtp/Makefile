#	$NetBSD: Makefile,v 1.2 2001/03/15 12:03:12 itojun Exp $

PROG=	lmtp

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	lmtp.c lmtp_connect.c lmtp_proto.c lmtp_chat.c lmtp_session.c \
	lmtp_addr.c lmtp_trouble.c lmtp_state.c lmtp_sasl_glue.c \
	lmtp_sasl_proto.c

DPADD+=	${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

NOMAN=1

.include <bsd.prog.mk>
