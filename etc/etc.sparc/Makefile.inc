#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.8 1997/08/31 23:22:02 pk Exp $

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
