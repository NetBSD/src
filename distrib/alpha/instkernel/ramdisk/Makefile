#	$NetBSD: Makefile,v 1.15 2002/03/07 23:51:48 lukem Exp $

.include "${.CURDIR}/../../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	9000b
MAKEFS_FLAGS=	-o density=5k

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common #${.CURDIR}/mtree.conf
IMAGEENDIAN=	le
MAKEDEVTARGETS=	minimal
IMAGEDEPENDS+=	${CRUNCHBIN} \
		disktab.preinstall dot.hdprofile dot.profile \
		${.CURDIR}/../src/install.sh ${.CURDIR}/../src/upgrade.sh \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/bootxx_ffs \
		${DESTDIR}/usr/share/misc/termcap \
		${_SRC_TOP_}/distrib/utils/script-installer/dot.commonutils \
		${_SRC_TOP_}/distrib/utils/script-installer/dot.instutils \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services

INSTALLNOTES!=	cd ${DISTRIBDIR}/notes/alpha && ${MAKE} echomore 
IMAGEDEPENDS+=	${INSTALLNOTES}
POPULATEENV+=	INSTALLNOTES=${INSTALLNOTES:Q} PWD_MKDB=${PWD_MKDB:Q}


realall: ${IMAGE}

release:


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
