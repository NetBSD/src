#	$NetBSD: Makefile,v 1.10.2.1 2011/02/08 16:20:10 bouyer Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}installboot
HOST_SRCDIR=	usr.sbin/installboot

HOST_CPPFLAGS+=	-I. -I${.CURDIR} -I${.CURDIR}/../mips-elf2ecoff
HOST_CPPFLAGS+=	-I${TOOLDIR}/include/nbinclude

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"

