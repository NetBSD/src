#	$NetBSD: Makefile.walnut.inc,v 1.4.98.1 2010/11/06 08:08:17 uebayasi Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh
CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	walnut_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/walnut/walnut_start.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
