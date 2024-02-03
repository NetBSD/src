#	$NetBSD: Makefile.inc,v 1.14.6.1 2024/02/03 11:47:04 martin Exp $
#
#	etc.evbppc/Makefile.inc -- evbppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "powerpc"
# oea
KERNEL_SETS+=		EV64260
KERNEL_SETS+=		PMPPC
BUILD_KERNELS+=		INSTALL_PMPPC
# ibm4xx
KERNEL_SETS+=		DHT
KERNEL_SETS+=		EXPLORA451
KERNEL_SETS+=		OPENBLOCKS200 OPENBLOCKS266
KERNEL_SETS+=		WALNUT
BUILD_KERNELS+=		INSTALL_OPENBLOCKS266
BUILD_KERNELS+=		INSTALL_WALNUT
# booke
KERNEL_SETS+=		MPC8536DS MPC8548CDS
KERNEL_SETS+=		P2020DS P2020RDB
KERNEL_SETS+=		RB800 RB850GX2
KERNEL_SETS+=		TWRP1025
BUILD_KERNELS+=		INSTALL_P2020DS INSTALL_P2020RDB
BUILD_KERNELS+=		INSTALL_RB850GX2
BUILD_KERNELS+=		INSTALL_TWRP1025
# wii
KERNEL_SETS+=		WII
BUILD_KERNELS+=		INSTALL_WII
#
KERNEL_SUFFIXES=	img bin ub
.endif

# Support for mkimage
MKIMAGE= 	${NETBSDSRCDIR}/distrib/utils/embedded/mkimage
IMAGE.rel=      ${RELEASEDIR}/${RELEASEMACHINEDIR}
IMAGE.dir=      ${IMAGE.rel}/binary/gzimg
IMAGE.kern=     ${IMAGE.rel}/binary/kernel
IMAGEENDIAN=	be

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
	HOST_SH=${HOST_SH} \
	KERNOBJDIR=${KERNOBJDIR} \
	MACHINE=${MACHINE} \
	MKDTB=${MKDTB} \
	${HOST_SH} -x ${MKIMAGE} -h ${.TARGET:S/smp_//} -D ${DESTDIR} \
            -S ${NETBSDSRCDIR} -B ${IMAGEENDIAN} ${MKI_OPTS.${.TARGET}} \
            ${IMAGE.dir}/${.TARGET:S/smp_//}.img.gz

.if ${MACHINE_ARCH} == "powerpc"
smp_wii: __mkimage
MKI_OPTS.smp_wii=	-K ${IMAGE.kern}
SNAP_MD_POST_DEPS+=	smp_wii
INSTALLATION_DIRS+=	binary/gzimg
.endif

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS}
