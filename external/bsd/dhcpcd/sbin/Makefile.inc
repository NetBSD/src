# $NetBSD: Makefile.inc,v 1.2.4.2 2008/10/19 22:39:50 haad Exp $

.include <bsd.own.mk>

WARNS?=		4
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

