#	$NetBSD: Makefile.inc,v 1.5 1999/02/13 02:54:56 lukem Exp $

CPPFLAGS+=-I${.CURDIR}/../../api
MKMAN=	no
KBD= unix.kbd

.PATH: ${.CURDIR}/../../api

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
