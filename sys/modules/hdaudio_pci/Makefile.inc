#	$NetBSD: Makefile.inc,v 1.1.2.2 2015/04/06 15:18:21 skrll Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	hdaudio_pci.ioconf
SRCS=	hdaudio_pci.c
