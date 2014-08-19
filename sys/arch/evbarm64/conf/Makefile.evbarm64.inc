#	$NetBSD: Makefile.evbarm64.inc,v 1.1.4.2 2014/08/20 00:02:57 tls Exp $

#
# If this is a install kernel and the ramdisk image exists in the object
# tree, insert it into the kernel *before* we make the u-boot images.
#
RAMDISKNAME?=	ramdisk
.if ${KERNEL_BUILD:T:M*INSTALL} != ""
RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISA64:T}/${RAMDISKNAME} && ${PRINTOBJDIR}

.if exists(${RAMDISKDIR}/${RAMDISKNAME}.fs)
SYSTEM_DEP+=	${RAMDISKDIR}/${RAMDISKNAME}.fs
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/${RAMDISKNAME}.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/${RAMDISKNAME}.fs
.endif
.endif

.if defined(BOARDMKFRAG)	# Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif

EXTRA_CLEAN+= ${KERNELS:=.map}
