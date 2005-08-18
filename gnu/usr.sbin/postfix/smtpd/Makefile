#	$NetBSD: Makefile,v 1.7 2005/08/18 22:30:37 rpaulo Exp $

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
DPADD+=	${LIBPTLS} ${LIBSSL}
LDADD+=	${LIBPTLS} -lssl
.endif

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
