#	$NetBSD: Makefile,v 1.5.2.1 2000/09/04 04:02:47 jhawk Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip
STRIPFLAGS?=	--strip-unneeded \
		    -K pcic_isa_alloc_iobase -K pcic_isa_alloc_iosize \
		    -K pcic_isa_intr_alloc_mask \
		    -K pcic_irq_probe

COMMONDIR=	${TOP}/bootfloppy-common
LISTS=		${COMMONDIR}/list
RAMDISK!=	cd $(.CURDIR)/../ramdisk/; \
	        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
		${MAKE} -s -f-
RAMDISK_T!=	cd ${.CURDIR}/../ramdisk-tiny/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-tiny.fs\n" | \
		${MAKE} -s -f-

netbsd.INSTALL.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	nm netbsd.tmp > netbsd.INSTALL.symbols
	${STRIP} ${STRIPFLAGS} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

netbsd.INSTALL_TINY.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL_TINY/netbsd ${RAMDISK_T}
	cp ${KERNOBJDIR}/INSTALL_TINY/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK_T}
	nm netbsd.tmp > netbsd.INSTALL_TINY.symbols
	${STRIP} ${STRIPFLAGS} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

KERNELS=netbsd.INSTALL.gz netbsd.INSTALL_TINY.gz
KERNELSYMS=netbsd.INSTALL.symbols netbsd.INSTALL_TINY.symbols

all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${RELEASEDIR}/binary/kernel
	cp ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${KERNELS}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
