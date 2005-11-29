#	$NetBSD: Makefile.netwinder.inc,v 1.7.24.1 2005/11/29 21:23:02 yamt Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKFLAGS=		-T ${THISARM}/conf/kern.ldscript
