#	@(#)Makefile	7.13 (Berkeley) 2/26/91
#
#	$Header: /cvsroot/src/sbin/newfs/Makefile,v 1.3 1993/03/23 00:29:15 cgd Exp $

PROG=	newfs
CFLAGS+=-DMFS
SRCS=	dkcksum.c newfs.c mkfs.c
MAN8=	newfs.0
LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mfs.8
.PATH:	${.CURDIR}/../disklabel

.include <bsd.prog.mk>
