#	$NetBSD: Makefile,v 1.19 2002/04/10 15:34:59 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

KERNEL?=	${KERNOBJDIR}/INSTALL/netbsd

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
