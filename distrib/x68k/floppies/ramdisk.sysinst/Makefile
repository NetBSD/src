#	$NetBSD: Makefile,v 1.16 2002/05/07 14:08:34 isaki Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

TOP=		${.CURDIR}/..
COMMONDIR=	${TOP}/common

IMAGE=		ramdisk.fs
IMAGESIZE=	1228800

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common ${.CURDIR}/mtree.conf
MAKEDEVTARGETS=	std md0 fd0 sd0 sd1 sd2 sd3 sd4 sd5 cd0 ttye0 tty00 opty

AUXDEPENDS= 	\
		dot.profile \
		dot.hdprofile \
		services \
		${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/group \
		${NETBSDSRCDIR}/etc/protocols \
		${NETBSDSRCDIR}/etc/netconfig \
		${DESTDIR}/usr/mdec/sdboot_ufs \
		${DESTDIR}/usr/mdec/boot \
		${DESTDIR}/usr/mdec/mboot \
		${COMMONDIR}/termcap.vt \
		${COMMONDIR}/disktab.preinstall

includes:

.include "${COMMONDIR}/Makefile.ramdisk"
