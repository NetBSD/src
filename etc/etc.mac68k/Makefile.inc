#	$NetBSD: Makefile.inc,v 1.2.2.2 1999/12/27 18:28:53 wrstuden Exp $
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
#EXTRA_KERNELS+= 	GENERIC GENERICSBC

INST_KERNELS+= 		INSTALL
INSTALLATION_DIRS+=	installation/instkernel

snap_md_post:
#
# Build the install kernels...
#
.for k in ${INST_KERNELS}
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
.for k in ${INST_KERNELS}
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/mac68k/instkernel/instkernel && \
		${MAKE} INSTALL_KERNEL=${k} && ${MAKE} release
.endfor
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/instkernel
