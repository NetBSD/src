#	$NetBSD: Makefile.inc,v 1.2.2.1 2002/06/06 07:51:10 lukem Exp $
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
