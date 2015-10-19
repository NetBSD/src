#	$NetBSD: Makefile,v 1.9 2015/10/19 16:16:32 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../dev ${.CURDIR}/../../../../dev/dkwedge

LIB=	rumpdev_disk
COMMENT=Disk-like device support (used e.g. by file systems)

SRCS=	disk_component.c

#
# We use subr_disk_mbr on all platforms.  The current structure of
# code allows us to pick only one readdisklabel() routine.  While
# this is not the native one for all platforms, it's probably the
# most common one in an image floating on the internetto.
SRCS+=	subr_disk_mbr.c

# sys/kern
SRCS+=	subr_disk.c subr_disk_open.c
# sys/dev
SRCS+=	dksubr.c dk.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
