#	$NetBSD: Makefile,v 1.23 2003/04/09 22:30:59 dsl Exp $
#

.include <bsd.own.mk>

PROG=	installboot
SRCS=	installboot.c sum.c machines.c fstypes.c \
	ffs.c ffs_bswap.c bbinfo.c \
	alpha.c amiga.c i386.c macppc.c news.c pmax.c \
	sparc.c sparc64.c sun68k.c vax.c x68k.c
MAN=	installboot.8

WARNS?=	3

UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
