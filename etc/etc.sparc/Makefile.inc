#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.7 1995/05/12 08:51:55 pk Exp $

.ifdef DESTDIR
snap_md: netbsd netbsd.scsi3 inst
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

inst:
	cp ${.CURDIR}/etc.sparc/README ${DESTDIR}/snapshot/README
	cp ${.CURDIR}/etc.sparc/install.sh ${DESTDIR}/snapshot/install.sh

.endif	# DESTDIR check
