#	$NetBSD: Makefile,v 1.16 2002/04/24 01:35:25 lukem Exp $
#

.include <bsd.own.mk>

PROG=	installboot
SRCS=	installboot.c sum.c machines.c fstypes.c \
	ffs.c ffs_bswap.c \
	alpha.c pmax.c sparc64.c sun68k.c vax.c
MAN=	installboot.8

WARNS?=	3

UFSSRC=		${_SRC_TOP_}/sys/ufs
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
