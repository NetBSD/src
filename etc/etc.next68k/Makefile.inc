#	$NetBSD: Makefile.inc,v 1.7 2008/03/18 04:20:37 lukem Exp $
#
#	etc.next68k/Makefile.inc -- next68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

snap_md_post:
	# install boot image in installation directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/boot
