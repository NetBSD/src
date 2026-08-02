#	$NetBSD: Makefile.inc,v 1.8 2026/08/02 02:59:14 brook Exp $
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
	TOOL_SED=${TOOL_SED} \
	TOOL_PWD_MKDB=${TOOL_PWD_MKDB} \
	HOST_SH=${HOST_SH} \
	KERNOBJDIR=${KERNOBJDIR} \
	MACHINE=${MACHINE} \
	MKDTB=${MKDTB} \
	MKPOSTFIX=${MKPOSTFIX} \
	${HOST_SH} ${MKIMAGE} -h ${.TARGET:S/smp_//} -D ${DESTDIR} \
            -S ${NETBSDSRCDIR} ${MKI_OPTS.${.TARGET}} \
            ${IMAGE.dir}/${.TARGET:S/smp_//}.img.gz
	for hash in ${HASHES}; do \
		(cd ${IMAGE.dir} && ${TOOL_CKSUM} -a $${hash} ${.TARGET:S/smp_//}.img.gz > ${.TARGET:S/smp_//}.img.gz.$${hash}) \
	done


.if ${MACHINE_ARCH} == "riscv64"
.if ${HAVE_UEFI} == "yes"
smp_efibootriscv64:
	cd ${KERNSRCDIR}/stand/efiboot/bootriscv64 && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.efi'
SNAP_MD_POST_DEPS+=	smp_efibootriscv64
INSTALLATION_DIRS+=	installation/misc
.endif

smp_riscv64: __mkimage
MKI_OPTS.smp_riscv64=		-K ${IMAGE.kern}
SNAP_MD_POST_DEPS_IMGZ+=	smp_riscv64
.endif


.if ${MACHINE_ARCH} == "riscv32"
smp_riscv32: __mkimage
MKI_OPTS.smp_riscv32=		-K ${IMAGE.kern}
SNAP_MD_POST_DEPS_IMGZ+=	smp_riscv32
.endif


smp_hashes:
	@echo "===> Making hash files: ${HASHES}"
.for hash in ${HASHES}
	cat ${IMAGE.dir}/*.${hash} > ${IMAGE.dir}/${hash} \
	&& rm -f ${IMAGE.dir}/*.${hash}
.endfor
SNAP_MD_POST_DEPS_HASHES+=	smp_hashes

snap_md_post_images: ${SNAP_MD_POST_DEPS_IMGZ} .WAIT ${SNAP_MD_POST_DEPS_HASHES}

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS} snap_md_post_images

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
