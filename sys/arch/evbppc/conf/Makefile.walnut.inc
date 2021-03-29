#	$NetBSD: Makefile.walnut.inc,v 1.10 2021/03/29 13:12:38 rin Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh
CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	walnut_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/walnut/walnut_start.S

SYSTEM_LD_TAIL_EXTRA_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; STAT=${TOOL_STAT}; \
		export OBJDUMP OBJCOPY STAT; ${MKIMG} $@ $@.img
