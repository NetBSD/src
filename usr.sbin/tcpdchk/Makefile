#	$NetBSD: Makefile,v 1.12.40.1 2009/05/13 19:20:41 jym Exp $

WARNS?=	1	# XXX: many issues in lib/libwrap to address first

.include <bsd.own.mk>

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap
DPADD=	${LIBWRAP}

CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libwrap -DSYS_ERRLIST_DEFINED

.include "${NETBSDSRCDIR}/lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
