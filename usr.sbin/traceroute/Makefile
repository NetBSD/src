#	$NetBSD: Makefile,v 1.12 2001/11/04 23:14:35 atatat Exp $	

PROG=	traceroute
MAN=	traceroute.8 

CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SOCKIO_H=1
CPPFLAGS+=-DHAVE_STRERROR=1 -DHAVE_SETLINEBUF=1 -DHAVE_SOCKADDR_SA_LEN=1 
CPPFLAGS+=-DHAVE_RAW_OPTIONS=1 -DHAVE_IFADDRS_H

CPPFLAGS+=-DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

BINOWN= root
BINMODE=4555

SRCS=	traceroute.c savestr.c ifaddrlist.c 
SRCS+= version.c as.c

AWKS=	median.awk mean.awk 

.include <bsd.prog.mk>
