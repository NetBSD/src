#	$NetBSD: Makefile,v 1.9 2002/04/06 15:27:55 bjh21 Exp $
#

PROG=	installboot
SRCS=	installboot.c sum.c machines.c
MAN=	installboot.8

WARNS?=	3

CPPFLAGS+=	-I${.CURDIR}
.PATH:		${.CURDIR}/arch

# list of MACHINEs to enable MD back-ends for
#
IB_MACHINES?=	alpha pmax sparc64 vax

SRCS+=		${IB_MACHINES:S/$/.c/}


.ifndef HOSTPROG
# list of MACHINEs to enable the compat symlink /usr/mdec/installboot
# XXX: eventually this should be done for all machines in ${IB_MACHINES}
#
COMPATLINKS?=	pmax
afterinstall:
.for l in ${COMPATLINKS}
.if (${MACHINE} == ${l})
	${INSTALL_SYMLINK} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${BINDIR}/${PROG} ${DESTDIR}/usr/mdec/${PROG}
.endif
.endfor


regenmachines: mkmachines.awk Makefile
	-mv -f ${.CURDIR}/machines.c ${.CURDIR}/machines.c.bak
	awk -f ${.CURDIR}/mkmachines.awk ${IB_MACHINES} > ${.TARGET}
	mv ${.TARGET} ${.CURDIR}/machines.c


.include <bsd.prog.mk>
.endif
