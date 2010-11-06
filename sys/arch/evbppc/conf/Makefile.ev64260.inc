#	$NetBSD: Makefile.ev64260.inc,v 1.4.98.1 2010/11/06 08:08:17 uebayasi Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

TEXTADDR?=	300000

SYSTEM_FIRST_OBJ=	${BOARDTYPE}_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/${BOARDTYPE}/${BOARDTYPE}_locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
