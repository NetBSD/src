#	$NetBSD: Makefile,v 1.20 2002/05/02 18:02:20 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

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
		install.sh upgrade.sh \
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

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
