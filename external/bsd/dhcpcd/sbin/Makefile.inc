# $NetBSD: Makefile.inc,v 1.1 2008/07/27 19:31:03 joerg Exp $

.include <bsd.own.mk>

WARNS?=	3
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

