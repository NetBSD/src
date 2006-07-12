#	$NetBSD: Makefile,v 1.1.2.2 2006/07/12 15:06:48 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	discard

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

SRCS	= discard.c

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
