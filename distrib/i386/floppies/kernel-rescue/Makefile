#	$NetBSD: Makefile,v 1.2.2.3 2002/02/09 20:26:48 he Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

RAMDISK_T!=	cd ${.CURDIR}/../ramdisk-rescuetiny/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-rescuetiny.fs\n" | \
		${MAKE} -s -f-
RAMDISK_S!=	cd ${.CURDIR}/../ramdisk-rescuesmall/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-rescuesmall.fs\n" | \
		${MAKE} -s -f-

netbsd.INSTALL_TINY.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL_TINY/netbsd ${RAMDISK_T}
	cp ${KERNOBJDIR}/INSTALL_TINY/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK_T}
	nm netbsd.tmp > netbsd.INSTALL_TINY.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

netbsd.INSTALL_SMALL.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL_SMALL/netbsd ${RAMDISK_S}
	cp ${KERNOBJDIR}/INSTALL_SMALL/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK_S}
	nm netbsd.tmp > netbsd.INSTALL_SMALL.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

KERNELS=netbsd.INSTALL_TINY.gz netbsd.INSTALL_SMALL.gz
KERNELSYMS=	netbsd.INSTALL_TINY.symbols netbsd.INSTALL_SMALL.symbols

all: ${KERNELS}

release:

clean cleandir distclean:
	rm -f *.core ${KERNELS} ${KERNELSYMS}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
