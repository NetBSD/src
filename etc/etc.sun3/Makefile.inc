#	$NetBSD: Makefile.inc,v 1.2.2.1 1997/12/20 23:07:45 perry Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

.ifdef DESTDIR
snap_md: netbsd-gen netbsd-dl

netbsd-gen:
	cd ${.CURDIR}/../sys/arch/sun3/compile/GENERIC && make
	gzip -9 < ${.CURDIR}/../sys/arch/sun3/compile/GENERIC/netbsd \
	  > ${DESTDIR}/snapshot/netbsd-gen.gz

netbsd-dl:
	cd ${.CURDIR}/../sys/arch/sun3/compile/DISKLESS && make
	gzip -9 < ${.CURDIR}/../sys/arch/sun3/compile/DISKLESS/netbsd \
	  > ${DESTDIR}/snapshot/netbsd-dl.gz

.endif	# DESTDIR check
