#	$NetBSD: Makefile.netwinder.inc,v 1.9.2.1 2015/09/22 12:05:48 skrll Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

KERNLDSCRIPT=		${THISARM}/conf/kern.ldscript
