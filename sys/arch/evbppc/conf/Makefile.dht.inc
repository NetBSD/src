#	$NetBSD: Makefile.dht.inc,v 1.1.2.2 2021/04/03 21:44:42 thorpej Exp $

CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405
TEXTADDR?=	100000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/dht/locore.S
