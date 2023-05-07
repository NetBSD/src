#	$NetBSD: Makefile.inc,v 1.4 2023/05/07 12:41:46 skrll Exp $
#
#	etc.riscv/Makefile.inc -- riscv-specific etc Makefile targets
#

.if ${MACHINE_ARCH} == "riscv64"
KERNEL_SETS=		GENERIC64
.else
KERNEL_SETS=		GENERIC
.endif

# Support for mkimage
MKIMAGE= 	${NETBSDSRCDIR}/distrib/utils/embedded/mkimage
IMAGE.rel=      ${RELEASEDIR}/${RELEASEMACHINEDIR}
IMAGE.dir=      ${IMAGE.rel}/binary/gzimg
IMAGE.kern=     ${IMAGE.rel}/binary/kernel

#	TOOL_MKUBOOTIMAGE=${TOOL_MKUBOOTIMAGE} \

__mkimage: .USE
	TOOL_MAKE=${MAKE} \
	TOOL_MAKEFS=${TOOL_MAKEFS} \
	TOOL_DISKLABEL=${TOOL_DISKLABEL} \
	TOOL_FDISK=${TOOL_FDISK} \
	TOOL_GPT=${TOOL_GPT} \
	TOOL_GZIP=${TOOL_GZIP} \
	TOOL_MKNOD=${TOOL_MKNOD} \
	TOOL_PAX=${TOOL_PAX} \
	TOOL_MTREE=${TOOL_MTREE} \
	HOST_SH=${HOST_SH} \
	KERNOBJDIR=${KERNOBJDIR} \
	MACHINE=${MACHINE} \
	MKDTB=${MKDTB} \
	${HOST_SH} ${MKIMAGE} -h ${.TARGET:S/smp_//} -D ${DESTDIR} \
            -S ${NETBSDSRCDIR} ${MKI_OPTS.${.TARGET}} \
            ${IMAGE.dir}/${.TARGET:S/smp_//}.img.gz


.if ${MACHINE_ARCH} == "riscv64"
.if ${HAVE_UEFI} == "yes"
smp_efibootriscv64:
	cd ${KERNSRCDIR}/stand/efiboot/bootriscv64 && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.efi'
SNAP_MD_POST_DEPS+=	smp_efibootriscv64
INSTALLATION_DIRS+=	installation/misc
.endif

smp_riscv64: __mkimage
MKI_OPTS.smp_riscv64=	-K ${IMAGE.kern}
SNAP_MD_POST_DEPS+=	smp_riscv64
.endif


.if ${MACHINE_ARCH} == "riscv32"
smp_riscv32: __mkimage
MKI_OPTS.smp_riscv32=	-K ${IMAGE.kern}
SNAP_MD_POST_DEPS+=	smp_riscv32
.endif

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS}

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
