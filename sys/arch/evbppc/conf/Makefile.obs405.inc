#	$NetBSD: Makefile.obs405.inc,v 1.6 2010/10/30 03:46:03 uebayasi Exp $

CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405


.if ${PRDCTTYPE} == "obs200"

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

TEXTADDR?=	450000

SYSTEM_FIRST_OBJ=	obs200_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/obs405/obs200_locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img


.elif ${PRDCTTYPE} == "obs266"

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${POWERPC}/${PPCDIR}/openbios/locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} $@ $@.img


.elif ${PRDCTTYPE} == "obs600"

TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	obs600_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/obs405/obs600_locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin;

.endif
