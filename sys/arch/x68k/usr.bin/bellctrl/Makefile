#	$NetBSD: Makefile,v 1.5 1998/08/04 16:39:55 minoura Exp $
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

CLEANFILES+=sample.fm

.include <bsd.prog.mk>
