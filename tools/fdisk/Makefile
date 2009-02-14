#	$NetBSD: Makefile,v 1.3 2009/02/14 13:56:41 abs Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	getcap.c disklabel.c

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+= -DBOOTSEL
.endif

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
