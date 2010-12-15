#	$NetBSD: Makefile,v 1.17 2010/12/15 00:09:41 pooka Exp $	

WARNS?=	1	# XXX: out of date third-party program 

USE_FORT?= yes	# network client

RUMPPRG=traceroute
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

.PATH: ${.CURDIR}/../../lib/libc/net
RUMPSRCS= getifaddrs.c
CPPFLAGS+= -DRUMP_ACTION

AWKS=	median.awk mean.awk 

.include <bsd.prog.mk>
