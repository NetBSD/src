#	$NetBSD: Makefile.netwinder.inc,v 1.1.2.1 2002/01/10 19:46:41 thorpej Exp $

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S
