#	$NetBSD: Makefile,v 1.2 2001/11/11 01:35:21 jmc Exp $

HOST_SRCDIR=	usr.sbin/mtree

.include "${.CURDIR}/../Makefile.host"

HOST_CPPFLAGS+=-DNO_MD5 -DNO_RMD160 -DNO_SHA1
${TIMESTAMP}: ${SRCS}
