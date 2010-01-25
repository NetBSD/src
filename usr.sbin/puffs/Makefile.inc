#	$NetBSD: Makefile.inc,v 1.13 2010/01/25 15:55:36 pooka Exp $

WARNS?=	4

.include <bsd.own.mk>
.include <bsd.sys.mk>

USE_FORT?= yes	# network client

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.ifdef ISRUMP
.ifdef MOUNTNAME
PROG=		rump_${MOUNTNAME}
MAN=		rump_${MOUNTNAME}.8

MOUNTDIR=	${.CURDIR}/../../../sbin
MOUNT=		${MOUNTDIR}/mount

CPPFLAGS+=	-I${MOUNTDIR}/mount_${MOUNTNAME} -I${MOUNT} -DMOUNT_NOMAIN
SRCS+=		mount_${MOUNTNAME}.c rump_${MOUNTNAME}.c pathadj.c fattr.c

# on select archs use the kernel module directly, otherwise fallback
# to the rump library
.ifndef DONOTLINKLIBS
.undef RUMPKMOD
.ifdef RUMPKMOD
OSRELEASE!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh
KMODULEDIR=	${DESTDIR}/stand/${MACHINE}/${OSRELEASE}/modules/${MOUNTNAME}
LDADD+=		${KMODULEDIR}/${MOUNTNAME}.kmod
.else
LDADD+=		-lrumpfs_${MOUNTNAME}
.endif
.endif

.PATH:		${MOUNT} ${MOUNTDIR}/mount_${MOUNTNAME}
.endif # MOUNTNAME

# disk components for disk-based file systems
.if RUMP_DISKFS
LDADD+=		-lrumpdev_disk -lrumpdev
.endif

LDADD+=		-lp2k -lukfs -lrumpvfs -lrump -lrumpuser -lpuffs -lutil
LDADD+=		-lpthread

DPADD+=		${LIBP2K} ${LIBUKFS} ${LIBRUMPVFS} ${LIBRUMP} ${LIBRUMPUSER}
DPADD+=		${LIBPUFFS} ${LIBUTIL} ${LIBPTHREAD}
.endif # ISRUMP
