#	$NetBSD: Makefile,v 1.2 2000/06/17 18:37:44 tsutsui Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.subdir.mk>

KERN=		${KERNOBJDIR}/INSTALL/netbsd
.if !make(print-objdir)
RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${MAKE} print-objdir
.endif
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
KERNEL=		netbsd.INSTALL.gz

CLEANFILES+=	${KERNEL} netbsd.tmp netbsd.tmp.gz

all:	${KERNEL}

${KERNEL}: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	mdsetimage -v netbsd.tmp ${RAMDISK}
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
