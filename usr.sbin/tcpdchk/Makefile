#	$NetBSD: Makefile,v 1.1.1.2 1999/08/27 15:16:17 itojun Exp $

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-L/usr/home/itojun/NetBSD2/src/lib/libwrap -lwrap
DPADD=	${LIBWRAP}

CPPFLAGS+= -I${.CURDIR}/../../lib/libwrap

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>

CFLAGS=
