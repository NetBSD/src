#	$NetBSD: Makefile.inc,v 1.30 2002/12/02 13:31:22 lukem Exp $
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

#
# Install miniroot images and auxiliary scripts to the release tree
#
snap_md_post:
	# install boot image in netboot directory
	cp -p ${DESTDIR}/usr/mdec/boot.net ${RELEASEDIR}/installation/netboot

	# install links to boot and installation utilities for various media
	rm -f ${RELEASEDIR}/installation/tape/tapefile1.gz
	rm -f ${RELEASEDIR}/installation/tape/tapefile2
	rm -f ${RELEASEDIR}/installation/floppy/disk1.gz
	rm -f ${RELEASEDIR}/installation/floppy/disk2
	rm -f ${RELEASEDIR}/installation/netboot/rootfs.tgz
	ln -s ../bootfs/netbsd.ram.aout.gz \
			${RELEASEDIR}/installation/tape/tapefile1.gz
	ln -s ../bootfs/instfs.tgz \
			${RELEASEDIR}/installation/tape/tapefile2
	ln -s ../bootfs/boot.fs.gz \
			${RELEASEDIR}/installation/floppy/disk1.gz
	ln -s ../bootfs/instfs.tgz \
			${RELEASEDIR}/installation/floppy/disk2
	ln -s ../bootfs/instfs.tgz \
			${RELEASEDIR}/installation/netboot/rootfs.tgz

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${dir} '*'
.endfor

CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sparc/bootfs && ${PRINTOBJDIR}
CDBOOTIMAGE=	${CDBOOTIMAGEDIR}/boot.fs

iso-image_md_post:
	mksunbootcd ${RELEASEDIR}/installation/cdrom/netbsd-${MACHINE}.iso \
		${CDBOOTIMAGE} ${CDBOOTIMAGE} ${CDBOOTIMAGE}
