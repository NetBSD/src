#	$NetBSD: Makefile.netwinder.inc,v 1.9 2014/11/15 14:03:37 uebayasi Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

LINKSCRIPT=		-T ${THISARM}/conf/kern.ldscript
