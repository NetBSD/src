#	$NetBSD: Makefile.inc,v 1.4 1998/09/06 02:54:48 lukem Exp $

CPPFLAGS+=-I${.CURDIR}/../../api
NOMAN=	noman
KBD= unix.kbd

.PATH: ${.CURDIR}/../../api

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
