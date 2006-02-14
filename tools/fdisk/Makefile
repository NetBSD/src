#	$NetBSD: Makefile,v 1.2 2006/02/14 04:49:50 dyoung Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	getcap.c disklabel.c

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
CPPFLAGS+= -DBOOTSEL
.endif

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
