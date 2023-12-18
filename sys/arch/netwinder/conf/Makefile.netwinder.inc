#	$NetBSD: Makefile.netwinder.inc,v 1.12 2023/12/18 23:05:25 uwe Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S
ENTRYPOINT=		nwstart

# don't let Makefile.kern.inc override placement of the text segment
# specified in the ld script
KERNLDSCRIPT=		${THISARM}/conf/kern.ldscript
LINKTEXT=		# suppress -Ttext
