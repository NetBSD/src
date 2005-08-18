#	$NetBSD: Makefile,v 1.7 2005/08/18 22:30:37 rpaulo Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtp

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	smtp.c smtp_connect.c smtp_proto.c smtp_chat.c smtp_session.c \
	smtp_addr.c smtp_trouble.c smtp_state.c smtp_rcpt.c \
	smtp_sasl_proto.c smtp_sasl_glue.c smtp_reuse.c smtp_map11.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS}

.if ${MKCRYPTO} != "no"
DPADD+=	${LIBPTLS} ${LIBSSL}
LDADD+=	${LIBPTLS} -lssl
.endif

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
