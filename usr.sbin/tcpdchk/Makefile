#	$NetBSD: Makefile,v 1.11 2002/05/24 09:57:25 tron Exp $

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap
DPADD=	${LIBWRAP}

CPPFLAGS+= -I${.CURDIR}/../../lib/libwrap -DSYS_ERRLIST_DEFINED

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
