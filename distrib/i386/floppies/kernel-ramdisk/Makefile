#	$NetBSD: Makefile,v 1.5 2001/07/21 14:46:09 jdolecek Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

MDEC=		${DESTDIR}/usr/mdec
STRIP?=		strip

#COMMONDIR=	${TOP}/bootfloppy-common
#LISTS=		${COMMONDIR}/list
RAMDISK!=	cd $(.CURDIR)/../ramdisk-big/; \
	        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-big.fs\n" | \
		${MAKE} -s -f-
RAMDISK_T!=	cd ${.CURDIR}/../ramdisk-tiny/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-tiny.fs\n" | \
		${MAKE} -s -f-
RAMDISK_S!=	cd ${.CURDIR}/../ramdisk-small/; \
		printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk-small.fs\n" | \
		${MAKE} -s -f-

netbsd.INSTALL.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	nm netbsd.tmp > netbsd.INSTALL.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

netbsd.INSTALL_LAPTOP.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL_LAPTOP/netbsd ${RAMDISK}
	cp ${KERNOBJDIR}/INSTALL_LAPTOP/netbsd netbsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK}
	nm netbsd.tmp > netbsd.INSTALL_LAPTOP.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

netbsd.INSTALL_TINY.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL_TINY/netbsd ${RAMDISK_T}
	cp ${KERNOBJDIR}/INSTALL_TINY/netbsd netbsd.tmp
	gdb --write -batch -x ${.CURDIR}/nocluster.gdb netbsd.tmp </dev/null
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

netbsd.INSTALL_PS2.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL_PS2/netbsd ${RAMDISK_S}
	cp ${KERNOBJDIR}/INSTALL_PS2/netbsd netbsd.tmp
	gdb --write -batch -x ${.CURDIR}/nocluster.gdb netbsd.tmp </dev/null
	${DESTDIR}/usr/sbin/mdsetimage -v netbsd.tmp ${RAMDISK_S}
	nm netbsd.tmp > netbsd.INSTALL_PS2.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

KERNELS=netbsd.INSTALL.gz netbsd.INSTALL_TINY.gz netbsd.INSTALL_SMALL.gz \
	netbsd.INSTALL_LAPTOP.gz netbsd.INSTALL_PS2.gz
KERNELSYMS=netbsd.INSTALL.symbols netbsd.INSTALL_TINY.symbols \
	   netbsd.INSTALL_SMALL.symbols netbsd.INSTALL_LAPTOP.symbols \
	   netbsd.INSTALL_PS2.symbols

all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core ${KERNELS} ${KERNELSYMS}

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
