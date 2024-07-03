#	$NetBSD: Makefile.inc,v 1.12.6.1 2024/07/03 18:38:55 martin Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} != "powerpc64"
KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_601 POWERMAC_G5 \
			POWERMAC_G5.MP

BUILD_KERNELS+=		INSTALL INSTALL_601 GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
.endif

# For "make iso-image"
# mkisofs arguments to create a hybrid (HFS/Rockridge) CD-ROM image.
# XXX: files in ${DISTRIB_CDROMDIR} should be moved under distrib/macppc/cdrom?
DISTRIB_CDROMDIR=	${NETBSDSRCDIR}/distrib/cdrom

BOOTHFS=		boothfs
UUDECODE_FILES=		${BOOTHFS}
.SUFFIXES:	.uue
.PATH.uue: ${DISTRIB_CDROMDIR}

MKISOFS_FLAGS+=	-hide-rr-moved \
	-hfs -part -hide-hfs-list ${DISTRIB_CDROMDIR}/hide-hfs.lst \
	--macbin -map ${DISTRIB_CDROMDIR}/hfsmap.lst \
	-hide-joliet-list ${DISTRIB_CDROMDIR}/hide-jol.lst \
	-boot-hfs-file ${.OBJDIR}/${BOOTHFS}
# exclude large debug sets to avoid 700MB overflow
MKISOFS_FLAGS+=	-m debug.tgz -m xdebug.tgz

MD_DISTRIBDIR=	${NETBSDSRCDIR}/distrib/${MACHINE}
CDINSTKERNEL=	${MD_DISTRIBDIR}/floppies/md-kernel
_INSTKERNELNOBJDIR!=	cd ${CDINSTKERNEL} && ${PRINTOBJDIR}
CDKERNELS=	netbsd-INSTALL.gz netbsd
CDKERNELS+=	netbsd-INSTALL_601.gz netbsd.601

iso-image-md-pre: ${BOOTHFS}
.for kernel target in ${CDKERNELS}
	${INSTALL} ${COPY} -m 0644 \
		${_INSTKERNELNOBJDIR}/${kernel} ${CDROM.dir}/${target}
.endfor
	${INSTALL} ${COPY} -m 0644 ${DESTDIR}/usr/mdec/ofwboot ${CDROM.dir}
	${INSTALL} ${COPY} -m 0644 ${DESTDIR}/usr/mdec/ofwboot.xcf ${CDROM.dir}

iso-image: do-iso-image

iso-image-md-post:
	${TOOL_MACPPCINSTALLBOOT} ${CDROM_IMAGE} \
		${DESTDIR}/usr/mdec/bootxx /ofwboot
