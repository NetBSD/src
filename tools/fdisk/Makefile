#	$NetBSD: Makefile,v 1.8 2021/09/18 01:47:10 christos Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-fdisk
HOST_SRCDIR=	sbin/fdisk
HOST_SRCS=	disklabel.c

HOST_CPPFLAGS+=	-DDEFAULT_BOOTDIR=\"${DESTDIR}/usr/mdec\"

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"

.PATH: ${.CURDIR}/../../lib/libc/gen
