#	$NetBSD: Makefile.inc,v 1.7 2001/12/12 01:48:47 tv Exp $

HOST_CPPFLAGS+=-I${.CURDIR}/../../api
NOMAN=	# defined
KBD= unix.kbd

.PATH: ${.CURDIR}/../../api

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
