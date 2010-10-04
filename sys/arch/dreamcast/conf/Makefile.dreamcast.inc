#	$NetBSD: Makefile.dreamcast.inc,v 1.11 2010/10/04 16:03:30 tsutsui Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
LINKFORMAT=	-N

SYSTEM_LD_TAIL_EXTRA+=;							\
	echo ${OBJCOPY} -O binary $@ $@.bin;				\
	${OBJCOPY} -O binary $@ $@.bin;					\
	chmod 755 $@.bin
