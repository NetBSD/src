#	$NetBSD: Makefile,v 1.9 2002/05/24 11:29:40 tron Exp $
#

PROG=	tcpdmatch
SRCS=	tcpdmatch.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdmatch.8
TCPDCHK=${.CURDIR}/../tcpdchk
.PATH: ${TCPDCHK}
CPPFLAGS+= -I${TCPDCHK} -I${.CURDIR}/../../lib/libwrap -DSYS_ERRLIST_DEFINED
LDADD=	-lwrap
DPADD=	${LIBWRAP}

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
