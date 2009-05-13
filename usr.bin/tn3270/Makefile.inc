#	$NetBSD: Makefile.inc,v 1.11.20.1 2009/05/13 19:20:08 jym Exp $

WARNS?=	1	# XXX -Wcast-qual -Wshadow issues

USE_FORT?= yes	# network client

CPPFLAGS+=-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
CPPFLAGS+=-I${.CURDIR} -I.
KBD=	unix.kbd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
