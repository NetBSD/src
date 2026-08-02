#	$NetBSD: Makefile.inc,v 1.141 2026/08/02 08:26:07 martin Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

MKIMAGE= ${NETBSDSRCDIR}/distrib/utils/embedded/mkimage

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents
EVBARM_BOARDS=
.for i in armv4 xscale armv5t armv6 armv7 arm64
EVBARM_BOARDS.${i}=
.endfor

.if !empty(MACHINE_ARCH:M*eb)
IMAGEENDIAN=	be
# big endian boards
KERNEL_SETS.armv4+=		IXM1200

EVBARM_BOARDS.xscale+=		ADI_BRH
KERNEL_SETS.xscale+=		NSLU2

EVBARM_BOARDS.armv5t+=		KUROBOX_PRO
KERNEL_SETS.armv5t+=		OPENBLOCKS_A6
# Other marvell-based machines may also work but not tested yet.

EVBARM_BOARDS.armv6+=		RPI
EVBARM_BOARDS.armv6hf+=	 	RPI
KERNEL_SETS.armv6+=		RPI2
KERNEL_SETS.armv6hf+=	 	RPI2

KERNEL_SETS.armv7+=		GENERIC
KERNEL_SETS.armv7hf+=		GENERIC

KERNEL_SETS.arm64+=		GENERIC64
KERNEL_SETS.arm64+=		GENERIC64_PMAPMI
.else
IMAGEENDIAN=	le
# little endian boards
EVBARM_BOARDS.armv4+=		GEMINI
EVBARM_BOARDS.armv4+=		INTEGRATOR
EVBARM_BOARDS.armv4+=		INTEGRATOR_CP
KERNEL_SETS.armv4+=		IXM1200
EVBARM_BOARDS.armv4+=		SMDK2410
EVBARM_BOARDS.armv4+=		SMDK2800
EVBARM_BOARDS.armv4+=		TS7200

EVBARM_BOARDS.xscale+=		ADI_BRH
EVBARM_BOARDS.xscale+=		CP3100
EVBARM_BOARDS.xscale+=		GUMSTIX
KERNEL_SETS.xscale+=		HDL_G
EVBARM_BOARDS.xscale+=		IQ80310
EVBARM_BOARDS.xscale+=		IQ80321
EVBARM_BOARDS.xscale+=		TEAMASA_NPWR
EVBARM_BOARDS.xscale+=		TWINTAIL

EVBARM_BOARDS.armv5t+=		KUROBOX_PRO
EVBARM_BOARDS.armv5t+=		MV2120
KERNEL_SETS.armv5t+=		OPENBLOCKS_A6
EVBARM_BOARDS.armv5t+=		OPENRD
EVBARM_BOARDS.armv5t+=		SHEEVAPLUG
KERNEL_SETS.armv5t+=		GENERIC_V5

EVBARM_BOARDS.armv6+=		RPI
EVBARM_BOARDS.armv6hf+=	 	RPI
KERNEL_SETS.armv6+=		RPI2
KERNEL_SETS.armv6hf+=	 	RPI2

KERNEL_SETS.armv7+=		GENERIC
KERNEL_SETS.armv7hf+=		GENERIC

KERNEL_SETS.arm64+=		GENERIC64
KERNEL_SETS.arm64+=		GENERIC64_PMAPMI
.endif

IMAGE.rel=	${RELEASEDIR}/${RELEASEMACHINEDIR}
IMAGE.dir=	${IMAGE.rel}/binary/gzimg
IMAGE.kern=	${IMAGE.rel}/binary/kernel
IMAGE.instk=	${IMAGE.rel}/installation/instkernel

