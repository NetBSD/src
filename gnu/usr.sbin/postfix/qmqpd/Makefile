#	$NetBSD: Makefile,v 1.1 2006/04/17 16:11:20 christos Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	qmqpd

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS=	qmqpd.c qmqpd_state.c qmqpd_peer.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
