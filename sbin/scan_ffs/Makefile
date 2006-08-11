# $NetBSD: Makefile,v 1.7.4.2 2006/08/11 04:41:25 riz Exp $

PROG=	scan_ffs

LINKS=	${BINDIR}/scan_ffs ${BINDIR}/scan_lfs
MLINKS=	scan_ffs.8 scan_lfs.8

WARNS=	4
LDADD=	-lutil
DPADD=	${LIBUTIL}

MAN=	scan_ffs.8

.include <bsd.prog.mk>
