#	$NetBSD: Makefile,v 1.7 1998/01/09 08:11:50 perry Exp $
#

WARNS?=	0
PROG=	tcpdmatch
SRCS=	tcpdmatch.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdmatch.8
TCPDCHK=${.CURDIR}/../tcpdchk
.PATH: ${TCPDCHK}
CPPFLAGS+= -I${TCPDCHK} -I${.CURDIR}/../../lib/libwrap
LDADD=	-lwrap
DPADD=	${LIBWRAP}

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
