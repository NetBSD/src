#	$NetBSD: Makefile,v 1.9 2005/06/12 20:24:14 dyoung Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}installboot
HOST_SRCDIR=	usr.sbin/installboot

HOST_CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../mips-elf2ecoff

.include "${.CURDIR}/../Makefile.disklabel"
.include "${.CURDIR}/../Makefile.host"

