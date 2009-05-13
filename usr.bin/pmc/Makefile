#	$NetBSD: Makefile,v 1.2.30.1 2009/05/13 19:20:01 jym Exp $

.if (${MACHINE_ARCH} == "i386")
PROG=	pmc

LDADD+=	-li386
DPADD+=	${LIBI386}
.else
MAN=	pmc.1
.endif

.include <bsd.prog.mk>
