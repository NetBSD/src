#	$NetBSD: Makefile,v 1.1.2.2 2009/06/20 07:20:35 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../dev ${.CURDIR}/../../../../dev/dkwedge

LIB=	rumpdev_disk

# sys/kern
SRCS=	kern_physio.c subr_disk.c subr_disk_mbr.c

# sys/dev
SRCS+=	dksubr.c dk.c

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
