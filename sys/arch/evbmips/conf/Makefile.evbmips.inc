#	Makefile.evbmips.inc,v 1.2 2005/11/24 12:54:29 dbj Exp

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISMIPS:T}/instkernel/ramdisk && ${PRINTOBJDIR}

.if defined(NEED_MDSETIMAGE) && ${NEED_MDSETIMAGE} != no \
    && exists(${RAMDISKDIR}/ramdisk.fs)
SYSTEM_DEP+= ${RAMDISKDIR}/ramdisk.fs
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs
.if defined(DEBUG)
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@.gdb ${RAMDISKDIR}/ramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@.gdb ${RAMDISKDIR}/ramdisk.fs
.endif
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
MKUBOOTIMAGEARGS=       -A mips -T kernel
MKUBOOTIMAGEARGS+=      -a ${TEXTADDR:C/....$/0000/}
.if ${NEED_UBOOTIMAGE:Mbsentry} == "bsentry"
MKUBOOTIMAGEARGS+=	-E $(TEXTADDR)
.else
MKUBOOTIMAGEARGS+=	-e $(TEXTADDR)
.endif
MKUBOOTIMAGEARGS+=      -n "$(MACHINE) ${_OSRELEASE} ($(KERNEL_BUILD:T))"

.if ${NEED_UBOOTIMAGE:Mgz} == "gz"
MKUBOOTIMAGEARGS+=	-C gz

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_GZIP_N} -9c $@.bin '>' $@.bin.gz; \
	${TOOL_GZIP_N} -9c $@.bin > $@.bin.gz; \
	echo ${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin.gz $@.ub; \
	${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin.gz $@.ub

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin.gz@}
.elif ${NEED_UBOOTIMAGE:Mlzma} == "lzma"
#TOOL_LZMA?=		/usr/pkg/bin/lzma e
#LZMA_ARGS?=		-d20 -pb4
TOOL_XZ?=		xz -F lzma
XZ_ARGS?=		-kf --lzma1=dict=1048576,lc=0,lp=0,pb=0
MKUBOOTIMAGEARGS+=	-C lzma

#	echo ${TOOL_LZMA} e $@.bin $@.bin.lzma -d20 -pb4; \
#	${TOOL_LZMA} e $@.bin $@.bin.lzma -d20 -pb4; \

SYSTEM_LD_TAIL_EXTRA+=; \
	(set -e 1; \
	rm -f $@.bin.lzma; \
	echo ${TOOL_XZ} ${XZ_ARGS} $@.bin; \
	${TOOL_XZ} ${XZ_ARGS} $@.bin; \
	lzmainfo $@.bin.lzma; \
	echo ${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin.lzma $@.ub; \
	${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin.lzma $@.ub) || \
	(rm $@ ; exit 1)

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin.lzma@}
.else
MKUBOOTIMAGEARGS+=	-C none
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin $@.ub; \
	${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin $@.ub; \

.endif

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ub@}
.endif
.endif
