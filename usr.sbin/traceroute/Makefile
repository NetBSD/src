#	$NetBSD: Makefile,v 1.6 1997/10/04 16:31:24 christos Exp $	

WARNS?=	1
PROG=	traceroute
MAN=	traceroute.8 

CFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SOCKIO_H=1
CFLAGS+=-DHAVE_STRERROR=1 -DHAVE_SETLINEBUF=1 -DHAVE_SOCKADDR_SA_LEN=1 

BINOWN= root
BINMODE=4555

SRCS=	traceroute.c savestr.c ifaddrlist.c 
SRCS+= version.c

AWKS=	median.awk mean.awk 

.include <bsd.prog.mk>
