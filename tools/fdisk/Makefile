#	$NetBSD: Makefile,v 1.1 2005/06/12 19:46:17 dyoung Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	getcap.c disklabel.c

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
