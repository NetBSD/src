#	$NetBSD: Makefile,v 1.6 2002/04/22 21:17:57 fredette Exp $

TOP=		${.CURDIR}/..

.include <bsd.own.mk>
.include <bsd.kernobj.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

IMG=		miniroot
IMAGE=		${IMG}
IMAGESIZE=	12m
MAKEFS_FLAGS=	-o minfree=0,optimization=space,density=8k,cpg=16

KERNEL=		${KERNOBJDIR}/INSTALL/netbsd

WARNS=		1
DBG=		-Os

LISTS=		${.CURDIR}/${IMG}.list
PARSELISTENV=	TOPDIR=${TOP} KERNEL=${KERNEL}
MTREECONF=	${TOP}/common/${IMG}.tree
IMAGEENDIAN=	be
MAKEDEVTARGETS=	std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0 fd0
IMAGEDEPENDS=	${KERNEL} install.sub

realall: ${IMG} ib.stamp

ib.stamp: ${IMG}
	${INSTALLBOOT} -v ${IMG} ${DESTDIR}/usr/mdec/bootxx /ufsboot
	touch ib.stamp

install.sub : ${TOP}/../miniroot/install.sub
	sed -e "/^VERSION=/s/=.*/=${DISTRIBREV}/" < $? > $@

${IMAGE}.gz: ${IMAGE}
	gzip -c -9 < ${IMAGE} > ${IMAGE}.gz

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: ${IMAGE}.gz
	-mkdir -p ${RELEASEDIR}/installation/miniroot
	${RELINSTALL} ${IMAGE}.gz ${RELEASEDIR}/installation/miniroot
.endif	# RELEASEDIR

.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
