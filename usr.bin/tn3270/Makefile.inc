#	$NetBSD: Makefile.inc,v 1.8 1997/10/24 07:52:03 lukem Exp $

WARNS=	0
CPPFLAGS+=-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
CPPFLAGS+=-I${.CURDIR} -I.
KBD=	unix.kbd

.if exists("${.CURDIR}/../../Makefile.inc")
.include "${.CURDIR}/../../Makefile.inc"
.endif
