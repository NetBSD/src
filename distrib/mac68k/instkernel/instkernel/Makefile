#	$NetBSD: Makefile,v 1.6 2002/02/03 19:59:30 wormey Exp $

.include "../../../Makefile.inc"

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

INSTALL_KERNEL?=INSTALL
MDSETIMAGE?=	mdsetimage
KERNEL=${KERNOBJDIR}/${INSTALL_KERNEL}/netbsd

all:
	cp ${KERNEL} netbsd-${INSTALL_KERNEL}
	${MDSETIMAGE} -v netbsd-${INSTALL_KERNEL} ${RAMDISK}
# XXX Doesn't boot with this. Why?
#	strip netbsd-${INSTALL_KERNEL}
# XXX Reported not to boot on some machines.
#	gzip -9 -f netbsd-${INSTALL_KERNEL}
	gzip -f netbsd-${INSTALL_KERNEL}

clean cleandir distclean:
	rm -f netbsd*

release: all
	${INSTALL} -d -o root -g wheel -m 755 \
		${RELEASEDIR}/installation/instkernel/	
	${INSTALL} ${COPY} -o root -g wheel -m 644 netbsd-${INSTALL_KERNEL}.gz \
		${RELEASEDIR}/installation/instkernel/	

.include <bsd.kernobj.mk>
.include <bsd.own.mk>
.include <bsd.obj.mk>
