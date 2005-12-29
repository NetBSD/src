#	$NetBSD: Makefile,v 1.33 2005/12/29 15:32:20 tsutsui Exp $
#

.include <bsd.own.mk>

PROG=	installboot
SRCS=	installboot.c sum.c machines.c fstypes.c \
	ffs.c ffs_bswap.c bbinfo.c \
	alpha.c amiga.c ews4800mips.c hp300.c hp700.c i386.c macppc.c \
	news.c next68k.c pmax.c \
	sparc.c sparc64.c sun68k.c vax.c x68k.c
MAN=	installboot.8

WARNS?=	3

UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CPPFLAGS+=	-I${.CURDIR} -I.
.PATH:		${.CURDIR}/arch ${UFSSRC}/ffs

# XXX
.if ${MACHINE_ARCH} == "m68000"
COPTS.ffs.c=	-O0
.endif

.include <bsd.prog.mk>
