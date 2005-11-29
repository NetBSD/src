#	$NetBSD: Makefile.explora.inc,v 1.3.14.1 2005/11/29 21:22:58 yamt Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} --adjust-vma 0x81000000 netbsd netbsd.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 netbsd netbsd.img
