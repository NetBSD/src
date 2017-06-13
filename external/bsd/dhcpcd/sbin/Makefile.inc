# $NetBSD: Makefile.inc,v 1.5 2017/06/13 00:28:10 roy Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
