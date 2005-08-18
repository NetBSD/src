#	$NetBSD: Makefile,v 1.1 2005/08/18 22:30:37 rpaulo Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	tlsmgr

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPTLS} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPTLS} ${LIBPUTIL} -lssl

.include <bsd.prog.mk>
