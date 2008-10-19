# $NetBSD: Makefile.inc,v 1.1.4.2 2008/10/19 22:39:50 haad Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
