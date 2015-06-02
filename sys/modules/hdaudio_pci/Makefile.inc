#	$NetBSD: Makefile.inc,v 1.2 2015/06/02 00:38:38 pooka Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	hdaudio_pci.ioconf
SRCS+=	hdaudio_pci.c
