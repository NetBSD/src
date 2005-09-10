# $NetBSD: Makefile,v 1.6 2005/09/10 23:08:37 wiz Exp $

PROG=	scan_ffs

LINKS=	${BINDIR}/scan_ffs ${BINDIR}/scan_lfs
MLINKS=	scan_ffs.8 scan_lfs.8

LDADD=	-lutil
DPADD=	${LIBUTIL}

.include <bsd.prog.mk>
