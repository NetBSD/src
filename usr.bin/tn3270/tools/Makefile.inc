#	$NetBSD: Makefile.inc,v 1.2 1998/03/04 13:16:10 christos Exp $

CPPFLAGS+=-I${.CURDIR}/../../api
NOMAN=	noman
KBD= unix.kbd

.PATH: ${.CURDIR}/../../api

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
