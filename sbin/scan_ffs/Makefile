# $NetBSD: Makefile,v 1.7 2005/09/10 23:15:52 xtraeme Exp $

PROG=	scan_ffs

LINKS=	${BINDIR}/scan_ffs ${BINDIR}/scan_lfs
MLINKS=	scan_ffs.8 scan_lfs.8

LDADD=	-lutil
DPADD=	${LIBUTIL}

MAN=	scan_ffs.8

.include <bsd.prog.mk>
