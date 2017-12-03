#	$NetBSD: Makefile.inc,v 1.2.16.2 2017/12/03 11:38:54 jdolecek Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	hdaudio_pci.ioconf
SRCS+=	hdaudio_pci.c
