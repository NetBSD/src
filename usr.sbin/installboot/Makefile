#	$NetBSD: Makefile,v 1.15 2002/04/23 02:39:33 lukem Exp $
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


sun68k.c:	sun68k_bbinfo.h

sun68k_bbinfo.h: ${_SRC_TOP_}/sys/arch/sun68k/stand/libsa/bbinfo.h
	@echo "Copying ${.ALLSRC} to ${.TARGET}"
	@cp ${.ALLSRC} ${.TARGET}

CLEANFILES+=	sun68k_bbinfo.h


.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
