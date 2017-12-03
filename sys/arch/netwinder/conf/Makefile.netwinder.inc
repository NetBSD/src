#	$NetBSD: Makefile.netwinder.inc,v 1.8.122.1 2017/12/03 11:36:31 jdolecek Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S

KERNLDSCRIPT=		${THISARM}/conf/kern.ldscript
