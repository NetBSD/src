#	$NetBSD: Makefile,v 1.1.2.4 2010/03/11 15:04:33 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../dev ${.CURDIR}/../../../../dev/dkwedge

LIB=	rumpdev_disk

#
# We use subr_disk_mbr on all platforms.  The current structure of
# code allows us to pick only one readdisklabel() routine.  While
# this is not the native one for all platforms, it's probably the
# most common one in an image floating on the internetto.
SRCS=	subr_disk_mbr.c

# sys/kern
SRCS+=	subr_disk.c subr_disk_open.c
# sys/dev
SRCS+=	dksubr.c dk.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
