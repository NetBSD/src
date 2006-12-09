#	$NetBSD: Makefile,v 1.2 2006/12/09 20:13:13 dyoung Exp $

HOSTPROGNAME=	nbdisklabel-${MAKEWRAPPERMACHINE}
HOST_SRCDIR=	sbin/disklabel
HOST_SRCS=	getcap.c disklabel.c

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
