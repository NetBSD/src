#	from: @(#)Makefile	7.13 (Berkeley) 2/26/91
#	$Id: Makefile,v 1.6 1994/02/12 17:52:00 chopps Exp $

PROG=	newfs
CFLAGS+=-DMFS
SRCS=	dkcksum.c newfs.c mkfs.c
MAN8=	newfs.0
LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8
.PATH:	${.CURDIR}/../disklabel

.if (${MACHINE} == "amiga")
CFLAGS+=-D${MACHINE}
.endif

.include <bsd.prog.mk>
