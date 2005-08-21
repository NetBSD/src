#	$NetBSD: Makefile,v 1.8 2005/08/21 23:00:12 he Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtpd

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_LIBEXECDIR}

SRCS=	smtpd.c smtpd_token.c smtpd_check.c smtpd_chat.c smtpd_state.c \
	smtpd_peer.c smtpd_sasl_proto.c smtpd_sasl_glue.c smtpd_proxy.c \
	smtpd_xforward.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} 
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS}

.if ${MKCRYPTO} != "no"
DPADD+=	${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+=	${LIBPTLS} -lssl -lcrypto
.endif

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
