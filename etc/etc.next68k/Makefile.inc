#	$NetBSD: Makefile.inc,v 1.3 2002/06/05 13:49:33 deberg Exp $
#
#	etc.next68k/Makefile.inc -- next68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

# Build the release notes and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release

	# install boot image in installation directory
	cp -p ${DESTDIR}/usr/mdec/boot ${RELEASEDIR}/installation/boot
