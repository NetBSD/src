#	$NetBSD: Makefile,v 1.19 2002/05/15 09:44:55 lukem Exp $
#

.include <bsd.own.mk>

PROG=	installboot
SRCS=	installboot.c sum.c machines.c fstypes.c \
	ffs.c ffs_bswap.c bbinfo.c \
	alpha.c macppc.c pmax.c sparc.c sparc64.c sun68k.c vax.c
MAN=	installboot.8

WARNS?=	3

UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
