#	$NetBSD: Makefile.inc,v 1.4 1998/02/11 19:19:53 gwr Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

.ifdef DESTDIR
snap_md: netbsd.sun3 netbsd.sun3x
	cp ${.CURDIR}/../sys/arch/sun3/compile/GENERIC/netbsd \
	    ${DESTDIR}/snapshot/netbsd.sun3
	cp ${.CURDIR}/../sys/arch/sun3/compile/GENERIC3X/netbsd \
	    ${DESTDIR}/snapshot/netbsd.sun3x

netbsd.sun3:
	cd ${.CURDIR}/../sys/arch/sun3/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/sun3/compile/GENERIC && \
	    make clean && make depend && make

netbsd.sun3x:
	cd ${.CURDIR}/../sys/arch/sun3/conf && config GENERIC3X
	cd ${.CURDIR}/../sys/arch/sun3/compile/GENERIC3X && \
	    make clean && make depend && make

.endif	# DESTDIR check
