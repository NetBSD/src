#	$NetBSD: Makefile.obs200.inc,v 1.3 2005/11/24 12:54:29 dbj Exp $

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

CFLAGS+=-mcpu=403
AFLAGS+=-mcpu=403
TEXTADDR?=	450000

SYSTEM_FIRST_OBJ=	obs200_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/obs405/obs200_locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} netbsd netbsd.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img
