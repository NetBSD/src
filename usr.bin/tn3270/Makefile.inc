#	$NetBSD: Makefile.inc,v 1.7.2.1 1997/10/27 19:13:54 mellon Exp $

WARNS=	0
CFLAGS += -DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
KBD = unix.kbd

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
