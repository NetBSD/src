#	$NetBSD: Makefile,v 1.12 2002/09/18 03:54:37 lukem Exp $

.include <bsd.own.mk>

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap
DPADD=	${LIBWRAP}

CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libwrap -DSYS_ERRLIST_DEFINED

.include "${NETBSDSRCDIR}/lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
