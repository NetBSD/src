# $NetBSD: Makefile.inc,v 1.2.42.1 2018/07/28 13:20:46 martin Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif
