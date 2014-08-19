#	$NetBSD: Makefile.obs405.inc,v 1.6.18.2 2014/08/20 00:02:59 tls Exp $

CFLAGS+=-mcpu=405
AFLAGS+=-mcpu=405


.if ${PRDCTTYPE} == "obs200"

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

TEXTADDR?=	450000

SYSTEM_FIRST_OBJ=	obs200_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/obs405/obs200_locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; STAT=${TOOL_STAT]; \
		export OBJDUMP OBJCOPY STAT; ${MKIMG} $@ $@.img


.elif ${PRDCTTYPE} == "obs266"

MKIMG?=	${HOST_SH} ${THISPPC}/compile/walnut-mkimg.sh

TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	locore.o
SYSTEM_FIRST_SFILE=	${POWERPC}/${PPCDIR}/openbios/locore.S

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${MKIMG} $@ $@.img ; \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; STAT=${TOOL_STAT]; \
		export OBJDUMP OBJCOPY STAT; ${MKIMG} $@ $@.img


.elif ${PRDCTTYPE} == "obs600"

TEXTADDR?=	25000

SYSTEM_FIRST_OBJ=	obs600_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/obs405/obs600_locore.S

.if ${USETOOLS} == "yes"
MKUBOOTIMAGE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkubootimage
.else
MKUBOOTIMAGE=		mkubootimage
.endif
MKUBOOTIMAGEARGS=	-A powerpc -T kernel
MKUBOOTIMAGEARGS+=	-a $(TEXTADDR)
MKUBOOTIMAGEARGS+=	-n "NetBSD/openblocks600 ${_OSRELEASE}"
MKUBOOTIMAGEARGS_NONE=  ${MKUBOOTIMAGEARGS} -C none
MKUBOOTIMAGEARGS_GZ=    ${MKUBOOTIMAGEARGS} -C gz

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin; \
	echo "${TOOL_GZIP_N} -c $@.bin > $@.bin.gz"; \
	${TOOL_GZIP_N} -c $@.bin > $@.bin.gz; \
	echo ${MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS_GZ} $@.bin.gz $@.gz.ub; \
	${MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS_GZ} $@.bin.gz $@.gz.ub; \
	echo ${MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS_NONE} $@.bin $@.ub; \
	${MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS_NONE} $@.bin $@.ub;

.endif
