#	$NetBSD: Makefile.explora.inc,v 1.2.2.2 2005/12/11 10:28:17 christos Exp $

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403

SYSTEM_FIRST_OBJ=	explora_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/explora/explora_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} --adjust-vma 0x81000000 netbsd netbsd.img ; \
	${OBJCOPY} --adjust-vma 0x81000000 netbsd netbsd.img
