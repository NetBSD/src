#	$NetBSD: Makefile.mpc85xx.inc,v 1.2.4.2 2011/03/05 20:50:15 rmind Exp $

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

