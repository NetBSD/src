#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.2 1994/04/09 02:47:12 chopps Exp $

.ifdef DESTDIR
snap_md: netbsd-generic netbsd-a3000
	cp ${.CURDIR}/../sys/arch/amiga/compile/GENERIC/netbsd \
	    ${DESTDIR}/snapshot/netbsd-generic
	cp ${.CURDIR}/../sys/arch/amiga/compile/A3000/netbsd \
	    ${DESTDIR}/snapshot/netbsd-a3000
netbsd-generic:
	cd ${.CURDIR}/../sys/arch/amiga/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/amiga/compile/GENERIC && \
	    make clean && make depend && make

netbsd-a3000:
	cd ${.CURDIR}/../sys/arch/amiga/conf && config A3000
	cd ${.CURDIR}/../sys/arch/amiga/compile/A3000 && \
	    make clean && make depend && make
.endif	# DESTDIR check
