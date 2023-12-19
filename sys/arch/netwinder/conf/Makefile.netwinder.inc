#	$NetBSD: Makefile.netwinder.inc,v 1.11.48.1 2023/12/19 12:49:40 martin Exp $

CPPFLAGS+=	-D${MACHINE}

SYSTEM_FIRST_OBJ=	nwmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/${MACHINE}/nwmmu.S
ENTRYPOINT=		nwstart

# don't let Makefile.kern.inc override placement of the text segment
# specified in the ld script
KERNLDSCRIPT=		${THISARM}/conf/kern.ldscript
LINKTEXT=		# suppress -Ttext
