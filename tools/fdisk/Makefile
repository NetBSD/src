#	$NetBSD: Makefile,v 1.4.6.2 2012/10/30 19:00:12 yamt Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	disklabel.c

HOST_CPPFLAGS=	-DDEFAULT_BOOTDIR=\"${DESTDIR}/usr/mdec\"

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
