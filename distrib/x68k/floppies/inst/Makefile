#	$NetBSD: Makefile,v 1.5.2.4 2001/04/21 20:52:56 he Exp $

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.MAIN:	all

realall: inst.fs.gz
all: realall

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
