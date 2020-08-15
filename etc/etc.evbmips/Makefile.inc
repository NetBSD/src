#	$NetBSD: Makefile.inc,v 1.25 2020/08/15 05:03:06 simonb Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SUFFIXES=	srec ub elf32

.if ${MACHINE_ARCH} == "mipsel"
KERNEL_SETS=		ALCHEMY CI20 CPMBR1400 DBAU1500 DBAU1550 GDIUM
KERNEL_SETS+=		MALTA MTX-1 OMSAL400
BUILD_KERNELS=		INSTALL_CI20 INSTALL_OMSAL400 INSTALL_MALTA

KERNEL_SETS+=		P4032 P5064 P6032

.elif ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
KERNEL_SETS=		MALTA32 MALTA64 XLSATX32 XLSATX64
BUILD_KERNELS=		INSTALL_MALTA32 INSTALL_MALTA64
BUILD_KERNELS+=		INSTALL_XLSATX32 INSTALL_XLSATX64

. if ${MACHINE_ARCH} == "mips64el"
KERNEL_SETS+=		P5064-64
KERNEL_SETS+=		LOONGSON
BUILD_KERNELS+=		INSTALL_LOONGSON
KERNEL_SETS+=		GDIUM64
. else
KERNEL_SETS+=		OCTEON
BUILD_KERNELS+=		INSTALL_OCTEON
. endif
.else
KERNEL_SETS=		ALCHEMY DB120 DBAU1500 DBAU1550 MALTA RB433UAH
BUILD_KERNELS=		INSTALL_MALTA

.endif

# SBMIPS has a kernel for all targets
KERNEL_SETS+=		SBMIPS${MACHINE_ARCH:S/mips//:C/e[lb]//}

# Support for mkimage
MKIMAGE= 	${NETBSDSRCDIR}/distrib/utils/embedded/mkimage
IMAGE.rel=      ${RELEASEDIR}/${RELEASEMACHINEDIR}
IMAGE.dir=      ${IMAGE.rel}/binary/gzimg
IMAGE.kern=     ${IMAGE.rel}/binary/kernel
.if !empty(MACHINE_ARCH:M*eb)
IMAGEENDIAN=	be
.else
IMAGEENDIAN=	le
.endif

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
	${HOST_SH} ${MKIMAGE} -h ${.TARGET:S/smp_//} -D ${DESTDIR} \
            -S ${NETBSDSRCDIR} -B ${IMAGEENDIAN} ${MKI_OPTS.${.TARGET}} \
            ${IMAGE.dir}/${.TARGET:S/smp_//}.img.gz

.if ${MACHINE_ARCH} == "mips64eb"
smp_octeon: __mkimage
MKI_OPTS.smp_octeon=	-K ${IMAGE.kern}
SNAP_MD_POST_DEPS+=	smp_octeon
INSTALLATION_DIRS+=	binary/gzimg
.endif

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS}
