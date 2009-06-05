# $NetBSD: Makefile.inc,v 1.1.12.2 2009/06/05 17:01:46 snj Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
