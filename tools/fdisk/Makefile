#	$NetBSD: Makefile,v 1.4.8.1 2012/06/12 19:11:51 riz Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	getcap.c disklabel.c

HOST_CPPFLAGS=	-DDEFAULT_BOOTDIR=\"${DESTDIR}/usr/mdec\"

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
