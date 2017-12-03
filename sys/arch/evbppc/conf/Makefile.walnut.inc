#	$NetBSD: Makefile.walnut.inc,v 1.6.6.2 2017/12/03 11:36:11 jdolecek Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh
CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	walnut_start.o
SYSTEM_FIRST_SFILE=	${THISPPC}/walnut/walnut_start.S

SYSTEM_LD_TAIL_EXTRA_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; STAT=${TOOL_STAT}; \
		export OBJDUMP OBJCOPY STAT; ${MKIMG} $@ $@.img