__mkimage: .USE
	TOOL_MAKE=${MAKE} \
	TOOL_MAKEFS=${TOOL_MAKEFS} \
	TOOL_DISKLABEL=${TOOL_DISKLABEL} \
	TOOL_FDISK=${TOOL_FDISK} \
	TOOL_GPT=${TOOL_GPT} \
	TOOL_GZIP=${TOOL_GZIP} \
	TOOL_MKNOD=${TOOL_MKNOD} \
	TOOL_PAX=${TOOL_PAX} \
	TOOL_MKUBOOTIMAGE=${TOOL_MKUBOOTIMAGE} \
	TOOL_MTREE=${TOOL_MTREE} \
	TOOL_SED=${TOOL_SED} \
	TOOL_PWD_MKDB=${TOOL_PWD_MKDB} \
	HOST_SH=${HOST_SH} \
	KERNOBJDIR=${KERNOBJDIR} \
	MACHINE=${MACHINE} \
	MKDTB=${MKDTB} \
	MKPOSTFIX=${MKPOSTFIX} \
	${HOST_SH} ${MKIMAGE} -h ${.TARGET:R:S/smp_//} -D ${DESTDIR} \
	    -S ${NETBSDSRCDIR} -B ${IMAGEENDIAN} ${MKI_OPTS.${.TARGET}} \
	    ${IMAGE.dir}/${.TARGET:S/smp_//}

__mkbootimage: .USE
	@echo "===> Making bootable image ${IMAGE.dir:T}/${.TARGET:S/smp_//}"
	cp ${IMAGE.dir}/${.TARGET:R:S/smp_//:C/-.*//}.img ${IMAGE.dir}/${.TARGET:R:S/smp_//}
	${TOOL_INSTALLBOOT} -m ${MACHINE} -o board=${.TARGET:R:R:C/smp_[^-]*-//} \
			${IMAGE.dir}/${.TARGET:R:S/smp_//} \
	&& ${TOOL_GZIP} -f ${IMAGE.dir}/${.TARGET:R:S/smp_//} \
	&& for hash in ${HASHES}; do \
		(cd ${IMAGE.dir} && ${TOOL_CKSUM} -a $${hash} ${.TARGET:S/smp_//} > ${.TARGET:S/smp_//}.$${hash}) \
	done \
	|| rm ${IMAGE.dir}/${.TARGET:R:S/smp_//}*

__mkcompressed: .USE
	@echo "===> Compressing image ${IMAGE.dir}/${.TARGET:R:S/smp_//}"
	${TOOL_GZIP} -f ${GZIP_N_FLAG} -9 ${IMAGE.dir}/${.TARGET:R:S/smp_//}
.for hash in ${HASHES}
	(cd ${IMAGE.dir} && ${TOOL_CKSUM} -a ${hash} ${.TARGET:S/smp_//} > ${.TARGET:S/smp_//}.${hash})
.endfor

ARCHES.earmv4=		armv4
ARCHES.earmv4eb=	armv4
ARCHES.earmv5=		xscale armv5t
ARCHES.earmv5eb=	xscale armv5t
ARCHES.earmv6=		armv6
ARCHES.earmv6hf=	armv6hf
ARCHES.earmv6eb=	armv6
ARCHES.earmv6hfeb=	armv6hf
ARCHES.earmv7=		armv7
ARCHES.earmv7hf=	armv7hf
ARCHES.earmv7eb=	armv7
ARCHES.earmv7hfeb=	armv7hf
ARCHES.aarch64=		arm64
ARCHES.aarch64eb=	arm64
.for arch in ${ARCHES.${MACHINE_ARCH}}
.for board in ${EVBARM_BOARDS.${arch}}
KERNEL_SETS+=		${board}
BUILD_KERNELS+=		${board}_INSTALL
.endfor
KERNEL_SETS+=		${KERNEL_SETS.${arch}}
.endfor

.if !empty(MACHINE_ARCH:Maarch64*)
smp_efibootaa64:
	cd ${KERNSRCDIR}/stand/efiboot/bootaa64 && ${MAKE} release
SNAP_MD_POST_DEPS+=	smp_efibootaa64
INSTALLATION_DIRS+=	installation/misc
.endif

.if !empty(MACHINE_ARCH:Mearmv7hf)
smp_efibootarm:
	cd ${KERNSRCDIR}/stand/efiboot/bootarm && ${MAKE} release
SNAP_MD_POST_DEPS+=	smp_efibootarm
INSTALLATION_DIRS+=	installation/misc
.endif

.if !empty(MACHINE_ARCH:M*armv7*) && empty(ALL_KERNELS)
smp_armv7.img: __mkimage
smp_armv7.img.gz: __mkcompressed
MKI_OPTS.smp_armv7.img=		-K ${IMAGE.kern} -x
SNAP_MD_POST_DEPS_IMG+=		smp_armv7.img
SNAP_MD_POST_DEPS_IMGZ+=	smp_armv7.img.gz
.for board in ${INSTALLBOOT_BOARDS}
smp_armv7-${board}.img.gz: __mkbootimage
SNAP_MD_POST_DEPS_BOOTABLE+=	smp_armv7-${board}.img.gz
.endfor
.endif

.if !empty(MACHINE_ARCH:Maarch64*) && empty(ALL_KERNELS)
smp_arm64.img: __mkimage
smp_arm64.img.gz: __mkcompressed
MKI_OPTS.smp_arm64.img=		-K ${IMAGE.kern} -x
SNAP_MD_POST_DEPS_IMG+=		smp_arm64.img
SNAP_MD_POST_DEPS_IMGZ+=	smp_arm64.img.gz
.for board in ${INSTALLBOOT_BOARDS}
smp_arm64-${board}.img.gz: __mkbootimage
SNAP_MD_POST_DEPS_BOOTABLE+=	smp_arm64-${board}.img.gz
.endfor
smp_arm64mbr.img: __mkimage
smp_arm64mbr.img.gz: __mkcompressed
MKI_OPTS.smp_arm64mbr.img=	-K ${IMAGE.kern} -x
SNAP_MD_POST_DEPS_IMG+=		smp_arm64mbr.img
SNAP_MD_POST_DEPS_IMGZ+=	smp_arm64mbr.img.gz
.for board in ${INSTALLBOOT_BOARDS}
smp_arm64mbr-${board}.img.gz: __mkbootimage
SNAP_MD_POST_DEPS_BOOTABLE+=	smp_arm64mbr-${board}.img.gz
.endfor
.endif

.if !empty(KERNEL_SETS:MRPI)
smp_rpi.img: __mkimage
smp_rpi.img.gz: __mkcompressed
.if empty(ALL_KERNELS) || !empty(ALL_KERNELS:MRPI)
MKI_OPTS.smp_rpi.img=		-K ${IMAGE.kern}/netbsd-RPI.img.gz -x
SNAP_MD_POST_DEPS_IMG+=		smp_rpi.img
SNAP_MD_POST_DEPS_IMGZ+=	smp_rpi.img.gz
.for board in ${INSTALLBOOT_BOARDS}
smp_rpi-${board}.img.gz: __mkbootimage
SNAP_MD_POST_DEPS_BOOTABLE+=	smp_rpi-${board}.img.gz
.endfor
.endif
smp_rpi_inst.img: ${IMAGE.instk}/netbsd-RPI_INSTALL.img.gz
smp_rpi_inst.img: __mkimage
smp_rpi_inst.img.gz: __mkcompressed
.if empty(ALL_KERNELS) || !empty(ALL_KERNELS:MRPI_INSTALL)
MKI_OPTS.smp_rpi_inst.img=	-b -K ${IMAGE.instk}/netbsd-RPI_INSTALL.img.gz -x
SNAP_MD_POST_DEPS_IMG+=		smp_rpi_inst.img
SNAP_MD_POST_DEPS_IMGZ+=	smp_rpi_inst.img.gz
.for board in ${INSTALLBOOT_BOARDS}
smp_rpi_inst-${board}.img.gz: __mkbootimage
SNAP_MD_POST_DEPS_BOOTABLE+=	smp_rpi_inst-${board}.img.gz
.endfor
.endif
.endif

smp_hashes:
	.if exists(${IMAGE.dir})
		@echo "===> Making hash files: ${HASHES}"
.for hash in ${HASHES}
		cat ${IMAGE.dir}/*.${hash} > ${IMAGE.dir}/${hash} \
		&& rm -f ${IMAGE.dir}/*.${hash}
.endfor
	.endif

SNAP_MD_POST_DEPS_HASHES+=	smp_hashes

snap_md_post_images: ${SNAP_MD_POST_DEPS_IMG} .WAIT ${SNAP_MD_POST_DEPS_BOOTABLE} .WAIT ${SNAP_MD_POST_DEPS_IMGZ} .WAIT ${SNAP_MD_POST_DEPS_HASHES}

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS} snap_md_post_images

.if !empty(MACHINE_ARCH:Maarch64*)
KERNEL_SUFFIXES=	img
.else
KERNEL_SUFFIXES=	bin img srec ub
.endif

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
