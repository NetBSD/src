#	$NetBSD: Makefile,v 1.3 2000/05/06 05:32:29 mycroft Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

COMMONDIR=	${TOP}/bootfloppy-common
LISTS=		${COMMONDIR}/list
RAMDISK!=	cd $(.CURDIR)/../ramdisk/; \
	        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
		${MAKE} -s -f-

.for kernel in INSTALL INSTALL_SMALL INSTALL_TINY
netbsd.${kernel}.gz: .NOTMAIN ${KERNOBJDIR}/${kernel}/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/${kernel}/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
KERNELS+=	netbsd.${kernel}.gz
.endfor

all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${KERNELS}

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
