#	$NetBSD: Makefile,v 1.14 2002/04/22 23:23:05 bjh21 Exp $
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
# list of MACHINEs to enable the compat symlink /usr/mdec/installboot
# XXX: eventually this should be done for all machines.
# XXX: or /usr/mdec/installboot is replaced with a shell script that
# XXX: tells the user to use /usr/sbin/installboot instead
#
COMPATLINKS?=	pmax sun2
afterinstall:
.for l in ${COMPATLINKS}
.if (${MACHINE} == ${l})
	${INSTALL_SYMLINK} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${BINDIR}/${PROG} ${DESTDIR}/usr/mdec/${PROG}
.endif
.endfor

.include <bsd.prog.mk>
.endif
