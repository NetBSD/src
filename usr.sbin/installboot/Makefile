#	$NetBSD: Makefile,v 1.18 2002/05/06 16:24:45 pk Exp $
#

.include <bsd.own.mk>

PROG=	installboot
SRCS=	installboot.c sum.c machines.c fstypes.c \
	ffs.c ffs_bswap.c \
	alpha.c pmax.c sparc.c sparc64.c sun68k.c vax.c
MAN=	installboot.8

WARNS?=	3

UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
