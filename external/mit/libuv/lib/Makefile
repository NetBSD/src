# $NetBSD: Makefile,v 1.6 2022/09/22 21:22:50 christos Exp $

LIBISPRIVATE=pic

.include <bsd.own.mk>

MKLINT=	no
LIB=	uv

.if ${LIBISPRIVATE:Uno} == "no"
INCS=	uv.h uv/unix.h uv/bsd.h uv/errno.h uv/version.h uv/threadpool.h
INCSDIR=/usr/include
MAN=	libuv.3
.endif

LIBUVDIR=${.CURDIR}/../dist

.PATH: ${LIBUVDIR}/src ${LIBUVDIR}/src/unix ${LIBUVDIR}/include
SRCS+= \
async.c \
atomic-ops.h \
core.c \
dl.c \
fs.c \
getaddrinfo.c \
getnameinfo.c \
internal.h \
loop-watcher.c \
loop.c \
pipe.c \
poll.c \
process.c \
random-devurandom.c \
signal.c \
stream.c \
strtok.c \
tcp.c \
thread.c \
tty.c \
udp.c

SRCS+= \
bsd-ifaddrs.c \
bsd-proctitle.c \
kqueue.c \
netbsd.c \
posix-hrtime.c

SRCS+= \
fs-poll.c \
idna.c \
inet.c \
random.c \
strscpy.c \
threadpool.c \
timer.c \
uv-data-getter-setters.c \
uv-common.c \
version.c

CPPFLAGS+=	-I${LIBUVDIR}/include -I${LIBUVDIR}/src

LIBDPLIBS+=	kvm ${NETBSDSRCDIR}/lib/libkvm

.if ${LIBISPRIVATE} != "yes" && ${LIBISPRIVATE} != "pic"
SHLIB_MAJOR=	1
SHLIB_MINOR=	0

PKGCONFIG=libuv
.include "${.CURDIR}/../pkgconfig.mk"
.endif

.include <bsd.lib.mk>
