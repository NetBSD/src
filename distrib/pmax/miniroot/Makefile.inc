#	$NetBSD: Makefile.inc,v 1.22 2010/02/05 10:01:20 roy Exp $

.if ${MACHINE_ARCH} == "mips64el"
SFX=		64
.endif

DBG=		-Os

IMAGESIZE=	32m
MAKEFS_FLAGS=	-f 15
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS=		${.CURDIR}/list ${ARCHDIR}/list${SFX}
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/rescue/terminfo.db \
		${DESTDIR}/.profile ${DESTDIR}/boot.pmax \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC${SFX}/netbsd

.if ${MACHINE_ARCH} == "mipsel"
IMAGEDEPENDS+=	${KERNOBJDIR}/GENERIC/netbsd.ecoff \
		${KERNOBJDIR}/INSTALL/nfsnetbsd.ecoff
.endif

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
