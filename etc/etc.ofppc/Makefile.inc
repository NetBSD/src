#	$NetBSD: Makefile.inc,v 1.9 2008/05/01 01:58:10 garbled Exp $
#
#	etc.ofppc/Makefile.inc -- ofppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

MD_INSTALLATION_DIRS=	installation/netboot

INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}

INSTALLATION_SYMLINKS=	\
	netboot/netbsd-INSTALL.gz	../../binary/kernel/netbsd-INSTALL.gz \
	netboot/netbsd-GENERIC.gz	../../binary/kernel/netbsd-GENERIC.gz

snap_md_post:
	# install ofwboot in netboot directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/ofwboot \
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
