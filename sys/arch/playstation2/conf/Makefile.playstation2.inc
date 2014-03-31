#	$NetBSD: Makefile.playstation2.inc,v 1.4 2014/03/31 11:25:49 martin Exp $

EETOOLDIR	=	/usr/pkg/cross-ps2
TARGET	=	mipsEEel-netbsd

CC	=	${EETOOLDIR}/bin/${TARGET}-gcc
LD	=	${EETOOLDIR}/bin/${TARGET}-ld
CXX	=	${EETOOLDIR}/bin/${TARGET}-g++
AS	=	${EETOOLDIR}/bin/${TARGET}-as
CPP	=	${EETOOLDIR}/bin/${TARGET}-cpp
RANLIB	=	${EETOOLDIR}/bin/${TARGET}-ranlib
AR	=	${EETOOLDIR}/bin/${TARGET}-ar
NM	=	${EETOOLDIR}/bin/${TARGET}-nm
SIZE	=	${EETOOLDIR}/bin/${TARGET}-size
STRIP	=	${EETOOLDIR}/bin/${TARGET}-strip

CFLAGS+= -mcpu=r5900 -mips1 -fno-pic \
		-D__NetBSD__ -Ulinux -U__linux__ -U__linux
AFLAGS+= -mcpu=r5900 -mips1 -fno-pic \
		-D__NetBSD__ -Ulinux -U__linux__ -U__linux
