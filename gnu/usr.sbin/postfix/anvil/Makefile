#	$NetBSD: Makefile,v 1.1 2005/08/18 22:30:37 rpaulo Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	anvil

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
