#	$NetBSD: Makefile.inc,v 1.4 2004/11/16 05:14:12 yamt Exp $

CPPFLAGS+=	-include ${.CURDIR}/../compat_openbsd.h

WARNS?=	1

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
