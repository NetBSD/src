#	$NetBSD: Makefile.explora.inc,v 1.1 2003/03/11 10:57:55 hannken Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo objcopy --adjust-vma 0x81000000 netbsd netbsd.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 netbsd netbsd.img
