#	$NetBSD: Makefile,v 1.6 2003/07/22 13:58:49 lukem Exp $

PROG=	stat

.if !defined(HOSTPROG)
LINKS=	${BINDIR}/stat ${BINDIR}/readlink
MLINKS=	stat.1 readlink.1
.else
CPPFLAGS+=-DHOSTPROG
.endif

.include <bsd.prog.mk>
