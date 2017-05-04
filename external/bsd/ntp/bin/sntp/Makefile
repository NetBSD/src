# $NetBSD: Makefile,v 1.5.8.3 2017/05/04 05:53:37 snj Exp $

USE_FORT?= yes	# network client/server

PROG=	sntp
MAN=	sntp.1
SRCS= \
	crypto.c \
	kod_management.c \
	log.c \
	main.c \
	networking.c \
	sntp-opts.c \
	sntp.c \
	utilities.c

BINDIR=	/usr/sbin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/sntp

CPPFLAGS+=-I${DIST} -I${DIST}/libopts

.PATH: ${DIST}

.include <bsd.own.mk>

CPPFLAGS+= -DLOCKNAME=\"/var/run/sntp.pid\"
CPPFLAGS+= -DSAVENAME=\"/var/db/sntp.state\"
LDADD+= -L${LIBISC} -liscntp -levent -lm
DPADD+=	${LIBM} ${LIBEVENT}

WARNS=1
LIBEVENTDIR=${.CURDIR}/../../dist/sntp/libevent
CPPFLAGS+=-I${LIBEVENTDIR}/include -I${.CURDIR}
.PATH: ${LIBEVENTDIR}
SRCS+= \
       event.c \
       evmap.c \
       evthread.c \
       evutil.c \
       evutil_rand.c \
       evutil_time.c \
       evlog.c \
       kqueue.c \
       signal.c \


.include <bsd.prog.mk>
