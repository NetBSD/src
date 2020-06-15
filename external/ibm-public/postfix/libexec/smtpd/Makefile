#	$NetBSD: Makefile,v 1.9 2020/06/15 01:57:31 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtpd

TOP=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist
DIST=${TOP}/src/${PROG}
.PATH:	${DIST}

SRCS+= \
pfilter.c \
smtpd.c \
smtpd_chat.c \
smtpd_check.c \
smtpd_dsn_fix.c \
smtpd_expand.c \
smtpd_haproxy.c \
smtpd_milter.c \
smtpd_peer.c \
smtpd_proxy.c \
smtpd_resolve.c \
smtpd_sasl_glue.c \
smtpd_sasl_proto.c \
smtpd_state.c \
smtpd_token.c \
smtpd_xforward.c

DPADD+= ${LIBPMASTER} ${LIBPMILTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}
LDADD+= ${LIBPMASTER} ${LIBPMILTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}

DPADD+=	${LIBBLOCKLIST}
LDADD+=	-lblocklist

DPADD+=	${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+=	${LIBPTLS} -lssl -lcrypto

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
