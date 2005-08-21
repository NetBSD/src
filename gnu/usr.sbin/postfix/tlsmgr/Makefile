#	$NetBSD: Makefile,v 1.2 2005/08/21 20:26:39 he Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	tlsmgr

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR=	${PFIX_LIBEXECDIR}

DPADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPTLS} ${LIBCRYPTO} ${LIBPUTIL}
LDADD+= ${LIBPMASTER} ${LIBPGLOBAL} ${LIBPTLS} ${LIBCRYPTO} ${LIBPUTIL} -lssl

.include <bsd.prog.mk>
