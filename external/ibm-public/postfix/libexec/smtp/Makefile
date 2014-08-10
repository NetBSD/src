#	$NetBSD: Makefile,v 1.1.26.1 2014/08/10 07:12:50 tls Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtp

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	smtp.c smtp_connect.c smtp_proto.c smtp_chat.c smtp_session.c \
	smtp_addr.c smtp_trouble.c smtp_state.c smtp_rcpt.c \
	smtp_sasl_proto.c smtp_sasl_glue.c smtp_reuse.c smtp_map11.c \
	smtp_sasl_auth_cache.c smtp_key.c smtp_tls_policy.c

LINKS+=	 ${PFIX_LIBEXECDIR}/smtp ${PFIX_LIBEXECDIR}/lmtp

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}

.if ${MKCRYPTO} != "no"
DPADD+=	${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+=	${LIBPTLS} -lssl -lcrypto
.endif

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
