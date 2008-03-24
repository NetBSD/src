#	$NetBSD: Makefile.inc,v 1.6.10.1 2008/03/24 07:14:41 keiichi Exp $
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
