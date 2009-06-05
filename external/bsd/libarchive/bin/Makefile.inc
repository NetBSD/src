# $NetBSD: Makefile.inc,v 1.1.14.2 2009/06/05 17:19:29 snj Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
