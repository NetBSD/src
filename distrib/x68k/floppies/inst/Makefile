#	$NetBSD: Makefile,v 1.5 2000/11/28 03:03:27 minoura Exp $

PROG=	inst.fs.gz
SRCS=
MKMAN=	no

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk.sysinst ; make print-objdir

inst.fs.gz: inst.fs
	gzip -9vc inst.fs > ${.TARGET}
CLEANFILES+=	inst.fs.gz

inst.fs: ${RAMDISKDIR}/ramdisk.fs
	cp ${RAMDISKDIR}/ramdisk.fs ${.TARGET}
CLEANFILES+=	inst.fs

${RAMDISKDIR}/ramdisk.fs:
	cd ${.CURDIR}/../ramdisk.sysinst ; make

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p inst.fs inst.fs.gz ${RELEASEDIR}/installation/floppy

.include <bsd.prog.mk>
