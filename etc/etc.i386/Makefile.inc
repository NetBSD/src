#	$NetBSD: Makefile.inc,v 1.8 1996/10/16 02:25:08 perry Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd-adp netbsd-other

netbsd-adp:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICADP
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICADP && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICADP/netbsd \
	    ${DESTDIR}/snapshot/netbsd-adp

netbsd-other:
	cd ${.CURDIR}/../sys/arch/i386/conf && config GENERICOTHER
	cd ${.CURDIR}/../sys/arch/i386/compile/GENERICOTHER && \
	    make clean && make depend && make
	cp ${.CURDIR}/../sys/arch/i386/compile/GENERICOTHER/netbsd \
	    ${DESTDIR}/snapshot/netbsd-other

.endif	# DESTDIR check
