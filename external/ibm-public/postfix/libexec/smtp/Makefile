#	$NetBSD: Makefile,v 1.4 2020/03/18 19:05:23 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtp

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS+= \
smtp.c \
smtp_addr.c \
smtp_chat.c \
smtp_connect.c \
smtp_key.c \
smtp_map11.c \
smtp_misc.c \
smtp_proto.c \
smtp_rcpt.c \
smtp_reuse.c \
smtp_sasl_auth_cache.c \
smtp_sasl_glue.c \
smtp_sasl_proto.c \
smtp_session.c \
smtp_state.c \
smtp_tls_policy.c \
smtp_trouble.c \
smtp_unalias.c

LINKS+=	 ${PFIX_LIBEXECDIR}/smtp ${PFIX_LIBEXECDIR}/lmtp

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}

DPADD+=	${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+=	${LIBPTLS} -lssl -lcrypto

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
