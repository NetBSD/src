#	$NetBSD: Makefile.walnut.inc,v 1.11 2021/03/30 02:04:43 rin Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh
CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${POWERPC}/${PPCDIR}/openbios/locore.S

SYSTEM_LD_TAIL_EXTRA_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; STAT=${TOOL_STAT}; \
		export OBJDUMP OBJCOPY STAT; ${MKIMG} $@ $@.img
