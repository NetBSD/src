#	$NetBSD: Makefile,v 1.10 2002/04/22 21:17:58 fredette Exp $

TOP=		${.CURDIR}/..

.include <bsd.own.mk>
.include <bsd.kernobj.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

IMG=		ramdisk
IMAGE=		${IMG}.fs
IMAGESIZE=	400k
MAKEFS_FLAGS=	-f 15 -o minfree=0,optimization=space,density=4096,cpg=20

WARNS=		1
DBG=		-Os

CRUNCHBIN=	rd_bin
LISTS=		${.CURDIR}/${IMG}.list
PARSELISTENV=	TOPDIR=${TOP}
MTREECONF=	${TOP}/common/${IMG}.tree
IMAGEENDIAN=	be
MAKEDEVTARGETS=	__ramdisk std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0
IMAGEDEPENDS=	${CRUNCHBIN} \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

MDSETIMAGE?=	mdsetimage
KERNEL=		${KERNOBJDIR}/RAMDISK/netbsd
KERNELS=	netbsd.RAMDISK
CLEANFILES+=	${KERNELS}

netbsd.RAMDISK : ${IMAGE} ${KERNEL}
	cp ${KERNEL} netbsd-tmp
	${MDSETIMAGE} -v netbsd-tmp ${IMAGE}
	-mv -f netbsd-tmp $@

realall: ${IMAGE} ${KERNELS}

.if !defined(RELEASEDIR)
release:
	@echo setenv RELEASEDIR before doing that!
	@false
.else	# RELEASEDIR
release: $(KERNELS)
.for x in ${KERNELS}
	gzip -c -9 < ${x} > \
	 ${RELEASEDIR}/binary/kernel/${x}.gz
.endfor # KERNELS
.endif	# RELEASEDIR

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
