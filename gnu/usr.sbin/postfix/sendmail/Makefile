#	$NetBSD: Makefile,v 1.4 2002/09/19 03:09:50 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	sendmail

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
