#	$NetBSD: Makefile.zaurus.inc,v 1.2.2.2 2006/12/18 11:42:09 yamt Exp $

MACHINE_ARCH=		arm
CPPFLAGS+=		-D${MACHINE}

SYSTEM_FIRST_OBJ=	zaurus_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/zaurus/zaurus_start.S

LINKFLAGS=		-T ${THISARM}/conf/ldscript

EXTRA_CLEAN+= netbsd.map assym.d
