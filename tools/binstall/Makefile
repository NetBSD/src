#	$NetBSD: Makefile,v 1.5.4.1 2007/09/03 07:05:24 wrstuden Exp $

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-install
HOST_SRCDIR=	usr.bin/xinstall
CPPFLAGS+=	-I${.CURDIR}/../compat/sys
CPPFLAGS+=	-DTARGET_STRIP=\"${STRIP}\"

.include "${.CURDIR}/../Makefile.host"

INSTALL=	./xinstall
