#	$NetBSD: Makefile,v 1.9 2001/11/14 00:58:49 tv Exp $

.include <bsd.prog.mk>
.MAIN:	all

realall: inst.fs.gz
all: realall

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk.sysinst && ${PRINTOBJDIR}

inst.fs.gz: inst.fs
	gzip -9vc inst.fs > ${.TARGET}
CLEANFILES+=	inst.fs.gz

inst.fs: ${RAMDISKDIR}/ramdisk.fs
	cp ${RAMDISKDIR}/ramdisk.fs ${.TARGET}
CLEANFILES+=	inst.fs

${RAMDISKDIR}/ramdisk.fs:
	cd ${.CURDIR}/../ramdisk.sysinst ; ${MAKE}

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p inst.fs inst.fs.gz ${RELEASEDIR}/installation/floppy
