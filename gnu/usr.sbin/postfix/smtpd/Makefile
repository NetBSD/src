#	$NetBSD: Makefile,v 1.3 2001/12/12 01:49:13 tv Exp $

PROG=	smtpd

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_LIBEXECDIR}

SRCS=	smtpd.c smtpd_token.c smtpd_check.c smtpd_chat.c smtpd_state.c \
	smtpd_peer.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPDNS} ${LIBPUTIL}

NOMAN=# defined

.include <bsd.prog.mk>
