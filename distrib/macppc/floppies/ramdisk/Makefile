#	$NetBSD: Makefile,v 1.54 2023/07/24 08:21:09 rin Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
# This must be kept in sync with config option MEMORY_DISK_ROOT_SIZE in
#   src/sys/arch/macppc/conf/INSTALL
#   src/sys/arch/macppc/conf/INSTALL_601
#   src/sys/arch/macppc/conf/GENERIC_MD
IMAGESIZE=	2880k
MAKEFS_FLAGS+=	-f 15

WARNS=		1

# No point in using 64bit binaries (if building for MACHINE_ARCH=powerpc64)
# on the ramdisk, keep it small
DBG=		-Os -m32
HACK_LD_FLAGS=	-m elf32ppc_nbsd
LDFLAGS+=	-m32 -Wl,-melf32ppc_nbsd

CRUNCHBIN=	ramdiskbin
CRUNCHENV+=	LDFLAGS=${LDFLAGS:Q}
# Enable Apple UFS support for x_fsck_ffs and x_newfs
CRUNCHENV+=	APPLE_UFS=1
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
		${DESTDIR}/usr/mdec/ofwboot ${DESTDIR}/usr/mdec/bootxx \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services
SMALLPROG_INET6=1

.if ${USE_INET6} != "no"
LISTS+=		${DISTRIBDIR}/common/list.inet6
.endif

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
