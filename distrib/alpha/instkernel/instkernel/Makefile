#	$NetBSD: Makefile,v 1.14 2001/10/21 23:23:19 jmc Exp $

.include "../../../../Makefile.inc"

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

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
.include <bsd.own.mk>
.include <bsd.obj.mk>
