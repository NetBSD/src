#	$NetBSD: Makefile,v 1.4 2001/10/21 23:23:21 jmc Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>
.include <bsd.subdir.mk>

KERN=		${KERNOBJDIR}/INSTALL/netbsd
RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
KERNEL=		netbsd.INSTALL.gz

CLEANFILES+=	${KERNEL} netbsd.tmp netbsd.tmp.gz
MDSETIMAGE?=	mdsetimage

all:	${KERNEL}

${KERNEL}: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

.ifndef RELEASEDIR
release:
	@echo set RELEASEDIR first!
	@false
.else
release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNEL} ${RELEASEDIR}/binary/kernel
.endif

clean cleandir distclean:
	rm -f *.core ${CLEANFILES}

.include <bsd.obj.mk>
