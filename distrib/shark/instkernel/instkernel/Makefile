#	$NetBSD: Makefile,v 1.1 2002/02/11 17:49:41 thorpej Exp $

.include "../../../../Makefile.inc"

TOP=	${.CURDIR}/../../../..

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

MDSETIMAGE?=	mdsetimage
KERNEL?=${KERNOBJDIR}/INSTALL/netbsd
KERNEL=/sys/arch/shark/compile/INSTALL/netbsd
STRIP?=		strip		# make STRIP=echo can be useful

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
.include <bsd.own.mk>
.include <bsd.obj.mk>
