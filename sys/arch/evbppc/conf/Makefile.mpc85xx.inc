#	$NetBSD: Makefile.mpc85xx.inc,v 1.3.14.1 2014/08/20 00:02:59 tls Exp $

CFLAGS+=		${${ACTIVE_CC} == "gcc":? -misel -Wa,-me500 :}
CFLAGS+=		${${ACTIVE_CC} == "gcc":? -fno-inline-functions-called-once :}
AFLAGS+=		${${ACTIVE_CC} == "gcc":? -Wa,-me500 :}
# gcc emits bad code with these options
#AFLAGS+=		-mcpu=8548 
#CFLAGS+=		-mcpu=8548 
TEXTADDR?=		20000

SYSTEM_FIRST_OBJ=	mpc85xx_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/mpc85xx/mpc85xx_start.S
