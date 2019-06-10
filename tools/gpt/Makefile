#	$NetBSD: Makefile,v 1.4.2.1 2019/06/10 22:10:14 christos Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}gpt
HOST_SRCDIR=	sbin/gpt
HOST_CPPFLAGS+=	-I${.CURDIR}

HOST_SRCS+= opendisk.c dehumanize_number.c

.PATH.c: ${.CURDIR}/../../lib/libutil ${.CURDIR}/../../lib/libc/gen

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
