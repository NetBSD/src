#	$NetBSD: Makefile.inc,v 1.7.2.2 1997/11/08 10:38:29 lukem Exp $

WARNS=	0
CPPFLAGS+=-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
CPPFLAGS+=-I${.CURDIR} -I.
KBD=	unix.kbd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
