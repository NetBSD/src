#	$NetBSD: Makefile.mpc85xx.inc,v 1.4 2014/08/08 16:56:31 joerg Exp $

CFLAGS+=		-misel -Wa,-me500
CFLAGS+=		-fno-inline-functions-called-once
AFLAGS+=		-Wa,-me500
# gcc emits bad code with these options
#AFLAGS+=		-mcpu=8548 
#CFLAGS+=		-mcpu=8548 
TEXTADDR?=		20000

SYSTEM_FIRST_OBJ=	mpc85xx_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/mpc85xx/mpc85xx_start.S
