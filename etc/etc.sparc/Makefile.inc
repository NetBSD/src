#	$NetBSD: Makefile.inc,v 1.31 2003/01/03 15:34:40 lukem Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_SCSI3 GENERIC_SUN4U

BUILD_KERNELS=		INSTALL

MD_INSTALLATION_DIRS=	installation/miniroot	\
			installation/netboot	\
			installation/bootfs	\
			installation/tape	\
			installation/floppy
INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}


INSTALLATION_SYMLINKS=	\
		tape/tapefile1.gz	../bootfs/netbsd.ram.aout.gz	\
		tape/tapefile2		../bootfs/instfs.tgz		\
		floppy/disk1.gz		../bootfs/boot.fs.gz		\
		floppy/disk2		../bootfs/instfs.tgz		\
		netboot/rootfs.tgz	../bootfs/instfs.tgz

#
# Install miniroot images and auxiliary scripts to the release tree
#
snap_md_post:
	# install boot image in netboot directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot.net \
	    ${RELEASEDIR}/${MACHINE}/installation/netboot

	# install links to boot and installation utilities for various media
.for linkdst linksrc in ${INSTALLATION_SYMLINKS} 
	rm -f ${RELEASEDIR}/${MACHINE}/installation/${linkdst}
	ln -s ${linksrc} ${RELEASEDIR}/${MACHINE}/installation/${linkdst}
.endfor

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/${dir} '*'
.endfor

CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sparc/bootfs && ${PRINTOBJDIR}
CDBOOTIMAGE=	${CDBOOTIMAGEDIR}/boot.fs

iso-image_md_post:
	mksunbootcd \
	    ${RELEASEDIR}/${MACHINE}/installation/cdrom/netbsd-${MACHINE}.iso \
	    ${CDBOOTIMAGE} ${CDBOOTIMAGE} ${CDBOOTIMAGE}
