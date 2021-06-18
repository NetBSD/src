#	$NetBSD: Makefile,v 1.50 2021/06/18 23:55:20 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		miniroot.fs
IMAGESIZE=	6500k
MAKEFS_FLAGS+=	-o density=8k

KERNEL3=	${KERNOBJDIR}/INSTALL/netbsd
KERNEL3X=	${KERNOBJDIR}/INSTALL3X/netbsd

WARNS=		1
DBG=		-Os
USE_WIDECHAR=	no

CRUNCHBIN=	instbin

LISTS=		${.CURDIR}/list
MTREECONF=	${.CURDIR}/mtree.conf
IMAGEENDIAN=	be
PARSELISTENV=	KERNEL3=${KERNEL3} KERNEL3X=${KERNEL3X}
MAKEDEVTARGETS=	std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0 fd0
IMAGEDEPENDS=	${CRUNCHBIN} \
		${KERNEL3} ${KERNEL3X} \
		fstab install.md profile \
		${DESTDIR}/dev/MAKEDEV ${DESTDIR}/dev/MAKEDEV.local \
		${DESTDIR}/etc/group \
		${DESTDIR}/etc/master.passwd ${DESTDIR}/etc/netconfig \
		${DESTDIR}/etc/passwd \
		${DISTRIBDIR}/common/protocols \
		${DESTDIR}/etc/pwd.db ${DISTRIBDIR}/common/services \
		${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/bootxx \
		${DESTDIR}/usr/mdec/netboot ${DESTDIR}/usr/mdec/tapeboot \
		${DESTDIR}/usr/mdec/ufsboot \
		${DISTRIBDIR}/miniroot/install.sh \
		${DISTRIBDIR}/miniroot/upgrade.sh \
		install.sub

IMAGEPOSTBUILD=		${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    	    ${DESTDIR}/usr/mdec/bootxx /ufsboot
IMAGE_RELEASEDIR=	installation/miniroot

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
HACK_CURSES=yes
.MAKEFLAGS+=	USE_WIDECHAR=no
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

install.sub: ${DISTRIBDIR}/miniroot/install.sub
	${TOOL_SED} -e "/^VERSION=/s/=.*/=${DISTRIBREV}/" \
		    -e "/^RELEASE=/s/=.*/=${DISTRIBVER}/" \
		    -e "/^MACHINE=/s/=.*/=${MACHINE}/" < $? > $@

CLEANFILES+= install.sub

fstab: .PHONY

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
