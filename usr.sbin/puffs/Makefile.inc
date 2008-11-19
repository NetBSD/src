#	$NetBSD: Makefile.inc,v 1.6 2008/11/19 14:10:49 pooka Exp $

USE_FORT?= yes	# network client

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.ifdef(ISRUMP)
.ifdef(MOUNTNAME)
PROG=		rump_${MOUNTNAME}
MAN=		rump_${MOUNTNAME}.8

MOUNTDIR=	${.CURDIR}/../../../sbin
MOUNT=		${MOUNTDIR}/mount

CPPFLAGS+=	-I${MOUNTDIR}/mount_${MOUNTNAME} -I${MOUNT} -DMOUNT_NOMAIN
SRCS+=		mount_${MOUNTNAME}.c rump_${MOUNTNAME}.c pathadj.c fattr.c

LDADD+=		-lrumpfs_${MOUNTNAME}
.PATH:		${MOUNT} ${MOUNTDIR}/mount_${MOUNTNAME}
.endif # MOUNTNAME

LDADD+=		-lp2k -lukfs -lrumpvfs -lrump -lrumpuser -lpuffs -lutil
LDADD+=		-lpthread

DPADD+=		${LIBP2K} ${LIBUKFS} ${LIBRUMPVFS} ${LIBRUMP} ${LIBRUMPUSER}
DPADD+=		${LIBPUFFS} ${LIBUTIL} ${LIBPTHREAD}

LDFLAGS+=	-Wl,--wrap=malloc
.endif # ISRUMP
