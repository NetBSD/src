#	$NetBSD: Makefile,v 1.4 2002/09/19 03:09:45 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	lmtp

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	lmtp.c lmtp_connect.c lmtp_proto.c lmtp_chat.c lmtp_session.c \
	lmtp_addr.c lmtp_trouble.c lmtp_state.c lmtp_sasl_glue.c \
	lmtp_sasl_proto.c

DPADD+=	${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

.include <bsd.prog.mk>
