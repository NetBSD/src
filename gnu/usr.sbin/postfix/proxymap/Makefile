#	$NetBSD: Makefile,v 1.1 2003/03/08 19:12:17 perry Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	proxymap

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_LIBEXECDIR}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
