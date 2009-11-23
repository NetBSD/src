#	$NetBSD: Makefile,v 1.5 2009/11/23 11:20:25 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern					\
	${.CURDIR}/../../../../dev ${.CURDIR}/../../../../dev/dkwedge

LIB=	rumpdev_disk

#
# XXX: not all platforms support this for now, since disklabel contains
# some MD weirdosities.  Enable only on i386.  Rest of the platforms
# get a dummy implementation (due to it being easier with setlists).

.if ${MACHINE} == "i386"
# sys/kern
SRCS=	subr_disk.c subr_disk_mbr.c subr_disk_open.c
# sys/dev
SRCS+=	dksubr.c dk.c
.else
SRCS+=	dummy.c
.endif

CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
