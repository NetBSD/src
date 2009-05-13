#	$NetBSD: Makefile,v 1.2.28.1 2009/05/13 19:19:42 jym Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	getcap.c disklabel.c

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+= -DBOOTSEL
.endif

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
