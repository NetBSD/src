#	$NetBSD: Makefile.sam460ex.inc,v 1.1 2026/06/16 21:51:19 rkujawa Exp $

CFLAGS+=-mcpu=440
AFLAGS+=-mcpu=440

TEXTADDR?=	0x100000

SYSTEM_FIRST_OBJ=	sam460ex_locore.o
SYSTEM_FIRST_SFILE=	${THISPPC}/sam460ex/sam460ex_locore.S

.if ${USETOOLS} == "yes"
MKUBOOTIMAGE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkubootimage
.else
MKUBOOTIMAGE=		mkubootimage
.endif
#
# -O linux is deliberate: this U-Boot's bootm only passes the
# FDT/ePAPR boot contract (r3 = device tree) for Linux-type images.
#
MKUBOOTIMAGEARGS=	-A powerpc -T kernel -O linux
MKUBOOTIMAGEARGS+=	-a ${TEXTADDR} -e ${TEXTADDR}
MKUBOOTIMAGEARGS+=	-n "NetBSD/sam460ex ${_OSRELEASE}"
.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MKUBOOTIMAGEARGS+=	-t "${MKREPRO_TIMESTAMP}"
.endif
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
