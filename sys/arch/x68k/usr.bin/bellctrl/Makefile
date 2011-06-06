#	$NetBSD: Makefile,v 1.8.150.1 2011/06/06 09:07:04 jruoho Exp $
#
# Makefile for bellctrl

all: bellctrl sample.fm

sample.fm: sample_fm.c
	$(CC) -c -o sample.aout ${.CURDIR}/sample_fm.c
	strip sample.aout
	dd bs=1 skip=32 count=52 if=sample.aout of=$@
	rm sample.aout

PROG=	bellctrl
NOMAN=	# defined
CPPFLAGS+=-I${.CURDIR}/../../..

CLEANFILES+=sample.fm

.include <bsd.prog.mk>
