#	$NetBSD: Makefile,v 1.1 2000/05/23 18:34:58 matt Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

RAMDISK!=	cd $(.CURDIR)/../ramdisk/; \
	        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
		${MAKE} -s -f-

.for kernel in INSTALL
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
