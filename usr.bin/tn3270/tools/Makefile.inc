#	$NetBSD: Makefile.inc,v 1.1 1997/10/24 07:52:17 lukem Exp $

CPPFLAGS+=-I${.CURDIR}/../../api
NOMAN=	noman

.PATH: ${.CURDIR}/../../api

.if exists("${.CURDIR}/../../Makefile.inc")
.include "${.CURDIR}/../../Makefile.inc"
.endif
