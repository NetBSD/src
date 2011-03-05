#	$NetBSD: Makefile.walnut.inc,v 1.4.100.1 2011/03/05 20:50:15 rmind Exp $

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
