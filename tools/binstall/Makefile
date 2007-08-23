#	$NetBSD: Makefile,v 1.5.2.1 2007/08/23 11:21:44 liamjfoy Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-install
HOST_SRCDIR=	usr.bin/xinstall
CPPFLAGS+=	-I${.CURDIR}/../compat/sys
CPPFLAGS+=	-DTARGET_STRIP=\"${STRIP}\"

.include "${.CURDIR}/../Makefile.host"

INSTALL=	./xinstall
