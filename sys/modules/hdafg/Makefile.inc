#	$NetBSD: Makefile.inc,v 1.2 2015/03/28 14:09:59 jmcneill Exp $

# This file is used by modules and rump kernels 

IOCONFDIR:=	${.PARSEDIR}
.PATH:	${NETBSDSRCDIR}/sys/dev/hdaudio ${IOCONFDIR}

IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c
