#	$NetBSD: Makefile.inc,v 1.41.22.1 2012/04/17 00:02:58 yamt Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC_SCSI3 GENERIC_SUN4U GENERIC.MP	\
			MRCOFFEE_OFW MRCOFFEE KRUPS

BUILD_KERNELS=		INSTALL

MD_INSTALLATION_DIRS=	installation/miniroot	\
			installation/netboot	\
			installation/bootfs	\
			installation/tape

# XXX floppy overflows, see below (add to above list to re-enable)
#			installation/floppy

INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}


INSTALLATION_SYMLINKS=	\
		tape/tapefile1.gz	../bootfs/netbsd.ram.aout.gz	\
		tape/tapefile2		../bootfs/instfs.tgz		\
		netboot/rootfs.tgz	../bootfs/instfs.tgz

# XXX - floppy overflows on boot.fs.gz - so disabled for now,
# add the following to above list to re-enable
#		floppy/disk1.gz		../bootfs/boot.fs.gz
#		floppy/disk2		../bootfs/instfs.tgz

#
# Install miniroot images and auxiliary scripts to the release tree
#
snap_md_post:
	# install boot images in netboot directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot.net \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/bootjs.net \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot

	# install links to boot and installation utilities for various media
.for linkdst linksrc in ${INSTALLATION_SYMLINKS} 
	rm -f ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/${linkdst}
	ln -s ${linksrc} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/${linkdst}
.endfor

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/${dir} '*'
.endfor

CDBOOTIMAGEDIR!= cd ${NETBSDSRCDIR}/distrib/sparc/bootfs && ${PRINTOBJDIR}
CDBOOTIMAGE=	${CDBOOTIMAGEDIR}/boot.fs

iso-image-md-post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
	    ${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
	    ${CDROM_IMAGE} ${CDBOOTIMAGE} ${CDBOOTIMAGE} ${CDBOOTIMAGE}
