#	$NetBSD: Makefile,v 1.2 2000/05/05 20:13:33 mycroft Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

COMMONDIR=	${TOP}/bootfloppy-common
LISTS=		${COMMONDIR}/list
RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNELS=	netbsd.INSTALL.gz netbsd.INSTALL_TINY.gz

all: ${KERNELS}

netbsd.INSTALL.gz: ${KERNOBJDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

netbsd.INSTALL_TINY.gz: ${KERNOBJDIR}/INSTALL_TINY/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL_TINY/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${KERNELS}

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
