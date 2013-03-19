#	$NetBSD: Makefile.evbarm.inc,v 1.23 2013/03/19 22:16:54 garbled Exp $

#
# If this is a install kernel and the ramdisk image exists in the object
# tree, insert it into the kernel *before* we make the u-boot images.
#
.if ${KERNEL_BUILD:T:MRPI_INSTALL} != ""
RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISARM:T}/instkernel/sshramdisk && ${PRINTOBJDIR}

.if exists(${RAMDISKDIR}/sshramdisk.fs)
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/sshramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/sshramdisk.fs
.endif
.elif ${KERNEL_BUILD:T:M*INSTALL} != ""
RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISARM:T}/instkernel/ramdisk && ${PRINTOBJDIR}

.if exists(${RAMDISKDIR}/ramdisk.fs)
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs; \
	${TOOL_MDSETIMAGE} -s $@ ${RAMDISKDIR}/ramdisk.fs
.endif
.endif

.if defined(BOARDMKFRAG)	# Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif

EXTRA_CLEAN+= ldscript tmp ${KERNELS:=.map}

.if defined(KERNEL_BASE_PHYS)

LINKFLAGS=	-T ldscript

netbsd: ldscript             # XXX

# generate ldscript from common template
ldscript: ${THISARM}/conf/ldscript.evbarm ${THISARM}/conf/Makefile.evbarm.inc Makefile ${BOARDMKFRAG}
	echo ${KERNELS}
	${TOOL_SED} -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.endif	# KERNEL_BASE_PHYS
