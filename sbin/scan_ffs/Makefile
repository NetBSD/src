# $NetBSD: Makefile,v 1.7.4.3 2006/08/21 20:22:50 tron Exp $

PROG=	scan_ffs

LINKS=	${BINDIR}/scan_ffs ${BINDIR}/scan_lfs
MLINKS=	scan_ffs.8 scan_lfs.8

.include <bsd.own.mk>

# Avoid -std=c99 on vax which still uses gcc 2.95.3.
.if ${MACHINE} == "vax"
WARNS=	3
.else
WARNS=	4
.endif

LDADD=	-lutil
DPADD=	${LIBUTIL}

MAN=	scan_ffs.8

.include <bsd.prog.mk>
