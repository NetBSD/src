#	from: @(#)Makefile	7.13 (Berkeley) 2/26/91
#	$Id: Makefile,v 1.5 1993/08/01 05:27:35 mycroft Exp $

PROG=	newfs
CFLAGS+=-DMFS
SRCS=	dkcksum.c newfs.c mkfs.c
MAN8=	newfs.0
LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8
.PATH:	${.CURDIR}/../disklabel

.include <bsd.prog.mk>
