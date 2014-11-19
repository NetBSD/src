#	$NetBSD: Makefile.inc,v 1.1 2014/11/19 00:17:10 pooka Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci/hdaudio ${IOCONFDIR}

IOCONF=	hdaudio.ioconf
SRCS=	hdaudio.c
SRCS+=	hdaudio_pci.c
