#	$NetBSD: Makefile.dreamcast.inc,v 1.9.100.1 2011/03/05 20:49:50 rmind Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
LINKFORMAT=	-N

SYSTEM_LD_TAIL_EXTRA+=;							\
	echo ${OBJCOPY} -O binary $@ $@.bin;				\
	${OBJCOPY} -O binary $@ $@.bin;					\
	chmod 755 $@.bin
