#	$NetBSD: Makefile,v 1.10 2000/05/02 16:06:45 sjg Exp $

.include "../../../../Makefile.inc"

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL?=${KERNOBJDIR}/INSTALL/netbsd
STRIP?=		strip		# make STRIP=echo can be useful

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	${STRIP} netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.kernobj.mk>
.include <bsd.own.mk>
.include <bsd.obj.mk>
