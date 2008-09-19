# $NetBSD: Makefile.inc,v 1.1 2008/09/19 22:01:25 joerg Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
