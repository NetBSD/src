#	$NetBSD: Makefile.netwinder.inc,v 1.2.4.2 2002/01/08 00:26:39 nathanw Exp $

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S
