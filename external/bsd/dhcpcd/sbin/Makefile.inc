# $NetBSD: Makefile.inc,v 1.1.2.2 2008/09/18 05:14:27 wrstuden Exp $

.include <bsd.own.mk>

WARNS?=	3
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

