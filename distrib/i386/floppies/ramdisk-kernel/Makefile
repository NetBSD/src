#	$NetBSD: Makefile,v 1.1 2000/05/05 19:29:28 mycroft Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

KERNDIR=	${.CURDIR}/../../../../sys/arch/i386/compile
COMMONDIR=	${TOP}/bootfloppy-common
LISTS=		${COMMONDIR}/list
RAMDISK	!=cd $(.CURDIR)/../ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNELS=	netbsd.INSTALL.gz netbsd.INSTALL_TINY.gz

all: ${KERNELS}

KERN?=		${.CURDIR}/../../../../sys/arch/i386/compile/INSTALL/netbsd
netbsd.INSTALL.gz: ${KERNDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNDIR}/INSTALL/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

KERN?=		${.CURDIR}/../../../../sys/arch/i386/compile/INSTALL_TINY/netbsd
netbsd.INSTALL_TINY.gz: ${KERNDIR}/INSTALL_TINY/netbsd ${RAMDISK}
	cp ${KERNDIR}/INSTALL_TINY/netbsd netbsd.tmp
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
