#	$NetBSD: Makefile,v 1.4 2002/03/30 06:55:18 thorpej Exp $

.include "../../../../Makefile.inc"
.include <bsd.own.mk>

TOP=	${.CURDIR}/../../../..

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

KERNEL?=${KERNOBJDIR}/INSTALL/netbsd

all:
	cp ${KERNEL} netbsd
	${MDSETIMAGE} -v netbsd ${RAMDISK}
	${STRIP} netbsd
	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
	    ${TOP}/sys/arch/arm/conf/elf2aout.sh netbsd netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p netbsd.gz ${RELEASEDIR}/binary/kernel/netbsd.INSTALL.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.kernobj.mk>
.include <bsd.obj.mk>
