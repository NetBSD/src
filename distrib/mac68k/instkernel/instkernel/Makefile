#	$NetBSD: Makefile,v 1.8 2002/03/04 16:43:10 shiba Exp $

TOP=            ${.CURDIR}/..

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

MDSETIMAGE?=	mdsetimage
INSTALL_KERNEL?=netbsd-INSTALL
INSTALL_KERNELSBC?=netbsd-INSTALLSBC
KERNEL=${KERNOBJDIR}/INSTALL/netbsd
KERNELSBC=${KERNOBJDIR}/INSTALLSBC/netbsd

${INSTALL_KERNEL}.gz: .NOTMAIN ${KERNOBJDIR}/INSTALL/netbsd ${RAMDISK}
	cp ${KERNEL} ${INSTALL_KERNEL}
	${MDSETIMAGE} -v ${INSTALL_KERNEL} ${RAMDISK}
	gzip -f ${INSTALL_KERNEL}

${INSTALL_KERNELSBC}.gz: .NOTMAIN ${KERNOBJDIR}/INSTALLSBC/netbsd ${RAMDISK}
	cp ${KERNELSBC} ${INSTALL_KERNELSBC}
	${MDSETIMAGE} -v ${INSTALL_KERNELSBC} ${RAMDISK}
	gzip -f ${INSTALL_KERNELSBC}

KERNELS=${INSTALL_KERNEL}.gz ${INSTALL_KERNELSBC}.gz

all:  ${KERNELS}

release: all
	${INSTALL} -d -m 755 ${RELEASEDIR}/installation/instkernel/ \
		${RELEASEDIR}/installation/instkernel/
	${INSTALL} ${COPY} -o root -g wheel -m 644 ${INSTALL_KERNEL}.gz \
		${RELEASEDIR}/installation/instkernel/  
	${INSTALL} ${COPY} -o root -g wheel -m 644 ${INSTALL_KERNELSBC}.gz \
		${RELEASEDIR}/installation/instkernel/

clean cleandir distclean:
	rm -f netbsd*

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
