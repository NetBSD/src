#	$NetBSD: Makefile.inc,v 1.3 1998/09/05 11:30:08 lukem Exp $

CPPFLAGS+=-I${.CURDIR}/../../api -Dunix
NOMAN=	noman
KBD= unix.kbd

.PATH: ${.CURDIR}/../../api

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif
