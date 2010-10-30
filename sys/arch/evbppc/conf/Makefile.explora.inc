#	$NetBSD: Makefile.explora.inc,v 1.5 2010/10/30 03:46:03 uebayasi Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} --adjust-vma 0x81000000 $@ $@.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 $@ $@.img
