#	$NetBSD: Makefile,v 1.16 2002/03/05 21:23:28 thorpej Exp $

.include "../../../../Makefile.inc"
.include <bsd.own.mk>

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETIMAGE?=	mdsetimage
KERNEL?=${KERNOBJDIR}/INSTALL/netbsd
STRIP?=		strip		# make STRIP=echo can be useful

all:
	cp ${KERNEL} netbsd
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	nm netbsd > netbsd.symbols
	${STRIP} netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz netbsd.symbols

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p netbsd.gz ${RELEASEDIR}/binary/kernel/netbsd.INSTALL.gz
	cp netbsd.symbols ${RELEASEDIR}/binary/kernel/netbsd.symbols

# nothing to do here
depend includes lint regress tags:

.include <bsd.kernobj.mk>
.include <bsd.obj.mk>
