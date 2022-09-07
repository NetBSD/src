#	$NetBSD: Makefile.inc,v 1.2 2022/09/07 03:34:44 khorben Exp $

# This file is used by modules and rump kernels

IOCONFDIR:=	${.PARSEDIR}

.PATH:	${NETBSDSRCDIR}/sys/dev/pci ${IOCONFDIR}

IOCONF=	emuxki.ioconf
SRCS+=	emuxki.c
SRCS+=	emuxki_boards.c
