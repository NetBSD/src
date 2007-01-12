#	$NetBSD: Makefile.zaurus.inc,v 1.2.6.2 2007/01/12 01:01:03 ad Exp $

MACHINE_ARCH=		arm
CPPFLAGS+=		-D${MACHINE}

SYSTEM_FIRST_OBJ=	zaurus_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/zaurus/zaurus_start.S

LINKFLAGS=		-T ${THISARM}/conf/ldscript

EXTRA_CLEAN+= netbsd.map assym.d
