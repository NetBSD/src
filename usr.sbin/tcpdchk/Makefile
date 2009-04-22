#	$NetBSD: Makefile,v 1.13 2009/04/22 15:23:08 lukem Exp $

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
