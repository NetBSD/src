#	$NetBSD: Makefile.dreamcast.inc,v 1.9.78.2 2010/10/09 03:31:42 yamt Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
LINKFORMAT=	-N

SYSTEM_LD_TAIL_EXTRA+=;							\
	echo ${OBJCOPY} -O binary $@ $@.bin;				\
	${OBJCOPY} -O binary $@ $@.bin;					\
	chmod 755 $@.bin
