# $NetBSD: Makefile.inc,v 1.3.6.1 2018/01/13 21:35:30 snj Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
