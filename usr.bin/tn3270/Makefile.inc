#	$NetBSD: Makefile.inc,v 1.12 2009/04/14 22:15:27 lukem Exp $

WARNS?=	1	# XXX -Wcast-qual -Wshadow issues

USE_FORT?= yes	# network client

CPPFLAGS+=-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
CPPFLAGS+=-I${.CURDIR} -I.
KBD=	unix.kbd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
