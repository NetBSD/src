#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:54 tron Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postkick

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_SBINDIR}           

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

.include <bsd.prog.mk>
