#	$NetBSD: Makefile,v 1.10 2002/09/18 03:54:38 lukem Exp $
#

.include <bsd.own.mk>

PROG=	tcpdmatch
SRCS=	tcpdmatch.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdmatch.8
TCPDCHK=${.CURDIR}/../tcpdchk
.PATH: ${TCPDCHK}
CPPFLAGS+= -I${TCPDCHK} -I${NETBSDSRCDIR}/lib/libwrap -DSYS_ERRLIST_DEFINED
LDADD=	-lwrap
DPADD=	${LIBWRAP}

.include "${NETBSDSRCDIR}/lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
