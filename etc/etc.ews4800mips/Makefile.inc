#	$NetBSD: Makefile.inc,v 1.2 2006/01/09 01:36:29 tsutsui Exp $
#
#	etc.ews4800mips/Makefile.inc --
#		ews4800mips-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=	 	RAMDISK

MD_INSTALLATION_DIRS=	installation/floppy installation/boot

INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}

snap_md_post:
	# install bootloader binaries in boot directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot \
	    ${RELEASEDIR}/${MACHINE}/installation/boot
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot.coff \
	    ${RELEASEDIR}/${MACHINE}/installation/boot

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/${dir} '*'
.endfor
