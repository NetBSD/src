#
#	$NetBSD: Makefile.inc,v 1.1.1.1.4.1 1997/10/29 16:10:54 mellon Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

.ifdef DESTDIR
snap_md: netbsd-ataritt netbsd-boot netbsd-bootx netbsd-generic netbsd-hades
	cp ${.CURDIR}/../sys/arch/atari/compile/ATARITT/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-ataritt.gz
	cp ${.CURDIR}/../sys/arch/atari/compile/BOOT/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-boot.gz
	cp ${.CURDIR}/../sys/arch/atari/compile/BOOTX/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-bootx.gz
	cp ${.CURDIR}/../sys/arch/atari/compile/GENERIC/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-generic.gz
	cp ${.CURDIR}/../sys/arch/atari/compile/HADES/netbsd.gz \
	    ${DESTDIR}/snapshot/netbsd-hades.gz

netbsd-ataritt:
	cd ${.CURDIR}/../sys/arch/atari/conf && config ATARITT
	cd ${.CURDIR}/../sys/arch/atari/compile/ATARITT && \
	    make clean && make depend && make && gzip -9 netbsd

netbsd-boot:
	cd ${.CURDIR}/../sys/arch/atari/conf && config BOOT
	cd ${.CURDIR}/../sys/arch/atari/compile/BOOT && \
	    make clean && make depend && make && gzip -9 netbsd

netbsd-bootx:
	cd ${.CURDIR}/../sys/arch/atari/conf && config BOOTX
	cd ${.CURDIR}/../sys/arch/atari/compile/BOOTX && \
	    make clean && make depend && make && gzip -9 netbsd

netbsd-generic:
	cd ${.CURDIR}/../sys/arch/atari/conf && config GENERIC
	cd ${.CURDIR}/../sys/arch/atari/compile/GENERIC && \
	    make clean && make depend && make && gzip -9 netbsd

netbsd-hades:
	cd ${.CURDIR}/../sys/arch/atari/conf && config HADES
	cd ${.CURDIR}/../sys/arch/atari/compile/HADES && \
	    make clean && make depend && make && gzip -9 netbsd

.endif	# DESTDIR check
