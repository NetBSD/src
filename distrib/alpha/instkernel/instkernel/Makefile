#	$NetBSD: Makefile,v 1.18 2002/04/07 04:29:15 lukem Exp $

.include "${.CURDIR}/../../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

KERNEL?=${KERNOBJDIR}/INSTALL/netbsd

CLEANFILES+=	netbsd netbsd.gz netbsd.symbols

realall:
	cp ${KERNEL} netbsd
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	${NM} netbsd > netbsd.symbols
	${STRIP} netbsd
	gzip -9f netbsd

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p netbsd.gz ${RELEASEDIR}/binary/kernel/netbsd.INSTALL.gz
	cp netbsd.symbols ${RELEASEDIR}/binary/kernel/netbsd.symbols

.include <bsd.prog.mk>
