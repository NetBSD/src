#	$NetBSD: Makefile.inc,v 1.4 2002/12/02 13:18:10 lukem Exp $
#
#	etc.next68k/Makefile.inc -- next68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

snap_md_post:
	# install boot image in installation directory
	cp -p ${DESTDIR}/usr/mdec/boot ${RELEASEDIR}/installation/boot
