#	$NetBSD: Makefile.inc,v 1.2 2001/11/29 22:45:58 thorpej Exp $
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
