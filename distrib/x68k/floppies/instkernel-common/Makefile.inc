#	$NetBSD: Makefile.inc,v 1.1 2001/12/07 06:57:50 minoura Exp $

.include <bsd.kernobj.mk>

RAMDISKDIR?=	$(.CURDIR)/../ramdisk/
KERN?=		netbsd-xxx
KERNNAME?=	netbsd.xxx
GZNAME?=	${KERNNAME}.gz
MDSETIMAGE?=	mdsetimage

.if ${RAMDISKDIR} != ""
RAMDISK	!=cd ${RAMDISKDIR}; ${MAKE} echoimage
.endif

CLEANFILES+=	${KERNNAME} ${GZNAME} netbsd.tmp*

realall: ${GZNAME}

.if defined(RAMDISK)
${KERNNAME}: ${KERN} ${RAMDISK}
	-/bin/rm -f netbsd.tmp*
	cp ${KERN} netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	mv -f netbsd.tmp ${.TARGET}
.else
${KERNNAME}: ${KERN}
	cp ${KERN} ${.TARGET}
.endif

${GZNAME}: ${KERNNAME}
	gzip -cv9 ${KERNNAME} > ${.TARGET}

depend:

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	-mkdir -p ${RELEASEDIR}/installation/misc
.ifndef NOKERNCOPY
	cp -p ${KERNNAME} ${RELEASEDIR}/binary/kernel
	cp -p ${GZNAME} ${RELEASEDIR}/binary/kernel
.endif

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES}

includes:

.include <bsd.own.mk>
.include <bsd.obj.mk>
