#	$NetBSD: Makefile.inc,v 1.12.2.2 2000/02/12 16:55:02 he Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

# These are packaged and distributed
EXTRA_KERNELS+= GENERIC GENERIC_TINY

# These are for use by the bootfloppy construction tools
INST_KERNELS= INSTALL INSTALL_TINY INSTALL_SMALL

# Build the INST_KERNELS and the boot floppies and install them
snap_md_post:
.for k in ${INST_KERNELS}
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${k} -s ${KERNSRCDIR} ${k}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${k} && ${MAKE} clean
.endif
	cd ${KERNOBJDIR}/${k} && ${MAKE} depend && ${MAKE} ${_J}
.endfor
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/i386/floppies && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/i386/floppies && \
		${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
