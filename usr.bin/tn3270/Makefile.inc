#	$NetBSD: Makefile.inc,v 1.6 1997/01/09 20:21:42 tls Exp $

CFLAGS += -DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270 -Dunix
KBD = unix.kbd

.include "${.CURDIR}/../../Makefile.inc"
