#	$NetBSD: Makefile,v 1.26 2009/09/20 19:49:07 abs Exp $

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
		disktab.preinstall dot.profile \
		install.sh upgrade.sh \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/bootxx_ffs \
		${DESTDIR}/usr/share/misc/termcap \
		${DISTRIBDIR}/utils/script-installer/dot.commonutils \
		${DISTRIBDIR}/utils/script-installer/dot.instutils \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services

INSTNOTESDIR!=	cd ${DISTRIBDIR}/notes/alpha && ${PRINTOBJDIR}
INSTALLNOTES=	${INSTNOTESDIR}/INSTALL.more
IMAGEDEPENDS+=	${INSTALLNOTES}
PARSELISTENV=	INSTALLNOTES=${INSTALLNOTES:Q} TOOL_PWD_MKDB=${TOOL_PWD_MKDB:Q}

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
