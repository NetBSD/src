#	$NetBSD: Makefile.evbarm.inc,v 1.1 2001/10/27 16:14:04 rearnsha Exp $

.if (${BOARDTYPE} == "integrator")
SYSTEM_FIRST_OBJ=	intmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/integrator/intmmu.S 
.endif
