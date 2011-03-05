#	$NetBSD: Makefile.explora.inc,v 1.4.100.1 2011/03/05 20:50:15 rmind Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} --adjust-vma 0x81000000 $@ $@.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 $@ $@.img
