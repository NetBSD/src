#	$NetBSD: Makefile.explora.inc,v 1.4.98.1 2010/11/06 08:08:17 uebayasi Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} --adjust-vma 0x81000000 $@ $@.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 $@ $@.img
