#	$NetBSD: Makefile,v 1.2 2002/04/03 05:58:44 lukem Exp $
#

PROG=	installboot
SRCS=	installboot.c sum.c machines.c
MAN=	installboot.8

WARNS?=	3

# list of MACHINEs to build this version of installboot for.
#
IB_MACHINES?=	vax

SRCS+=		${IB_MACHINES:S/$/.c/}


regenmachines: mkmachines.awk Makefile
	-mv -f ${.CURDIR}/machines.c ${.CURDIR}/machines.c.bak
	awk -f ${.CURDIR}/mkmachines.awk ${IB_MACHINES} > ${.TARGET}
	mv ${.TARGET} ${.CURDIR}/machines.c


.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
