#	$NetBSD: Makefile.inc,v 1.7 1997/10/20 03:37:19 mrg Exp $

WARNS=	0
CFLAGS += -DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
KBD = unix.kbd

.if exists("${.CURDIR}/../../Makefile.inc")
.include "${.CURDIR}/../../Makefile.inc"
.endif
