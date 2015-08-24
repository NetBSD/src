#	$NetBSD: Makefile.netwinder.inc,v 1.11 2015/08/24 14:04:24 uebayasi Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

KERNLDSCRIPT=		${THISARM}/conf/kern.ldscript
