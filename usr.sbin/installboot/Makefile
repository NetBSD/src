#	$NetBSD: Makefile,v 1.1 2002/04/03 05:21:16 lukem Exp $
#

PROG=	installboot
SRCS=	installboot.c sum.c machines.c
MAN=	installboot.8

WARNS?=	3

# list of MACHINEs to build this version of installboot for.
#
IB_MACHINES?=	vax

SRCS+=		${IB_MACHINES:S/$/.c/}


# XXX: maybe build as necessary and commit, rather than build each time?
#
CPPFLAGS+=	-I${.CURDIR}
CLEANFILES+=	machines.c

machines.c: mkmachines.awk Makefile
	awk -f ${.CURDIR}/mkmachines.awk ${IB_MACHINES} > ${.TARGET}


.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
