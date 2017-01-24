#	$NetBSD: Makefile.inc,v 1.25 2017/01/24 18:04:04 christos Exp $

.if ${MACHINE_ARCH} == "mips64el"
SFX=		64
.endif

DBG=		-Os

IMAGESIZE=	32m
MAKEFS_FLAGS+=	-f 15
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS=		${.CURDIR}/list ${ARCHDIR}/list${SFX}
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile ${DESTDIR}/boot.pmax \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC${SFX}/netbsd

.if ${MACHINE_ARCH} == "mipsel"
IMAGEDEPENDS+=	${KERNOBJDIR}/GENERIC/netbsd.ecoff \
		${KERNOBJDIR}/INSTALL/nfsnetbsd.ecoff
.endif

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
