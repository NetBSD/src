#	$NetBSD: Makefile.inc,v 1.1.2.3 2015/06/06 14:40:24 skrll Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	hdaudio_pci.ioconf
SRCS+=	hdaudio_pci.c
