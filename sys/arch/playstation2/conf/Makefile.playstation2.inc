#	$NetBSD: Makefile.playstation2.inc,v 1.1 2001/10/16 15:38:31 uch Exp $

TOOLDIR	=	/usr/pkg/cross-ps2
TARGET	=	mipsEEel-netbsd

CC	=	${TOOLDIR}/bin/${TARGET}-gcc
LD	=	${TOOLDIR}/bin/${TARGET}-ld
CXX	=	${TOOLDIR}/bin/${TARGET}-g++
AS	=	${TOOLDIR}/bin/${TARGET}-as
CPP	=	${TOOLDIR}/bin/${TARGET}-cpp
RANLIB	=	${TOOLDIR}/bin/${TARGET}-ranlib
AR	=	${TOOLDIR}/bin/${TARGET}-ar
NM	=	${TOOLDIR}/bin/${TARGET}-nm
SIZE	=	${TOOLDIR}/bin/${TARGET}-size
STRIP	=	${TOOLDIR}/bin/${TARGET}-strip

CFLAGS+= -mcpu=r5900 -mips1 -fno-pic \
		-D__NetBSD__ -Ulinux -U__linux__ -U__linux
AFLAGS+= -mcpu=r5900 -mips1 -fno-pic \
		-D__NetBSD__ -Ulinux -U__linux__ -U__linux
