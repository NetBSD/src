#	$NetBSD: Makefile,v 1.19 2002/04/26 15:50:24 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

IMAGE=		ramdisk.fs
IMAGESIZE=	4480k
MAKEFS_FLAGS=	-o density=5k

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
MAKEDEVTARGETS=	minimal
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.hdprofile dot.profile \
		${.CURDIR}/../src/install.sh ${.CURDIR}/../src/upgrade.sh \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/bootxx_ffs \
		${DESTDIR}/usr/share/misc/termcap \
		${DISTRIBDIR}/utils/script-installer/dot.commonutils \
		${DISTRIBDIR}/utils/script-installer/dot.instutils \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${NETBSDSRCDIR}/etc/protocols \
		${NETBSDSRCDIR}/etc/services

INSTALLNOTES!=	cd ${DISTRIBDIR}/notes/alpha && ${MAKE} echomore 
IMAGEDEPENDS+=	${INSTALLNOTES}
PARSELISTENV=	INSTALLNOTES=${INSTALLNOTES:Q} PWD_MKDB=${PWD_MKDB:Q}


realall: ${IMAGE}

release:


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
