#	Makefile.evbmips.inc,v 1.2 2005/11/24 12:54:29 dbj Exp

RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISMIPS:T}/instkernel/ramdisk && ${PRINTOBJDIR}

.if defined(NEED_MDSETIMAGE) && ${NEED_MDSETIMAGE} != no \
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
MKUBOOTIMAGEARGS=       -A mips -T kernel -C none
MKUBOOTIMAGEARGS+=      -a ${TEXTADDR:C/....$/0000/} -e $(TEXTADDR)
MKUBOOTIMAGEARGS+=      -n "NetBSD/$(MACHINE) ${_OSRELEASE} ($(KERNEL_BUILD:T))"

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin $@.ub; \
	${TOOL_MKUBOOTIMAGE} ${MKUBOOTIMAGEARGS} $@.bin $@.ub; \

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ub@}
.endif
.endif

