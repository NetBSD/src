#	$Netbsd$

PROG=	tcpdmatch
SRCS=	tcpdmatch.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdmatch.8
.PATH.c: ${.CURDIR}/../tcpdchk
CFLAGS+= -I${.CURDIR}/../tcpdchk -I${.CURDIR}/../../lib/libwrap
LDADD=	-lwrap
DPADD=	${LIBWRAP}

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
