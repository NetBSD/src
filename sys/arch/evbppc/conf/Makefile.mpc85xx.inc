#	$NetBSD: Makefile.mpc85xx.inc,v 1.3.28.1 2014/08/10 06:53:56 tls Exp $

CFLAGS+=		-misel -Wa,-me500
CFLAGS+=		-fno-inline-functions-called-once
AFLAGS+=		-Wa,-me500
# gcc emits bad code with these options
#AFLAGS+=		-mcpu=8548 
#CFLAGS+=		-mcpu=8548 
TEXTADDR?=		20000

SYSTEM_FIRST_OBJ=	mpc85xx_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/mpc85xx/mpc85xx_start.S
