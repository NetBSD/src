#	$NetBSD: Makefile.netwinder.inc,v 1.4 2001/11/27 00:16:01 thorpej Exp $

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S
