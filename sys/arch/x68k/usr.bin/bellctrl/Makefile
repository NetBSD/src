#	$NetBSD: Makefile,v 1.4 1997/10/26 22:09:43 lukem Exp $
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
CPPFLAGS+=-I${.CURDIR}/../../..

.include <bsd.prog.mk>
