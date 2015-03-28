#	$NetBSD: Makefile.inc,v 1.1 2015/03/28 14:09:59 jmcneill Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	hdaudio_pci.ioconf
SRCS=	hdaudio_pci.c
