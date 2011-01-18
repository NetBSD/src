#	$NetBSD: Makefile.mpc85xx.inc,v 1.2 2011/01/18 01:10:24 matt Exp $

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

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}

