#	$NetBSD: Makefile.inc,v 1.35 2023/06/02 20:49:07 lukem Exp $
#
#	etc.sparc64/Makefile.inc -- sparc64-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.UP

BUILD_KERNELS+=		INSTALL

MD_INSTALLATION_DIRS=	installation/miniroot	\
			installation/netboot	\
			installation/misc
INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}

snap_md_post:
	# Install miniroot images and auxiliary scripts to the release tree
.if exists($(XSRCDIR)/xc/Imakefile)
	# build X11
.ifndef XSRC_DONE
	(cd $(XSRCDIR); $(MAKE) release)
.endif
.endif

	# install boot image and installation scripts in netboot directory
.for file in ofwboot.net
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/${file} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot
.endfor
.for file in ofwboot bootblk
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/${file} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
.endfor
.for file in installboot
	${HOST_INSTALL_FILE} -m ${BINMODE} ${DESTDIR}/usr/sbin/${file} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
.endfor

	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/${dir} '*'
.endfor


ISOBOOTFILEDIR!=cd ${.CURDIR}/../distrib/${MACHINE}/bootfs && ${PRINTOBJDIR}
ISOBOOTFILE?=	${ISOBOOTFILEDIR}/boot.fs

iso-image-md-post:
	SUNLABEL=${TOOL_SUNLABEL:Q} \
	    ${HOST_SH} ${NETBSDSRCDIR}/distrib/common/sunbootcd.sh \
	    ${CDROM_IMAGE} - - - - ${ISOBOOTFILE}
