#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#
#	$NetBSD: Makefile.inc,v 1.6.2.1 1995/11/24 19:30:11 chopps Exp $

.ifdef DESTDIR
snap_md: netbsd-install
	cp ${.CURDIR}/../sys/arch/amiga/compile/INSTALL/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-install.gz
netbsd-install:
	cd ${.CURDIR}/../sys/arch/amiga/conf && config INSTALL
	cd ${.CURDIR}/../sys/arch/amiga/compile/INSTALL && \
	    make clean && make depend && make && gzip -9 netbsd
.endif	# DESTDIR check
