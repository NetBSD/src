#	$NetBSD: Makefile.inc,v 1.1 2022/09/07 00:29:23 khorben Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	emuxki.ioconf
SRCS+=	emuxki.c
