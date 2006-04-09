#	$NetBSD: Makefile,v 1.2 2006/04/09 18:54:28 christos Exp $

.if (${MACHINE_ARCH} == "i386")
PROG=	pmc
WARNS?= 4

LDADD+=	-li386
DPADD+=	${LIBI386}
.else
MAN=	pmc.1
.endif

.include <bsd.prog.mk>
