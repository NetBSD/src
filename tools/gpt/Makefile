#	$NetBSD: Makefile,v 1.3.14.1 2018/05/02 07:20:26 pgoyette Exp $

HOSTPROGNAME=	nbgpt
HOST_SRCDIR=	sbin/gpt
HOST_CPPFLAGS+=	-I${.CURDIR}

HOST_SRCS+= opendisk.c dehumanize_number.c

.PATH.c: ${.CURDIR}/../../lib/libutil ${.CURDIR}/../../lib/libc/gen

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
