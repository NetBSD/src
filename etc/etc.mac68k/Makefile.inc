#	$NetBSD: Makefile.inc,v 1.2.4.3 2000/06/27 15:12:52 he Exp $
#
#	mac68k-specific release building goo
#

# This is commented out in the distribution, because
#
#  1) GENERICSBC isn't distributed
#     (It's just GENERIC:s/ncrsci/sbc/),
#
# and
#
#  2) if we include GENERIC here without also setting
#     UPDATE, we'll end up building GENERIC twice.
#
#EXTRA_KERNELS+= 		GENERIC GENERICSBC
EXTRA_INSTALL_KERNELS+= 	INSTALL

INSTALLATION_DIRS+=		installation/instkernel

snap_md_post:
#
# Build the install kernels...
#
.for k in ${EXTRA_INSTALL_KERNELS}
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${k} -s ${KERNSRCDIR} ${k}
.ifndef UPDATE
	cd ${KERNOBJDIR}/${k} && ${MAKE} cleandir
.endif
	cd ${KERNOBJDIR}/${k} && ${MAKE} depend && ${MAKE} ${_J}
.endfor
#
# the ramdisk...
#
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && \
		${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/ramdisk && \
		${MAKE} ${_J}
#
# and put them all together.
#
.for k in ${EXTRA_INSTALL_KERNELS}
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} INSTALL_KERNEL=${k} && ${MAKE} release INSTALL_KERNEL=${k}
.endfor
	cd ${.CURDIR}/../distrib/notes && ${MAKE} && ${MAKE} release
	cd ${RELEASEDIR}/installation/instkernel && \
		cksum -o 1 *.gz >BSDSUM && \
		cksum *.gz >CKSUM && \
		cksum -m *.gz >MD5 && \
		cksum -o 2 *.gz >SYSVSUM
