#	$NetBSD: Makefile,v 1.4 2002/03/30 07:36:54 tsutsui Exp $

.include "${.CURDIR}/../../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

STRIP?= strip
MDSETIMAGE?=	mdsetimage

KERNELS= netbsd.INSTALL.gz netbsd.GENERIC_MD.gz
CLEANFILES+= ${KERNELS} netbsd

RAMDISKDIR!= cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK= ${RAMDISKDIR}/ramdisk.fs

all dependall: ${KERNELS}

netbsd.INSTALL.gz: ${KERNOBJDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL/netbsd .
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	${STRIP} netbsd
	gzip -9 netbsd
	mv netbsd.gz ${.TARGET}

netbsd.GENERIC_MD.gz: ${KERNOBJDIR}/GENERIC_MD/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/GENERIC_MD/netbsd .
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	gzip -9 netbsd
	mv netbsd.gz ${.TARGET}

release: netbsd.INSTALL.gz netbsd.GENERIC_MD.gz
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp ${KERNELS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${CLEANFILES}

.include <bsd.obj.mk>
