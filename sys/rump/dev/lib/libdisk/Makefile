#	$NetBSD: Makefile,v 1.3 2009/09/11 19:01:08 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../dev ${.CURDIR}/../../../../dev/dkwedge

LIB=	rumpdev_disk

# sys/kern
#SRCS=	kern_physio.c subr_disk.c subr_disk_mbr.c subr_disk_open.c

# sys/dev
#SRCS+=	dksubr.c dk.c

SRCS+=	dummy.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
