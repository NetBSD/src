#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#
#	$Id: Makefile.inc,v 1.2 1994/03/31 02:22:16 cgd Exp $

.ifdef DESTDIR
snap_md: netbsd-aha netbsd-bt
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICAHA/netbsd \
	    ${DESTDIR}/snapshot/netbsd-aha
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICAHBBT/netbsd \
	    ${DESTDIR}/snapshot/netbsd-ahbbt
netbsd-aha:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICAHA
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICAHA && \
	    make clean && make depend && make

netbsd-bt:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICBT
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICBT && \
	    make clean && make depend && make
.endif	# DESTDIR check
