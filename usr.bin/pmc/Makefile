#	$NetBSD: Makefile,v 1.1 2000/10/24 23:54:31 thorpej Exp $

.if (${MACHINE_ARCH} == "i386")
PROG=	pmc

LDADD+=	-li386
DPADD+=	${LIBI386}
.else
MAN=	pmc.1
.endif

.include <bsd.prog.mk>
