#	$NetBSD: Makefile.dht.inc,v 1.1 2021/04/02 07:00:33 rin Exp $

CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405
TEXTADDR?=	100000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/dht/locore.S
