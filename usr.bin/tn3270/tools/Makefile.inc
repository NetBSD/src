#	$NetBSD: Makefile.inc,v 1.1.2.2 1997/11/08 10:05:28 lukem Exp $

CPPFLAGS+=-I${.CURDIR}/../../api
NOMAN=	noman

.PATH: ${.CURDIR}/../../api

.if exists("${.CURDIR}/../../Makefile.inc")
.include "${.CURDIR}/../../Makefile.inc"
.endif
