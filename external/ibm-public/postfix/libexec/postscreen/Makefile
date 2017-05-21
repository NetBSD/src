#	$NetBSD: Makefile,v 1.4 2017/05/21 15:28:40 riastradh Exp $

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

DPADD+= ${LIBPTLS} ${LIBSSL} ${LIBCRYPTO}
LDADD+= ${LIBPTLS} -lssl -lcrypto

.include <bsd.prog.mk>
