#	$NetBSD: Makefile.inc,v 1.9 1997/09/20 03:09:18 mikel Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd netbsd.scsi3
	cp ${.CURDIR}/../sys/arch/sparc/compile/GENERIC/netbsd \
	    ${DESTDIR}/snapshot/netbsd
	cp ${.CURDIR}/../sys/arch/sparc/compile/GENERIC_SCSI3/netbsd \
	    ${DESTDIR}/snapshot/netbsd.scsi3

netbsd:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/sparc/compile/GENERIC && \
	    make clean && make depend && make

netbsd.scsi3:
	cd ${.CURDIR}/../sys/arch/sparc/conf && config GENERIC_SCSI3
	cd ${.CURDIR}/../sys/arch/sparc/compile/GENERIC_SCSI3 && \
	    make clean && make depend && make

.endif	# DESTDIR check
