#	$Netbsd$

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD=	-lwrap
DPADD=	${LIBWRAP}

CPPFLAGS+= -I${.CURDIR}/../../lib/libwrap

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
