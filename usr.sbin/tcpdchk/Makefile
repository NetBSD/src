#	$NetBSD: Makefile,v 1.14 2021/03/07 15:09:12 christos Exp $

WARNS?=	1	# XXX: many issues in lib/libwrap to address first

.include <bsd.own.mk>

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap -lblocklist 
DPADD=	${LIBWRAP} ${LIBBLOCKLIST} 

CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libwrap -DSYS_ERRLIST_DEFINED

.include "${NETBSDSRCDIR}/lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
