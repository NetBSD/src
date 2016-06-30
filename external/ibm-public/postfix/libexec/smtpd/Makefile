#	$NetBSD: Makefile,v 1.5 2016/06/30 03:05:45 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	smtpd

TOP=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist
DIST=${TOP}/src/${PROG}
.PATH:	${DIST}

SRCS=	smtpd.c smtpd_token.c smtpd_check.c smtpd_chat.c smtpd_state.c \
	smtpd_peer.c smtpd_sasl_proto.c smtpd_sasl_glue.c smtpd_proxy.c \
	smtpd_xforward.c smtpd_dsn_fix.c smtpd_milter.c smtpd_resolve.c \
	smtpd_expand.c smtpd_haproxy.c

DPADD+= ${LIBPMASTER} ${LIBPMILTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}
LDADD+= ${LIBPMASTER} ${LIBPMILTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPXSASL}

.if ${MKCRYPTO} != "no"
DPADD+=	${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+=	${LIBPTLS} -lssl -lcrypto
.else
.PATH:	${TOP}/src/tls
SRCS+=	tls_level.c
.endif

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
