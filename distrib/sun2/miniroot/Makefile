#	$NetBSD: Makefile,v 1.25 2007/06/24 05:41:52 isaki Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		miniroot.fs
IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=8k

KERNEL=		${KERNOBJDIR}/INSTALL/netbsd

WARNS=		1
DBG=		-Os

CRUNCHBIN=	instbin
CRUNCHBIN_FLAGS= -d "${DBG}

LISTS=		${.CURDIR}/list
MTREECONF=	${.CURDIR}/mtree.conf
IMAGEENDIAN=	be
PARSELISTENV=	KERNEL=${KERNEL}
MAKEDEVTARGETS=	std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0
IMAGEDEPENDS=	${CRUNCHBIN} \
		${KERNEL} \
		fstab install.md profile termcap \
		${DESTDIR}/dev/MAKEDEV ${DESTDIR}/dev/MAKEDEV.local \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/group \
		${DESTDIR}/etc/master.passwd ${DESTDIR}/etc/netconfig \
		${DESTDIR}/etc/passwd ${DESTDIR}/etc/protocols \
		${DESTDIR}/etc/pwd.db ${DESTDIR}/etc/services \
		${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/bootxx ${DESTDIR}/usr/mdec/bootyy \
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
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

install.sub: ${DISTRIBDIR}/miniroot/install.sub
	sed -e "/^VERSION=/s/=.*/=${DISTRIBREV}/" < $? > $@

CLEANFILES+= install.sub

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
