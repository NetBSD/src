#	$NetBSD: Makefile,v 1.2 1996/08/30 17:46:03 thorpej Exp $
#
# Makefile for bellctrl

all: bellctrl sample.fm

sample.fm: sample_fm.c
	$(CC) -c -o sample.aout ${.CURDIR}/sample_fm.c
	strip sample.aout
	dd bs=1 skip=32 count=52 if=sample.aout of=$@
	rm sample.aout

PROG = bellctrl
CFLAGS+=-I${.CURDIR}/../../..

.include <bsd.prog.mk>
