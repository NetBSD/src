#	$NetBSD: Makefile,v 1.25 2025/12/24 20:06:01 thorpej Exp $

.include <bsd.init.mk>

USE_FORT?= yes	# network client

RUMPPRG=traceroute
MAN=	traceroute.8

CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SOCKIO_H=1
CPPFLAGS+=-DHAVE_STRERROR=1 -DHAVE_SETLINEBUF=1 -DHAVE_SOCKADDR_SA_LEN=1
CPPFLAGS+=-DHAVE_RAW_OPTIONS=1
CPPFLAGS+=-DHAVE_ICMP_NEXTMTU=1


.if (${MKIPSEC} != "no")
CPPFLAGS+=-DIPSEC
LDADD+=	-lipsec
DPADD+=	${LIBIPSEC}
.endif

BINOWN= root
BINMODE=4555

SRCS=	traceroute.c ifaddrlist.c
SRCS+= version.c as.c

.PATH: ${.CURDIR}/../../lib/libc/net
RUMPSRCS= getifaddrs.c
CPPFLAGS+= -DRUMP_ACTION

SANITIZER_RENAME_CLASSES+=	rump
SANITIZER_RENAME_FILES.rump+=	${PROG}_rumpops.c ${RUMPSRCS}
SANITIZER_RENAME_SYMBOL.rump+=	getifaddrs

AWKS=	median.awk mean.awk

COPTS.traceroute.c+=	${CC_WNO_ARRAY_BOUNDS}

.include <bsd.prog.mk>
