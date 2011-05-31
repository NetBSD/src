#	$NetBSD: Makefile.mpc85xx.inc,v 1.2.4.3 2011/05/31 03:04:00 rmind Exp $

CFLAGS+=		-mmultiple
CFLAGS+=		-mno-string -misel -Wa,-me500
CFLAGS+=		-fno-inline-functions-called-once
AFLAGS+=		-Wa,-me500
# gcc emits bad code with these options
#AFLAGS+=		-mcpu=8548 
#CFLAGS+=		-mcpu=8548 
TEXTADDR?=		20000

SYSTEM_FIRST_OBJ=	mpc85xx_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/mpc85xx/mpc85xx_start.S
