#	$NetBSD: Makefile,v 1.3 1997/06/22 07:09:00 mrg Exp $
#
# Makefile for bellctrl

all: bellctrl sample.fm

sample.fm: sample_fm.c
	$(CC) -c -o sample.aout ${.CURDIR}/sample_fm.c
	strip sample.aout
	dd bs=1 skip=32 count=52 if=sample.aout of=$@
	rm sample.aout

PROG = bellctrl
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../../..

.include <bsd.prog.mk>
