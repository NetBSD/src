#	$Id: Makefile.inc,v 1.4 1993/08/02 17:56:24 mycroft Exp $

CFLAGS += -DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO -DTN3270
KBD = unix.kbd

.include "${.CURDIR}/../../Makefile.inc"
