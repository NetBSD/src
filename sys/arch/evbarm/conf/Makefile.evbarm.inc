#	$NetBSD: Makefile.evbarm.inc,v 1.31.2.1 2015/09/22 12:05:40 skrll Exp $

#
# If this is a install kernel and the ramdisk image exists in the object
# tree, insert it into the kernel *before* we make the u-boot images.
#
RAMDISKNAME?=	ramdisk
.if ${KERNEL_BUILD:T:M*INSTALL*} != ""
RAMDISKDIR!=	cd ${NETBSDSRCDIR}/distrib/${THISARM:T}/instkernel/${RAMDISKNAME} && ${PRINTOBJDIR}

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

.if defined(KERNEL_BASE_PHYS) && defined(KERNEL_BASE_VIRT)
. if ${KERNEL_BASE_PHYS} == ${KERNEL_BASE_VIRT}
CPPFLAGS+=-DKERNEL_BASES_EQUAL -DKERNEL_BASE_VOFFSET=0
. else
CPPFLAGS+=-DKERNEL_BASE_VOFFSET="(${KERNEL_BASE_VIRT}-${KERNEL_BASE_PHYS})"
. endif
.endif

EXTRA_CLEAN+=	${KERNELS:=.map}

.if defined(KERNEL_BASE_PHYS)
LINKTEXT=
KERNLDSCRIPT=	ldscript

EXTRA_CLEAN+=	${KERNLDSCRIPT} tmp

# generate ldscript from common template
${KERNLDSCRIPT}: \
    ${THISARM}/conf/ldscript.evbarm \
    ${THISARM}/conf/Makefile.evbarm.inc \
    Makefile \
    ${BOARDMKFRAG}
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.else
LINKTEXT=	-Ttext ${LOADADDRESS}
.endif	# KERNEL_BASE_PHYS
