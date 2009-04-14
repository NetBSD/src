#	$NetBSD: Makefile,v 1.3 2009/04/14 22:15:25 lukem Exp $

.if (${MACHINE_ARCH} == "i386")
PROG=	pmc

LDADD+=	-li386
DPADD+=	${LIBI386}
.else
MAN=	pmc.1
.endif

.include <bsd.prog.mk>
