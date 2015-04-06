#	$NetBSD: Makefile.inc,v 1.1.2.1 2015/04/06 15:18:21 skrll Exp $

# This file is used by modules and rump kernels 

IOCONFDIR:=	${.PARSEDIR}
.PATH:	${NETBSDSRCDIR}/sys/dev/hdaudio ${IOCONFDIR}

IOCONF=	hdafg.ioconf
SRCS=	hdafg.c hdafg_dd.c
