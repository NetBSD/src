#	$NetBSD: Makefile.inc,v 1.29 2024/07/11 13:46:45 rin Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SUFFIXES=	srec ub elf32

.if !${MACHINE_MIPS64}
# 32-bit common
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA MIPSSIM
BUILD_KERNELS=		INSTALL_MALTA

. if ${MACHINE_ARCH} == "mipsel"
KERNEL_SETS+=		CI20 CPMBR1400 GDIUM MTX-1 OMSAL400 P4032 P5064 P6032
BUILD_KERNELS+=		INSTALL_CI20 INSTALL_OMSAL400
. else
KERNEL_SETS+=		DB120 RB433UAH
. endif

.else
# 64-bit common
KERNEL_SETS=		MALTA32 MALTA64 XLSATX32 XLSATX64
BUILD_KERNELS=		INSTALL_MALTA32 INSTALL_MALTA64
BUILD_KERNELS+=		INSTALL_XLSATX32 INSTALL_XLSATX64
KERNEL_SETS+=		MIPSSIM64

. if !empty(MACHINE_ARCH:Mmips*64el)
KERNEL_SETS+=		P5064-64
KERNEL_SETS+=		LOONGSON
BUILD_KERNELS+=		INSTALL_LOONGSON
KERNEL_SETS+=		GDIUM64
. else
KERNEL_SETS+=		OCTEON
BUILD_KERNELS+=		INSTALL_OCTEON
. endif
.endif

# SBMIPS has a kernel for all targets
KERNEL_SETS+=		SBMIPS${MACHINE_ARCH:S/mipsn//:S/mips//:C/e[lb]//}

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

.if !empty(MACHINE_ARCH:Mmips*64eb)
smp_octeon: __mkimage
MKI_OPTS.smp_octeon=	-K ${IMAGE.kern}
SNAP_MD_POST_DEPS+=	smp_octeon
INSTALLATION_DIRS+=	binary/gzimg
.endif

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS}
