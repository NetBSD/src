#	$NetBSD: Makefile,v 1.1 2000/06/10 18:56:49 he Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

RAMDISKDIR=	${.CURDIR}/../ramdisk-rescue
COMMONDIR=	${TOP}/bootfloppy-common
LISTS=		${COMMONDIR}/list
RAMDISK!=	cd ${RAMDISKDIR}; \
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

${RAMDISK}:
	cd ${RAMDISKDIR}; ${MAKE}

release:

clean cleandir distclean:
	rm -f *.core ${KERNELS}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
