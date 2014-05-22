#	$NetBSD: Makefile,v 1.1.6.2 2014/05/22 14:08:04 yamt Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postscreen

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

SRCS=	postscreen.c postscreen_dict.c postscreen_dnsbl.c \
	postscreen_early.c postscreen_expand.c postscreen_misc.c \
	postscreen_send.c postscreen_smtpd.c postscreen_starttls.c \
	postscreen_state.c postscreen_tests.c postscreen_haproxy.c \
	postscreen_endpt.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.if ${MKCRYPTO} != "no"
DPADD+= ${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+= ${LIBPTLS} -lssl -lcrypto
.endif

.include <bsd.prog.mk>
