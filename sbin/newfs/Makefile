#	@(#)Makefile	7.13 (Berkeley) 2/26/91
#
#	$Id: Makefile,v 1.2 1993/03/22 08:04:00 cgd Exp $

PROG=	newfs
CFLAGS+=-DMFS
SRCS=	dkcksum.c newfs.c mkfs.c
MAN8=	newfs.0
LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mfs.8
.PATH:	${.CURDIR}/../disklabel

.include <bsd.prog.mk>
