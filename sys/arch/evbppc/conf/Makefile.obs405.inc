#	$NetBSD: Makefile.obs405.inc,v 1.4 2009/05/18 05:17:28 mrg Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405
TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${POWERPC}/${PPCDIR}/openbios/locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} netbsd netbsd.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
