#	$NetBSD: Makefile,v 1.5 2002/03/17 21:34:38 lukem Exp $

.include "${.CURDIR}/../../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

KERN=		${KERNOBJDIR}/INSTALL/netbsd
RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
KERNEL=		netbsd.INSTALL.gz

CLEANFILES+=	${KERNEL} netbsd.tmp netbsd.tmp.gz

all: ${KERNEL}

${KERNEL}: ${KERN} ${RAMDISK}
	cp ${KERN} netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${RAMDISK}
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNEL} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
