#	$NetBSD: Makefile,v 1.12 2000/09/09 19:10:10 jhawk Exp $

.include "../../../../Makefile.inc"

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL?=${KERNOBJDIR}/INSTALL/netbsd
STRIP?=		strip		# make STRIP=echo can be useful

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	nm netbsd > netbsd.symbols
	${STRIP} netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p netbsd.gz ${RELEASEDIR}/binary/kernel/netbsd.INSTALL.gz
	cp netbsd.symbols ${RELEASEDIR}/binary/kernel/netbsd.symbols

# nothing to do here
depend includes lint regress tags:

.include <bsd.kernobj.mk>
.include <bsd.own.mk>
.include <bsd.obj.mk>
