#	$NetBSD: Makefile.inc,v 1.5 2003/01/03 15:34:39 lukem Exp $
#
#	etc.next68k/Makefile.inc -- next68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

snap_md_post:
	# install boot image in installation directory
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${DESTDIR}/usr/mdec/boot \
	    ${RELEASEDIR}/${MACHINE}/installation/boot
