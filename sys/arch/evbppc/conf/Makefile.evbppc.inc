#	$NetBSD: Makefile.evbppc.inc,v 1.6.2.1 2011/06/23 14:19:08 cherry Exp $

CPPFLAGS += -D__${BOARDTYPE}__

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISPPC:T}/ramdisk && ${PRINTOBJDIR}

.if defined(NEED_MDSETIMAGE) && $(NEED_MDSETIMAGE) != "no" \
    && exists(${RAMDISKDIR}/ramdisk.fs)
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs
.endif

.if defined(NEED_SREC) && ${NEED_SREC} != "no"
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
.endif

.if defined(NEED_BINARY) && ${NEED_BINARY} != "no"
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}

.if defined(NEED_UBOOTIMAGE) && ${NEED_UBOOTIMAGE} != "no"
_OSRELEASE!=		${HOST_SH} $S/conf/osrelease.sh
MKUBOOTIMAGEARGS=       -A powerpc -T kernel -C gz
MKUBOOTIMAGEARGS+=      -a ${TEXTADDR:C/....$/0000/} -e $(TEXTADDR)
MKUBOOTIMAGEARGS+=      -n "NetBSD/$(MACHINE) ${_OSRELEASE} ($(KERNEL_BUILD:T))"

.if defined(USE_PIGZGZIP) && ${USE_PIGZGZIP} != "no"
TOOL_GZIP?=		${TOOL_PIGZ}
.endif
TOOL_GZIP?=		gzip
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_GZIP} -9cn $@.bin '>' $@.bin.gz; \
	${TOOL_GZIP} -9cn $@.bin > $@.bin.gz; \
	echo ${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin.gz $@.ub; \
	${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin.gz $@.ub

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin.gz@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ub@}
.endif
.endif

.-include "${THISPPC}/conf/Makefile.${BOARDTYPE}.inc"
