#	$NetBSD: Makefile.inc,v 1.10 1998/03/04 13:16:04 christos Exp $

CPPFLAGS+=-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
CPPFLAGS+=-I${.CURDIR} -I.
KBD=	unix.kbd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
