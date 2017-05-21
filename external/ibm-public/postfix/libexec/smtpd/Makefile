#	$NetBSD: Makefile,v 1.6 2017/05/21 15:28:40 riastradh Exp $

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

DPADD+=	${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+=	${LIBPTLS} -lssl -lcrypto

DPADD+=	${LIBPUTIL}
LDADD+=	${LIBPUTIL}

.include <bsd.prog.mk>
