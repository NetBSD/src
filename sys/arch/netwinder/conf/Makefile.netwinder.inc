#	$NetBSD: Makefile.netwinder.inc,v 1.10 2015/08/20 06:22:18 uebayasi Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

KERNLDSCRIPT=		${THISARM}/conf/kern.ldscript
LINKSCRIPT=		-T ${KERNLDSCRIPT}
