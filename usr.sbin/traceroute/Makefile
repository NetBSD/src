#	$NetBSD: Makefile,v 1.15.20.1 2009/05/13 19:20:42 jym Exp $	

WARNS?=	1	# XXX: out of date third-party program 

USE_FORT?= yes	# network client

PROG=	traceroute
MAN=	traceroute.8 

CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SOCKIO_H=1
CPPFLAGS+=-DHAVE_STRERROR=1 -DHAVE_SETLINEBUF=1 -DHAVE_SOCKADDR_SA_LEN=1 
CPPFLAGS+=-DHAVE_RAW_OPTIONS=1

CPPFLAGS+=-DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}

BINOWN= root
BINMODE=4555

SRCS=	traceroute.c ifaddrlist.c 
SRCS+= version.c as.c

AWKS=	median.awk mean.awk 

.include <bsd.prog.mk>
