#	$NetBSD: Makefile.inc,v 1.6 1999/07/14 21:36:36 wrstuden Exp $

HOST_CPPFLAGS+=-I${.CURDIR}/../../api
MKMAN=	no
KBD= unix.kbd

.PATH: ${.CURDIR}/../../api

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
